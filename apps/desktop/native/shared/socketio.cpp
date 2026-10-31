#include "socketio.h"

#include "log.h"
#include "system.h"

#include <algorithm>
#include <vector>

namespace fileship {

namespace {

constexpr int INITIAL_DELAY_MS = 1000;
constexpr int MAX_DELAY_MS = 5000;
constexpr int MAX_REJECTED_DELAY_MS = 60000;
constexpr int RECEIVE_TIMEOUT_MS = 60000;
constexpr int CONNECT_TIMEOUT_MS = 15000;

int jitter(int delay) {
    uint32_t random = 0;
    randomBytes(&random, sizeof(random));
    double factor = 0.5 + (random % 1000) / 1000.0;
    return static_cast<int>(delay * factor);
}

}

SocketIo::SocketIo(HttpEndpoint endpoint) : endpoint(std::move(endpoint)) {}

SocketIo::~SocketIo() {
    close();
    if (worker.joinable() && worker.get_id() != std::this_thread::get_id()) worker.join();
}

void SocketIo::on(std::wstring const& event, Handler handler) {
    std::lock_guard lock(mutex);
    handlers[event] = std::move(handler);
}

void SocketIo::onReconnect(std::function<void()> handler) {
    std::lock_guard lock(mutex);
    reconnectHandler = std::move(handler);
}

void SocketIo::onConnectError(std::function<void()> handler) {
    std::lock_guard lock(mutex);
    connectErrorHandler = std::move(handler);
}

void SocketIo::connect() {
    if (worker.joinable()) return;

    stopping = false;
    worker = std::thread([this] { run(); });
}

void SocketIo::close() {
    stopping = true;
    wake.notify_all();
    closeSocket();

    if (!worker.joinable()) return;
    if (worker.get_id() == std::this_thread::get_id()) {
        worker.detach();
    } else {
        worker.join();
    }
}

bool SocketIo::isConnected() const {
    return connected;
}

void SocketIo::emit(std::wstring const& event, IJsonValue const& payload) {
    JsonArray packet;
    packet.Append(JsonValue::CreateStringValue(event));
    packet.Append(payload);
    sendText("42" + stringify(packet));
}

bool SocketIo::waitFor(int delayMs) {
    std::unique_lock lock(mutex);
    wake.wait_for(lock, std::chrono::milliseconds(delayMs), [this] { return stopping.load(); });
    return !stopping;
}

void SocketIo::run() {
    int attempt = 0;

    while (!stopping) {
        rejected = false;

        if (open()) {
            std::shared_ptr<WebSocket> current;
            {
                std::lock_guard lock(mutex);
                current = socket;
            }

            while (!stopping && current) {
                auto packet = current->receive();
                if (!packet) break;

                try {
                    if (!packet->empty()) handlePacket(*packet);
                } catch (std::exception const& failure) {
                    logWarn(L"Socket packet failed: " + toWide(failure.what()));
                }

                if (rejected) break;
            }

            if (connected && !rejected) attempt = 0;
        }

        if (connected) {
            logInfo(L"Socket: disconnected");
        }

        connected = false;
        closeSocket();
        hasAttempted = true;
        if (stopping) break;

        int delay = jitter(std::min(INITIAL_DELAY_MS << std::min(attempt, 8),
                                    rejected ? MAX_REJECTED_DELAY_MS : MAX_DELAY_MS));
        attempt++;
        if (!waitFor(delay)) break;
    }

    connected = false;
}

bool SocketIo::open() {
    auto created = std::make_shared<WebSocket>();
    int status = 0;

    try {
        status = created->open(endpoint, L"/socket.io/?EIO=4&transport=websocket", CONNECT_TIMEOUT_MS,
                               RECEIVE_TIMEOUT_MS);
    } catch (std::invalid_argument const&) {
        logWarn(L"Socket: invalid instance address " + endpoint.baseUrl);
        stopping = true;
        return false;
    }

    if (status != 101) {
        if (status) {
            logWarn(L"Socket: upgrade refused with " + std::to_wstring(status));
        } else {
            logWarn(L"Socket: " + toWide(created->error));
        }
        return false;
    }

    std::lock_guard lock(mutex);
    if (stopping) {
        created->close();
        return false;
    }

    socket = std::move(created);
    return true;
}

void SocketIo::closeSocket() {
    std::shared_ptr<WebSocket> current;
    {
        std::lock_guard sending(sendMutex);
        std::lock_guard lock(mutex);
        current = std::move(socket);
    }

    if (current) current->close();
}

bool SocketIo::sendText(std::string const& text) {
    std::lock_guard sending(sendMutex);
    std::shared_ptr<WebSocket> current;

    {
        std::lock_guard lock(mutex);
        current = socket;
    }

    return current && current->send(text);
}

void SocketIo::handlePacket(std::string const& packet) {
    char type = packet[0];

    if (type == '0') {
        JsonObject auth;
        auth.SetNamedValue(L"token", JsonValue::CreateStringValue(endpoint.token));
        sendText("40" + stringify(auth));
        return;
    }

    if (type == '2') {
        sendText("3" + packet.substr(1));
        return;
    }

    if (type == '1') {
        closeSocket();
        return;
    }

    if (type != '4' || packet.size() < 2) return;

    char kind = packet[1];

    if (kind == '0') {
        connected = true;

        std::function<void()> handler;
        {
            std::lock_guard lock(mutex);
            handler = reconnectHandler;
        }

        if (hasAttempted && handler) handler();
    } else if (kind == '1') {
        logInfo(L"Socket: the server closed the connection");
        rejected = true;
    } else if (kind == '4') {
        logWarn(L"Socket: connection refused " + toWide(packet.substr(2)));
        rejected = true;

        std::function<void()> handler;
        {
            std::lock_guard lock(mutex);
            handler = connectErrorHandler;
        }

        if (handler) handler();
    } else if (kind == '2') {
        dispatch(packet.substr(2));
    }
}

void SocketIo::dispatch(std::string const& data) {
    size_t start = 0;
    if (!data.empty() && data[0] == '/') {
        start = data.find(',');
        if (start == std::string::npos) return;
        start++;
    }

    while (start < data.size() && data[start] >= '0' && data[start] <= '9') start++;

    auto packet = parseArray(std::string_view(data).substr(start));
    if (!packet || packet->Size() == 0 || packet->GetAt(0).ValueType() != JsonValueType::String) return;

    std::wstring name(packet->GetAt(0).GetString());
    IJsonValue payload = packet->Size() > 1 ? packet->GetAt(1) : JsonValue::CreateNullValue();

    Handler handler;
    {
        std::lock_guard lock(mutex);
        auto found = handlers.find(name);
        if (found == handlers.end()) return;
        handler = found->second;
    }

    try {
        handler(payload);
    } catch (std::exception const& failure) {
        logWarn(L"Socket handler " + name + L" failed: " + toWide(failure.what()));
    }
}

}

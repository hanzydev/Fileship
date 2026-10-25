#include "socketio.h"

#include "log.h"

#include <bcrypt.h>

#include <algorithm>
#include <iterator>
#include <vector>

namespace fileship {

namespace {

constexpr int INITIAL_DELAY_MS = 1000;
constexpr int MAX_DELAY_MS = 5000;
constexpr int MAX_REJECTED_DELAY_MS = 60000;
constexpr DWORD RECEIVE_TIMEOUT_MS = 60000;
constexpr DWORD CONNECT_TIMEOUT_MS = 15000;

int jitter(int delay) {
    uint32_t random = 0;
    BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(&random), sizeof(random),
                    BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    double factor = 0.5 + (random % 1000) / 1000.0;
    return static_cast<int>(delay * factor);
}

}

SocketIo::SocketIo(HttpEndpoint endpoint) : endpoint(std::move(endpoint)) {}

SocketIo::~SocketIo() {
    close();
    if (worker.joinable() && worker.get_id() != std::this_thread::get_id()) worker.join();
    if (internet) WinHttpCloseHandle(internet);
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
            std::vector<char> buffer(64 * 1024);
            std::string message;

            while (!stopping) {
                HINTERNET current = nullptr;
                {
                    std::lock_guard lock(mutex);
                    current = socket;
                }
                if (!current) break;

                DWORD read = 0;
                WINHTTP_WEB_SOCKET_BUFFER_TYPE type{};
                DWORD error = WinHttpWebSocketReceive(current, buffer.data(),
                                                      static_cast<DWORD>(buffer.size()), &read, &type);
                if (error != NO_ERROR || type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) break;

                if (type == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE ||
                    type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE) {
                    message.append(buffer.data(), read);
                }

                if (type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE) {
                    std::string packet = std::move(message);
                    message.clear();

                    try {
                        if (!packet.empty()) handlePacket(packet);
                    } catch (winrt::hresult_error const& failure) {
                        logWarn(L"Socket packet failed: " + std::wstring(failure.message()));
                    } catch (std::exception const& failure) {
                        logWarn(L"Socket packet failed: " + toWide(failure.what()));
                    }
                } else if (type == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE) {
                    message.clear();
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

bool SocketIo::session() {
    if (internet) return true;

    internet = WinHttpOpen(endpoint.userAgent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                           WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    return internet != nullptr;
}

bool SocketIo::open() {
    if (!session()) {
        logWarn(L"Socket: WinHttpOpen failed with " + std::to_wstring(GetLastError()));
        return false;
    }

    URL_COMPONENTS url{};
    url.dwStructSize = sizeof(url);
    wchar_t host[256]{};
    wchar_t base[2048]{};
    url.lpszHostName = host;
    url.dwHostNameLength = static_cast<DWORD>(std::size(host));
    url.lpszUrlPath = base;
    url.dwUrlPathLength = static_cast<DWORD>(std::size(base));

    if (!WinHttpCrackUrl(endpoint.baseUrl.c_str(), 0, 0, &url)) {
        logWarn(L"Socket: invalid instance address " + endpoint.baseUrl);
        stopping = true;
        return false;
    }

    std::wstring path(base);
    while (!path.empty() && path.back() == L'/') path.pop_back();
    path += L"/socket.io/?EIO=4&transport=websocket";

    HINTERNET connect = WinHttpConnect(internet, host, url.nPort, 0);
    if (!connect) return false;

    DWORD flags = url.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connect, L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                           WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        WinHttpCloseHandle(connect);
        return false;
    }

    WinHttpSetTimeouts(request, CONNECT_TIMEOUT_MS, CONNECT_TIMEOUT_MS, CONNECT_TIMEOUT_MS,
                       RECEIVE_TIMEOUT_MS);
    WinHttpSetOption(request, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0);

    std::wstring headers = L"User-Agent: " + endpoint.userAgent + L"\r\n";
    if (!endpoint.token.empty()) headers += L"Authorization: Bearer " + endpoint.token + L"\r\n";

    DWORD status = 0;
    DWORD size = sizeof(status);
    bool sent = WinHttpSendRequest(request, headers.c_str(), static_cast<DWORD>(headers.size()),
                                   WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                WinHttpReceiveResponse(request, nullptr) &&
                WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                    WINHTTP_HEADER_NAME_BY_INDEX, &status, &size,
                                    WINHTTP_NO_HEADER_INDEX);

    if (!sent || status != 101) {
        if (!sent) {
            logWarn(L"Socket: connection failed with " + std::to_wstring(GetLastError()));
        } else {
            logWarn(L"Socket: upgrade refused with " + std::to_wstring(status));
        }
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        return false;
    }

    HINTERNET upgraded = WinHttpWebSocketCompleteUpgrade(request, 0);
    WinHttpCloseHandle(request);

    if (!upgraded) {
        WinHttpCloseHandle(connect);
        return false;
    }

    DWORD timeout = RECEIVE_TIMEOUT_MS;
    WinHttpSetOption(upgraded, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    std::lock_guard lock(mutex);
    if (stopping) {
        WinHttpCloseHandle(upgraded);
        WinHttpCloseHandle(connect);
        return false;
    }

    connection = connect;
    socket = upgraded;
    return true;
}

void SocketIo::closeSocket() {
    std::lock_guard sending(sendMutex);
    std::lock_guard lock(mutex);

    if (socket) {
        WinHttpWebSocketShutdown(socket, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, nullptr, 0);
        WinHttpCloseHandle(socket);
        socket = nullptr;
    }

    if (connection) {
        WinHttpCloseHandle(connection);
        connection = nullptr;
    }
}

bool SocketIo::sendText(std::string const& text) {
    std::lock_guard sending(sendMutex);
    HINTERNET current = nullptr;

    {
        std::lock_guard lock(mutex);
        current = socket;
    }

    if (!current) return false;

    return WinHttpWebSocketSend(current, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
                                const_cast<char*>(text.data()), static_cast<DWORD>(text.size())) == NO_ERROR;
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
    } catch (winrt::hresult_error const& failure) {
        logWarn(L"Socket handler " + name + L" failed: " + std::wstring(failure.message()));
    } catch (std::exception const& failure) {
        logWarn(L"Socket handler " + name + L" failed: " + toWide(failure.what()));
    }
}

}

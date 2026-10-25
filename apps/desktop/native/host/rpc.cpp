#include "rpc.h"

#include "app.h"

#include <shared/log.h>

#include <aclapi.h>
#include <sddl.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

namespace fileship::host {

namespace {

constexpr DWORD BUFFER_SIZE = 64 * 1024;
constexpr size_t MAX_LINE = 256 * 1024 * 1024;

struct Client {
    uint64_t id = 0;
    HANDLE pipe = INVALID_HANDLE_VALUE;
    std::mutex writeMutex;
    std::atomic<bool> isOpen = true;
    bool isWindow = false;
    DWORD pid = 0;
};

std::mutex handlersMutex;
std::map<std::wstring, RpcHandler> handlers;

std::mutex clientsMutex;
std::map<uint64_t, std::shared_ptr<Client>> clients;
std::vector<std::function<void(uint64_t)>> closeListeners;
std::atomic<uint64_t> nextClientId = 1;
std::atomic<bool> isStopping = false;
HANDLE stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
std::thread listenerThread;

thread_local uint64_t activeClient = 0;

std::wstring securityDescriptor() {
    HANDLE processToken = nullptr;
    std::wstring sid;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &processToken)) {
        DWORD size = 0;
        GetTokenInformation(processToken, TokenUser, nullptr, 0, &size);
        std::vector<uint8_t> buffer(size);

        if (GetTokenInformation(processToken, TokenUser, buffer.data(), size, &size)) {
            LPWSTR text = nullptr;
            if (ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid, &text)) {
                sid = text;
                LocalFree(text);
            }
        }

        CloseHandle(processToken);
    }

    return L"D:P(A;;GA;;;" + sid + L")";
}

HANDLE createPipe(bool isFirst) {
    static std::wstring const descriptor = securityDescriptor();

    PSECURITY_DESCRIPTOR security = nullptr;
    ConvertStringSecurityDescriptorToSecurityDescriptorW(descriptor.c_str(), SDDL_REVISION_1, &security, nullptr);

    SECURITY_ATTRIBUTES attributes{sizeof(attributes), security, FALSE};
    DWORD mode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | (isFirst ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0);

    HANDLE pipe = CreateNamedPipeW(pipeName().c_str(), mode, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT |
                                                                 PIPE_REJECT_REMOTE_CLIENTS,
                                   PIPE_UNLIMITED_INSTANCES, BUFFER_SIZE, BUFFER_SIZE, 0, &attributes);

    if (security) LocalFree(security);
    return pipe;
}

bool writeAll(Client& client, std::string const& line) {
    std::lock_guard lock(client.writeMutex);
    if (!client.isOpen) return false;

    OVERLAPPED overlapped{};
    overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    size_t offset = 0;
    bool isOk = true;

    while (offset < line.size()) {
        DWORD written = 0;
        ResetEvent(overlapped.hEvent);
        auto chunk = static_cast<DWORD>(std::min<size_t>(line.size() - offset, 1024 * 1024));

        if (!WriteFile(client.pipe, line.data() + offset, chunk, nullptr, &overlapped) &&
            GetLastError() != ERROR_IO_PENDING) {
            isOk = false;
            break;
        }

        if (!GetOverlappedResult(client.pipe, &overlapped, &written, TRUE) || !written) {
            isOk = false;
            break;
        }

        offset += written;
    }

    CloseHandle(overlapped.hEvent);
    return isOk;
}

void send(Client& client, JsonObject const& message) {
    if (!writeAll(client, stringify(message) + "\n")) client.isOpen = false;
}

std::shared_ptr<Client> findClient(uint64_t id) {
    std::lock_guard lock(clientsMutex);
    auto found = clients.find(id);
    return found == clients.end() ? nullptr : found->second;
}

void respond(std::shared_ptr<Client> const& client, JsonValue const& id, std::string const& line) {
    JsonObject response;
    response.SetNamedValue(L"id", id);

    auto request = parseObject(line);
    if (!request) {
        response.SetNamedValue(L"ok", JsonValue::CreateBooleanValue(false));
        response.SetNamedValue(L"error", JsonValue::CreateStringValue(L"Invalid request"));
        send(*client, response);
        return;
    }

    auto channel = jsonString(*request, L"channel");
    auto args = request->HasKey(L"args") && request->GetNamedValue(L"args").ValueType() == JsonValueType::Array
                    ? request->GetNamedArray(L"args")
                    : JsonArray();

    activeClient = client->id;

    try {
        auto result = invokeHandler(channel, args);
        response.SetNamedValue(L"ok", JsonValue::CreateBooleanValue(true));
        response.SetNamedValue(L"result", result ? result : IJsonValue(JsonValue::CreateNullValue()));
    } catch (ApiError const& error) {
        response.SetNamedValue(L"ok", JsonValue::CreateBooleanValue(false));
        response.SetNamedValue(L"error", JsonValue::CreateStringValue(errorMessage(error)));
        response.SetNamedValue(L"data", error.data ? error.data : JsonObject());
    } catch (std::exception const& error) {
        response.SetNamedValue(L"ok", JsonValue::CreateBooleanValue(false));
        response.SetNamedValue(L"error", JsonValue::CreateStringValue(errorMessage(error)));
    } catch (winrt::hresult_error const& error) {
        response.SetNamedValue(L"ok", JsonValue::CreateBooleanValue(false));
        response.SetNamedValue(L"error", JsonValue::CreateStringValue(error.message()));
    }

    activeClient = 0;
    send(*client, response);
}

void handleLine(std::shared_ptr<Client> const& client, std::string line) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line.empty()) return;

    runAsync([client, line = std::move(line)] {
        JsonValue id = JsonValue::CreateNullValue();
        if (auto request = parseObject(line); request && request->HasKey(L"id")) id = request->GetNamedValue(L"id");
        respond(client, id, line);
    });
}

void closeClient(std::shared_ptr<Client> const& client) {
    {
        std::lock_guard lock(client->writeMutex);
        client->isOpen = false;
        FlushFileBuffers(client->pipe);
        DisconnectNamedPipe(client->pipe);
        CloseHandle(client->pipe);
    }

    std::vector<std::function<void(uint64_t)>> listeners;
    {
        std::lock_guard lock(clientsMutex);
        clients.erase(client->id);
        listeners = closeListeners;
    }

    if (client->isWindow) removeWindowProcess(client->pid);
    for (auto const& listener : listeners) listener(client->id);
}

void serve(std::shared_ptr<Client> client) {
    OVERLAPPED overlapped{};
    overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    std::vector<char> buffer(BUFFER_SIZE);
    std::string pending;

    while (client->isOpen && !isStopping) {
        DWORD read = 0;
        ResetEvent(overlapped.hEvent);

        if (!ReadFile(client->pipe, buffer.data(), BUFFER_SIZE, nullptr, &overlapped) &&
            GetLastError() != ERROR_IO_PENDING && GetLastError() != ERROR_MORE_DATA)
            break;

        HANDLE events[] = {overlapped.hEvent, stopEvent};
        if (WaitForMultipleObjects(2, events, FALSE, INFINITE) != WAIT_OBJECT_0) {
            CancelIoEx(client->pipe, &overlapped);
            GetOverlappedResult(client->pipe, &overlapped, &read, TRUE);
            break;
        }

        if (!GetOverlappedResult(client->pipe, &overlapped, &read, FALSE) && GetLastError() != ERROR_MORE_DATA)
            break;

        pending.append(buffer.data(), read);
        if (pending.size() > MAX_LINE) break;

        size_t newline;
        while ((newline = pending.find('\n')) != std::string::npos) {
            handleLine(client, pending.substr(0, newline));
            pending.erase(0, newline + 1);
        }
    }

    CloseHandle(overlapped.hEvent);
    closeClient(client);
}

void listen() {
    bool isFirst = true;

    while (!isStopping) {
        HANDLE pipe = createPipe(isFirst);
        if (pipe == INVALID_HANDLE_VALUE) {
            logError(L"Could not create the host pipe: " + std::to_wstring(GetLastError()));
            if (WaitForSingleObject(stopEvent, 1000) == WAIT_OBJECT_0) return;
            continue;
        }

        isFirst = false;

        OVERLAPPED overlapped{};
        overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        bool isConnected = ConnectNamedPipe(pipe, &overlapped) != FALSE;
        DWORD error = GetLastError();

        if (!isConnected && error == ERROR_IO_PENDING) {
            HANDLE events[] = {overlapped.hEvent, stopEvent};
            if (WaitForMultipleObjects(2, events, FALSE, INFINITE) == WAIT_OBJECT_0) {
                DWORD ignored = 0;
                isConnected = GetOverlappedResult(pipe, &overlapped, &ignored, FALSE) != FALSE;
            } else {
                CancelIoEx(pipe, &overlapped);
            }
        } else if (error == ERROR_PIPE_CONNECTED) {
            isConnected = true;
        }

        CloseHandle(overlapped.hEvent);

        if (!isConnected || isStopping) {
            CloseHandle(pipe);
            continue;
        }

        auto client = std::make_shared<Client>();
        client->id = nextClientId++;
        client->pipe = pipe;

        {
            std::lock_guard lock(clientsMutex);
            clients[client->id] = client;
        }

        std::thread(serve, client).detach();
    }
}

}

void registerHandler(std::wstring const& channel, RpcHandler handler) {
    std::lock_guard lock(handlersMutex);
    handlers[channel] = std::move(handler);
}

IJsonValue invokeHandler(std::wstring const& channel, JsonArray const& args) {
    RpcHandler handler;

    {
        std::lock_guard lock(handlersMutex);
        auto found = handlers.find(channel);
        if (found == handlers.end()) throw std::runtime_error(toUtf8(L"Unknown channel: " + channel));
        handler = found->second;
    }

    return handler(args);
}

void broadcast(std::wstring const& event, IJsonValue const& payload) {
    std::vector<std::shared_ptr<Client>> targets;

    {
        std::lock_guard lock(clientsMutex);
        for (auto const& [id, client] : clients) targets.push_back(client);
    }

    JsonObject message;
    message.SetNamedValue(L"event", JsonValue::CreateStringValue(event));
    message.SetNamedValue(L"payload", payload ? payload : IJsonValue(JsonValue::CreateNullValue()));

    for (auto const& client : targets) send(*client, message);
}

uint64_t currentClient() {
    return activeClient;
}

void sendTo(uint64_t id, std::wstring const& event, IJsonValue const& payload) {
    auto client = findClient(id);
    if (!client) return;

    JsonObject message;
    message.SetNamedValue(L"event", JsonValue::CreateStringValue(event));
    message.SetNamedValue(L"payload", payload ? payload : IJsonValue(JsonValue::CreateNullValue()));
    send(*client, message);
}

void onClientClosed(std::function<void(uint64_t)> listener) {
    std::lock_guard lock(clientsMutex);
    closeListeners.push_back(std::move(listener));
}

bool hasWindowClient() {
    std::lock_guard lock(clientsMutex);

    for (auto const& [id, client] : clients) {
        if (client->isWindow && client->isOpen) return true;
    }

    return false;
}

void handleWindowClient(uint64_t id, DWORD pid) {
    auto client = findClient(id);
    if (!client) return;

    {
        std::lock_guard lock(clientsMutex);
        client->isWindow = true;
        client->pid = pid;
    }

    addWindowProcess(pid);
}

void startRpcServer() {
    listenerThread = std::thread(listen);
}

void stopRpcServer() {
    isStopping = true;
    SetEvent(stopEvent);
    if (listenerThread.joinable()) listenerThread.join();
}

bool forwardToRunningHost(std::vector<std::wstring> const& arguments) {
    auto name = pipeName();
    HANDLE pipe = INVALID_HANDLE_VALUE;
    auto deadline = GetTickCount64() + 10000;

    while (GetTickCount64() < deadline) {
        pipe = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (pipe != INVALID_HANDLE_VALUE) break;

        if (GetLastError() == ERROR_PIPE_BUSY) WaitNamedPipeW(name.c_str(), 1000);
        else Sleep(100);
    }

    if (pipe == INVALID_HANDLE_VALUE) return false;

    JsonArray argv;
    for (auto const& argument : arguments) argv.Append(JsonValue::CreateStringValue(argument));

    JsonArray args;
    args.Append(argv);
    args.Append(JsonValue::CreateStringValue(std::filesystem::current_path().wstring()));

    JsonObject message;
    message.SetNamedValue(L"id", JsonValue::CreateNumberValue(1));
    message.SetNamedValue(L"channel", JsonValue::CreateStringValue(L"app:launch"));
    message.SetNamedValue(L"args", args);

    auto line = stringify(message) + "\n";
    DWORD written = 0;
    bool isSent = WriteFile(pipe, line.data(), static_cast<DWORD>(line.size()), &written, nullptr) != FALSE;

    char buffer[4096];
    DWORD read = 0;
    if (isSent) ReadFile(pipe, buffer, sizeof(buffer), &read, nullptr);

    CloseHandle(pipe);
    return isSent;
}

}

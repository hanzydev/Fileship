#include "rpc.h"

#include "app.h"
#include "platform.h"

#include <shared/log.h>
#include <shared/platform.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>

namespace fileship::host {

namespace {

constexpr size_t BUFFER_SIZE = 64 * 1024;
constexpr size_t MAX_LINE = 256 * 1024 * 1024;

struct Client {
    uint64_t id = 0;
    std::shared_ptr<RpcChannel> channel;
    std::mutex writeMutex;
    std::atomic<bool> isOpen = true;
    bool isWindow = false;
    uint32_t pid = 0;
};

std::mutex handlersMutex;
std::map<std::wstring, RpcHandler> handlers;

std::mutex clientsMutex;
std::map<uint64_t, std::shared_ptr<Client>> clients;
std::vector<std::function<void(uint64_t)>> closeListeners;
std::atomic<uint64_t> nextClientId = 1;

thread_local uint64_t activeClient = 0;

void send(Client& client, JsonObject const& message) {
    std::lock_guard lock(client.writeMutex);
    if (!client.isOpen) return;

    if (!client.channel->write(stringify(message) + "\n")) client.isOpen = false;
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
    } catch (...) {
        response.SetNamedValue(L"ok", JsonValue::CreateBooleanValue(false));
        response.SetNamedValue(L"error", JsonValue::CreateStringValue(exceptionMessage()));
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
        client->channel->close();
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

JsonObject eventMessage(std::wstring const& event, IJsonValue const& payload) {
    JsonObject message;
    message.SetNamedValue(L"event", JsonValue::CreateStringValue(event));
    message.SetNamedValue(L"payload", payload ? payload : IJsonValue(JsonValue::CreateNullValue()));
    return message;
}

}

void serveRpcChannel(std::shared_ptr<RpcChannel> channel) {
    auto client = std::make_shared<Client>();
    client->id = nextClientId++;
    client->channel = std::move(channel);

    {
        std::lock_guard lock(clientsMutex);
        clients[client->id] = client;
    }

    std::vector<char> buffer(BUFFER_SIZE);
    std::string pending;

    while (client->isOpen) {
        size_t read = client->channel->read(buffer.data(), buffer.size());
        if (!read) break;

        pending.append(buffer.data(), read);
        if (pending.size() > MAX_LINE) break;

        size_t newline;
        while ((newline = pending.find('\n')) != std::string::npos) {
            handleLine(client, pending.substr(0, newline));
            pending.erase(0, newline + 1);
        }
    }

    closeClient(client);
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

    auto message = eventMessage(event, payload);
    for (auto const& client : targets) send(*client, message);
}

uint64_t currentClient() {
    return activeClient;
}

void sendTo(uint64_t id, std::wstring const& event, IJsonValue const& payload) {
    if (auto client = findClient(id)) send(*client, eventMessage(event, payload));
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

void handleWindowClient(uint64_t id, uint32_t pid) {
    auto client = findClient(id);
    if (!client) return;

    {
        std::lock_guard lock(clientsMutex);
        client->isWindow = true;
        client->pid = pid;
    }

    addWindowProcess(pid);
}

bool forwardToRunningHost(std::vector<std::wstring> const& arguments) {
    JsonArray argv;
    for (auto const& argument : arguments) argv.Append(JsonValue::CreateStringValue(argument));

    JsonArray args;
    args.Append(argv);
    args.Append(JsonValue::CreateStringValue(std::filesystem::current_path().wstring()));

    JsonObject message;
    message.SetNamedValue(L"id", JsonValue::CreateNumberValue(1));
    message.SetNamedValue(L"channel", JsonValue::CreateStringValue(L"app:launch"));
    message.SetNamedValue(L"args", args);

    return sendToRunningHost(stringify(message) + "\n");
}

}

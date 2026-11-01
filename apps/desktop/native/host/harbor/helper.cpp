#include "helper.h"

#include "support.h"

#include <host/platform.h>
#include <host/services.h>
#include <shared/i18n.h>
#include <shared/log.h>

#include <future>
#include <stdexcept>
#include <thread>

namespace fileship::host::harbor {

namespace {

HelperEvent parseEvent(JsonObject const& message) {
    HelperEvent event;
    event.type = stringField(message, L"type").value_or(L"");
    event.identity = stringField(message, L"identity").value_or(L"");
    event.path = stringField(message, L"path").value_or(L"");
    event.from = stringField(message, L"from").value_or(L"");
    event.to = stringField(message, L"to");
    event.isFolder = boolField(message, L"isFolder").value_or(false);
    event.size = static_cast<int64_t>(numberField(message, L"size").value_or(0));
    event.requestId = static_cast<uint64_t>(numberField(message, L"requestId").value_or(0));
    event.action = stringField(message, L"action").value_or(L"");

    if (message.HasKey(L"paths") && message.GetNamedValue(L"paths").ValueType() == JsonValueType::Array) {
        for (auto const& value : message.GetNamedArray(L"paths")) {
            if (value.ValueType() == JsonValueType::String) event.paths.emplace_back(value.GetString());
        }
    }

    return event;
}

std::vector<ScanItem> itemsOf(JsonObject const& result) {
    std::vector<ScanItem> items;

    for (auto const& value : result.GetNamedArray(L"items", JsonArray())) {
        auto object = value.GetObject();
        ScanItem item;
        item.path = stringField(object, L"path").value_or(L"");
        item.isFolder = boolField(object, L"isFolder").value_or(false);
        item.identity = stringField(object, L"identity");
        if (item.identity && item.identity->empty()) item.identity.reset();
        item.inSync = boolField(object, L"inSync").value_or(false);
        item.modified = boolField(object, L"modified").value_or(false);
        item.hydrated = boolField(object, L"hydrated").value_or(false);
        item.isUnreadable = boolField(object, L"unreadable").value_or(false);
        item.size = static_cast<int64_t>(numberField(object, L"size").value_or(0));
        items.push_back(std::move(item));
    }

    return items;
}

}

std::filesystem::path helperPath() {
    return resourcesPath().parent_path() / L"harbor" / executableName(L"fileship-harbor");
}

bool isHelperAvailable() {
    std::error_code error;
    return std::filesystem::exists(helperPath(), error);
}

std::filesystem::path harborSessionPath() {
    return localDataFolder() / L"Fileship" / L"harbor.json";
}

HarborHelper::~HarborHelper() = default;

void HarborHelper::start() {
    auto self = shared_from_this();
    ChildProcess::Handlers handlers;
    handlers.onError = [](std::string const& line) {
        if (!line.empty()) logWarn(L"[harbor] " + toWide(line));
    };
    handlers.onOutput = [self](std::string const& line) {
        if (!line.empty()) self->handleLine(line);
    };
    handlers.onOutputClosed = [self] { self->finish(); };

    auto child = std::make_unique<ChildProcess>(helperPath().wstring(), std::vector<std::wstring>{}, std::move(handlers));

    std::lock_guard lock(mutex);
    process = std::move(child);
    running = true;
}

bool HarborHelper::isRunning() const {
    return running;
}

void HarborHelper::write(std::string const& line) {
    std::lock_guard writing(writeMutex);
    std::lock_guard lock(mutex);
    if (process && running) process->write(line);
}

void HarborHelper::requestAsync(std::wstring const& type, JsonObject payload, Callback callback,
                                int timeoutMs) {
    if (!running) {
        callback(std::nullopt, t(L"harbor.helperNotRunning"));
        return;
    }

    uint64_t id = 0;

    {
        std::lock_guard lock(mutex);
        id = nextId++;
        pending[id] = Pending{std::move(callback), 0, type};
    }

    std::weak_ptr<HarborHelper> weak = weak_from_this();
    uint64_t timer = timers().set(timeoutMs, [weak, id] {
        auto self = weak.lock();
        if (!self) return;

        Pending entry;
        {
            std::lock_guard lock(self->mutex);
            auto found = self->pending.find(id);
            if (found == self->pending.end()) return;
            entry = std::move(found->second);
            self->pending.erase(found);
        }

        entry.callback(std::nullopt, t(L"harbor.helperNoAnswer", {{L"request", entry.type}}));
    });

    {
        std::lock_guard lock(mutex);
        auto found = pending.find(id);
        if (found != pending.end()) found->second.timer = timer;
    }

    payload.SetNamedValue(L"id", JsonValue::CreateNumberValue(static_cast<double>(id)));
    payload.SetNamedValue(L"type", JsonValue::CreateStringValue(type));
    write(stringify(payload) + "\n");
}

JsonObject HarborHelper::request(std::wstring const& type, JsonObject payload, int timeoutMs) {
    auto promise = std::make_shared<std::promise<JsonObject>>();
    auto future = promise->get_future();

    requestAsync(
        type, std::move(payload),
        [promise](std::optional<JsonObject> const& result, std::wstring const& error) {
            if (result) {
                promise->set_value(*result);
            } else {
                promise->set_exception(std::make_exception_ptr(std::runtime_error(toUtf8(error))));
            }
        },
        timeoutMs);

    return future.get();
}

std::vector<ScanItem> HarborHelper::scan() {
    return itemsOf(request(L"scan"));
}

PathIdentity HarborHelper::identityOf(std::wstring const& path) {
    JsonObject payload;
    payload.SetNamedValue(L"path", JsonValue::CreateStringValue(path));

    auto result = request(L"identity", payload);
    auto identity = stringField(result, L"identity");
    if (identity && identity->empty()) identity.reset();
    return {boolField(result, L"exists").value_or(false), identity};
}

std::vector<ScanItem> HarborHelper::adopt(std::wstring const& path, std::wstring const& identity,
                                          std::wstring const& stamp) {
    JsonObject payload;
    payload.SetNamedValue(L"path", JsonValue::CreateStringValue(path));
    payload.SetNamedValue(L"identity", JsonValue::CreateStringValue(identity));
    if (!stamp.empty()) payload.SetNamedValue(L"stamp", JsonValue::CreateStringValue(stamp));

    return itemsOf(request(L"adopt", payload));
}

bool HarborHelper::remove(std::wstring const& path) {
    JsonObject payload;
    payload.SetNamedValue(L"path", JsonValue::CreateStringValue(path));
    return boolField(request(L"remove", payload), L"kept").value_or(false);
}

void HarborHelper::ack(uint64_t requestId, bool ok) {
    if (!running) return;

    JsonObject message;
    message.SetNamedValue(L"type", JsonValue::CreateStringValue(L"ack"));
    message.SetNamedValue(L"requestId", JsonValue::CreateNumberValue(static_cast<double>(requestId)));
    message.SetNamedValue(L"ok", JsonValue::CreateBooleanValue(ok));
    write(stringify(message) + "\n");
}

void HarborHelper::stop() {
    if (!running) return;

    try {
        request(L"stop", JsonObject(), 5000);
    } catch (std::exception const&) {
    }

    std::lock_guard lock(mutex);
    if (process && !process->wait(2000)) process->kill();
    running = false;
}

void HarborHelper::handleLine(std::string const& line) {
    auto message = parseObject(line);

    if (!message) {
        logWarn(L"[harbor] invalid message " + toWide(line));
        return;
    }

    if (stringField(*message, L"type") != L"result") {
        if (onEvent) {
            try {
                onEvent(parseEvent(*message));
            } catch (std::exception const& error) {
                logWarn(L"[harbor] event failed: " + errorMessage(error));
            } catch (...) {
                logWarn(L"[harbor] event failed: " + describeException(std::current_exception()));
            }
        }
        return;
    }

    auto id = static_cast<uint64_t>(numberField(*message, L"id").value_or(0));
    Pending entry;

    {
        std::lock_guard lock(mutex);
        auto found = pending.find(id);
        if (found == pending.end()) return;
        entry = std::move(found->second);
        pending.erase(found);
    }

    timers().clear(entry.timer);

    if (boolField(*message, L"ok").value_or(false)) {
        entry.callback(*message, L"");
    } else {
        auto error = stringField(*message, L"error").value_or(L"");
        entry.callback(std::nullopt, error.empty() ? t(L"harbor.helperFailed") : error);
    }
}

void HarborHelper::finish() {
    std::optional<int> code;
    std::map<uint64_t, Pending> rejected;

    {
        std::lock_guard writing(writeMutex);
        std::lock_guard lock(mutex);

        if (process) {
            if (process->wait(5000)) code = process->exitCode();
            process->closeInput();
        }

        rejected.swap(pending);
        running = false;
    }

    for (auto& [id, entry] : rejected) {
        timers().clear(entry.timer);
        entry.callback(std::nullopt, t(L"harbor.helperStopped"));
    }

    if (onExit) onExit(code);
}

}

#include "helper.h"

#include "platform.h"
#include "support.h"

#include <shared/i18n.h>
#include <shared/log.h>
#include <shared/platform.h>
#include <shared/utf8.h>

#include <future>
#include <stdexcept>

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

bool isHelperAvailable() {
    std::error_code error;
    return std::filesystem::exists(helperPath(), error);
}

void HarborHelper::start() {
    process = HelperProcess::start(helperPath());
    running = true;

    auto self = shared_from_this();
    process->listen([self](std::string const& line) { self->handleLine(line); },
                    [](std::string const& line) { logWarn(L"[harbor] " + toWide(line)); },
                    [self] { self->finish(); });
}

bool HarborHelper::isRunning() const {
    return running;
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
    process->write(stringify(payload) + "\n");
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
    process->write(stringify(message) + "\n");
}

void HarborHelper::stop() {
    if (!running) return;

    try {
        request(L"stop", JsonObject(), 5000);
    } catch (std::exception const&) {
    }

    if (!process->wait(2000)) process->terminate();
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
            } catch (...) {
                logWarn(L"[harbor] event failed: " + exceptionMessage());
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
    auto code = process->wait(5000);
    std::map<uint64_t, Pending> rejected;

    process->closeInput();

    {
        std::lock_guard lock(mutex);
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

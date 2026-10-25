#include "helper.h"

#include "support.h"

#include <host/services.h>
#include <shared/i18n.h>
#include <shared/log.h>

#include <shlobj.h>

#include <future>
#include <stdexcept>
#include <thread>

#undef GetObject

namespace fileship::host::harbor {

namespace {

std::wstring localAppData() {
    PWSTR folder = nullptr;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &folder);
    std::wstring result(folder);
    CoTaskMemFree(folder);
    return result;
}

[[noreturn]] void throwLastError(char const* what) {
    throw std::runtime_error(std::string(what) + " failed with " + std::to_string(GetLastError()));
}

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
    return resourcesPath().parent_path() / L"harbor" / L"fileship-harbor.exe";
}

bool isHelperAvailable() {
    std::error_code error;
    return std::filesystem::exists(helperPath(), error);
}

std::filesystem::path harborSessionPath() {
    return std::filesystem::path(localAppData()) / L"Fileship" / L"harbor.json";
}

HarborHelper::~HarborHelper() {
    if (input) CloseHandle(input);
    if (process) CloseHandle(process);
}

void HarborHelper::start() {
    SECURITY_ATTRIBUTES inherit{sizeof(inherit), nullptr, TRUE};
    HANDLE inputRead = nullptr, inputWrite = nullptr;
    HANDLE outputRead = nullptr, outputWrite = nullptr;
    HANDLE errorRead = nullptr, errorWrite = nullptr;

    if (!CreatePipe(&inputRead, &inputWrite, &inherit, 0) ||
        !CreatePipe(&outputRead, &outputWrite, &inherit, 0) ||
        !CreatePipe(&errorRead, &errorWrite, &inherit, 0)) {
        throwLastError("CreatePipe");
    }

    SetHandleInformation(inputWrite, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(outputRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(errorRead, HANDLE_FLAG_INHERIT, 0);

    HANDLE inherited[] = {inputRead, outputWrite, errorWrite};
    SIZE_T size = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
    std::vector<char> attributes(size);
    auto list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
    InitializeProcThreadAttributeList(list, 1, 0, &size);
    UpdateProcThreadAttribute(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherited, sizeof(inherited),
                              nullptr, nullptr);

    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup.StartupInfo.wShowWindow = SW_HIDE;
    startup.StartupInfo.hStdInput = inputRead;
    startup.StartupInfo.hStdOutput = outputWrite;
    startup.StartupInfo.hStdError = errorWrite;
    startup.lpAttributeList = list;

    auto executable = helperPath().wstring();
    std::wstring command = L"\"" + executable + L"\"";
    PROCESS_INFORMATION info{};

    BOOL created = CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
                                  CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr,
                                  &startup.StartupInfo, &info);
    DWORD error = GetLastError();

    DeleteProcThreadAttributeList(list);
    CloseHandle(inputRead);
    CloseHandle(outputWrite);
    CloseHandle(errorWrite);

    if (!created) {
        CloseHandle(inputWrite);
        CloseHandle(outputRead);
        CloseHandle(errorRead);
        SetLastError(error);
        throwLastError("CreateProcess");
    }

    CloseHandle(info.hThread);

    {
        std::lock_guard lock(mutex);
        process = info.hProcess;
        input = inputWrite;
        running = true;
    }

    auto self = shared_from_this();
    std::thread([self, outputRead] { self->readOutput(outputRead); }).detach();
    std::thread([self, errorRead] { self->readErrors(errorRead); }).detach();
}

bool HarborHelper::isRunning() const {
    return running;
}

void HarborHelper::write(std::string const& line) {
    std::lock_guard writing(writeMutex);
    HANDLE handle = nullptr;

    {
        std::lock_guard lock(mutex);
        handle = input;
    }

    if (!handle) return;

    DWORD written = 0;
    WriteFile(handle, line.data(), static_cast<DWORD>(line.size()), &written, nullptr);
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

    HANDLE handle = nullptr;
    {
        std::lock_guard lock(mutex);
        handle = process;
    }

    if (handle && WaitForSingleObject(handle, 2000) == WAIT_TIMEOUT) TerminateProcess(handle, 1);
    running = false;
}

void HarborHelper::readOutput(HANDLE output) {
    std::string buffer;
    char chunk[65536];
    DWORD read = 0;

    while (ReadFile(output, chunk, sizeof(chunk), &read, nullptr) && read > 0) {
        buffer.append(chunk, read);

        size_t newline = 0;
        while ((newline = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, newline);
            buffer.erase(0, newline + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) handleLine(line);
        }
    }

    CloseHandle(output);
    finish();
}

void HarborHelper::readErrors(HANDLE errors) {
    std::string buffer;
    char chunk[4096];
    DWORD read = 0;

    while (ReadFile(errors, chunk, sizeof(chunk), &read, nullptr) && read > 0) {
        buffer.append(chunk, read);

        size_t newline = 0;
        while ((newline = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, newline);
            buffer.erase(0, newline + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) logWarn(L"[harbor] " + toWide(line));
        }
    }

    CloseHandle(errors);
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
            } catch (winrt::hresult_error const& error) {
                logWarn(L"[harbor] event failed: " + std::wstring(error.message()));
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
    std::optional<DWORD> code;
    std::map<uint64_t, Pending> rejected;

    {
        std::lock_guard writing(writeMutex);
        std::lock_guard lock(mutex);

        if (process) {
            WaitForSingleObject(process, 5000);
            DWORD exitCode = 0;
            if (GetExitCodeProcess(process, &exitCode) && exitCode != STILL_ACTIVE) code = exitCode;
        }

        if (input) {
            CloseHandle(input);
            input = nullptr;
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

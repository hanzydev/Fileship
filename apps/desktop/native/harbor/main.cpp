#include "com.h"
#include "status.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <thread>

using namespace winrt;

namespace harbor {

namespace {

Config currentConfig;
std::wstring currentToken;
std::mutex tokenMutex;
std::mutex outputMutex;

struct PendingAck {
    CF_TRANSFER_KEY transferKey;
    CF_REQUEST_KEY requestKey;
};

std::mutex pendingMutex;
std::map<uint64_t, PendingAck> pendingAcks;
std::atomic<uint64_t> nextRequestId{1};

void executeAck(PendingAck const& pending, bool ok) {
    CF_OPERATION_INFO info{};
    info.StructSize = sizeof(info);
    info.Type = CF_OPERATION_TYPE_ACK_RENAME;
    info.ConnectionKey = connectionKey();
    info.TransferKey = pending.transferKey;
    info.RequestKey = pending.requestKey;

    CF_OPERATION_PARAMETERS parameters{};
    parameters.ParamSize = CF_SIZE_OF_OP_PARAM(AckRename);
    parameters.AckRename.CompletionStatus = ok ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

    HRESULT result = CfExecute(&info, &parameters);
    if (FAILED(result)) log(L"CfExecute ack failed: " + std::to_wstring(result));
}

void acknowledgeLater(CF_CALLBACK_INFO const* info, JsonObject message) {
    uint64_t requestId = nextRequestId++;

    {
        std::lock_guard lock(pendingMutex);
        pendingAcks[requestId] = PendingAck{info->TransferKey, info->RequestKey};
    }

    message.SetNamedValue(L"requestId", JsonValue::CreateNumberValue(static_cast<double>(requestId)));
    send(message);

    std::thread([requestId] {
        std::this_thread::sleep_for(std::chrono::seconds(60));
        resolveAck(requestId, false);
    }).detach();
}

std::wstring trimLine(std::wstring line) {
    while (!line.empty() && (line.back() == L'\r' || line.back() == L'\n')) line.pop_back();
    return line;
}

JsonObject reply(JsonObject const& request, bool ok) {
    JsonObject message;
    message.SetNamedValue(L"type", JsonValue::CreateStringValue(L"result"));
    message.SetNamedValue(L"id", JsonValue::CreateNumberValue(request.GetNamedNumber(L"id", 0)));
    message.SetNamedValue(L"ok", JsonValue::CreateBooleanValue(ok));
    return message;
}

std::vector<PlaceholderItem> readItems(JsonObject const& request) {
    std::vector<PlaceholderItem> items;

    for (auto const& value : request.GetNamedArray(L"items")) {
        auto object = value.GetObject();
        items.push_back(PlaceholderItem{
            std::wstring(object.GetNamedString(L"identity")),
            std::wstring(object.GetNamedString(L"path")),
            object.GetNamedBoolean(L"isFolder", false),
            static_cast<int64_t>(object.GetNamedNumber(L"size", 0)),
            static_cast<int64_t>(object.GetNamedNumber(L"created", 0)),
        });
    }

    return items;
}

std::vector<std::wstring> readPaths(JsonObject const& request) {
    std::vector<std::wstring> paths;
    for (auto const& value : request.GetNamedArray(L"paths")) paths.emplace_back(value.GetString());
    return paths;
}

void hostShellHandlers() {
    try {
        setHosted();
        writeShellRegistration();
        saveSession();
        registerClassObjects();
    } catch (winrt::hresult_error const& error) {
        log(L"Could not host the shell handlers: " + std::wstring(error.message()));
    } catch (std::exception const& error) {
        log(L"Could not host the shell handlers: " + toWide(error.what()));
    }
}

void handle(JsonObject const& request) {
    std::wstring type(request.GetNamedString(L"type"));
    auto response = reply(request, true);

    try {
        if (type == L"init") {
            currentConfig.root = request.GetNamedString(L"root");
            currentConfig.syncRootId = request.GetNamedString(L"syncRootId");
            currentConfig.displayName = request.GetNamedString(L"displayName");
            currentConfig.iconPath = request.GetNamedString(L"iconPath");
            currentConfig.version = request.GetNamedString(L"version");
            currentConfig.instanceUrl = request.GetNamedString(L"instanceUrl");
            currentConfig.userAgent = request.GetNamedString(L"userAgent");
            currentConfig.appPath = request.GetNamedString(L"appPath", L"");
            currentConfig.labels = request.GetNamedObject(L"labels", JsonObject());
            currentConfig.properties.clear();

            for (auto const& value : request.GetNamedArray(L"properties", JsonArray())) {
                auto property = value.GetObject();
                currentConfig.properties.emplace_back(static_cast<int32_t>(property.GetNamedNumber(L"id")),
                                                      std::wstring(property.GetNamedString(L"name")));
            }
            setToken(std::wstring(request.GetNamedString(L"token")));

            std::filesystem::create_directories(currentConfig.root);
            registerSyncRoot();
            hostShellHandlers();
            connectSyncRoot();
            startWatcher();
        } else if (type == L"create") {
            auto items = readItems(request);
            createPlaceholders(items);
            for (auto const& item : items) settle(item.isFolder ? item.path : parentOf(item.path));
        } else if (type == L"move") {
            std::wstring from(request.GetNamedString(L"from"));
            std::wstring to(request.GetNamedString(L"to"));
            movePlaceholder(from, to);
            settle(parentOf(from));
            settleItem(to);
        } else if (type == L"update") {
            std::wstring path(request.GetNamedString(L"path"));
            std::optional<std::wstring> identity;
            if (request.HasKey(L"identity")) identity = request.GetNamedString(L"identity");
            updatePlaceholder(path, identity, true);
            settle(parentOf(path));
        } else if (type == L"remove") {
            std::wstring path(request.GetNamedString(L"path"));
            removePath(path);
            settle(parentOf(path));
        } else if (type == L"adopt") {
            std::wstring path(request.GetNamedString(L"path"));
            adoptPath(path, std::wstring(request.GetNamedString(L"identity")));
            settleItem(path);
        } else if (type == L"markInSync") {
            auto paths = readPaths(request);
            std::ranges::sort(paths, std::greater{}, depthOf);
            JsonArray marked;

            for (auto const& path : paths) {
                if (markInSync(path)) marked.Append(JsonValue::CreateStringValue(path));
                settleItem(path);
            }

            settle(L"");
            response.SetNamedValue(L"marked", marked);
        } else if (type == L"prepare") {
            auto identity = preparePath(std::wstring(request.GetNamedString(L"path")),
                                        std::wstring(request.GetNamedString(L"identity")));
            response.SetNamedValue(L"identity", JsonValue::CreateStringValue(identity));
        } else if (type == L"progress") {
            reportProgress(std::wstring(request.GetNamedString(L"path")),
                           static_cast<int64_t>(request.GetNamedNumber(L"total")),
                           static_cast<int64_t>(request.GetNamedNumber(L"completed")));
        } else if (type == L"setStates") {
            response.SetNamedValue(L"failed", setStates(request.GetNamedArray(L"items")));
        } else if (type == L"scan") {
            response.SetNamedValue(L"items", scanRoot());
        } else if (type == L"ack") {
            resolveAck(static_cast<uint64_t>(request.GetNamedNumber(L"requestId")),
                       request.GetNamedBoolean(L"ok"));
            return;
        } else if (type == L"syncStatus") {
            auto error = request.GetNamedValue(L"error", JsonValue::CreateNullValue());
            setHarborState(std::wstring(request.GetNamedString(L"state", L"")));
            reportSyncStatus(error.ValueType() == winrt::Windows::Data::Json::JsonValueType::String
                                 ? std::optional<std::wstring>(error.GetString())
                                 : std::nullopt);
        } else if (type == L"unregister") {
            revokeClassObjects();
            stopWatcher();
            disconnectSyncRoot();
            unregisterSyncRoot(syncRootId());
            removeShellRegistration();
            removeSession();
        } else if (type == L"stop") {
            revokeClassObjects();
            stopWatcher();
            disconnectSyncRoot();
            send(response);
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown command");
        }
    } catch (winrt::hresult_error const& error) {
        response = reply(request, false);
        response.SetNamedValue(L"error", JsonValue::CreateStringValue(error.message()));
    } catch (std::exception const& error) {
        response = reply(request, false);
        response.SetNamedValue(L"error", JsonValue::CreateStringValue(toWide(error.what())));
    }

    if (request.HasKey(L"id")) send(response);
}

}

Config& config() {
    return currentConfig;
}

std::wstring token() {
    std::lock_guard lock(tokenMutex);
    return currentToken;
}

void setToken(std::wstring const& value) {
    std::lock_guard lock(tokenMutex);
    currentToken = value;
}

std::wstring label(std::wstring const& key) {
    return std::wstring(currentConfig.labels.GetNamedString(key, key));
}

void send(JsonObject const& message) {
    auto line = toUtf8(std::wstring(message.Stringify())) + "\n";
    std::lock_guard lock(outputMutex);
    fwrite(line.data(), 1, line.size(), stdout);
    fflush(stdout);
}

void sendEvent(std::wstring const& type, JsonObject message) {
    message.SetNamedValue(L"type", JsonValue::CreateStringValue(type));
    send(message);
}

void log(std::wstring const& text) {
    auto line = toUtf8(text) + "\n";
    std::lock_guard lock(outputMutex);
    fwrite(line.data(), 1, line.size(), stderr);
    fflush(stderr);
}

std::wstring toWide(std::string const& text) {
    if (text.empty()) return {};
    int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring result(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length);
    return result;
}

std::string toUtf8(std::wstring const& text) {
    if (text.empty()) return {};
    int length = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    std::string result(length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length, nullptr, nullptr);
    return result;
}

std::wstring fullPath(std::wstring const& relative) {
    return (std::filesystem::path(currentConfig.root) / relative).wstring();
}

std::wstring parentOf(std::wstring const& relative) {
    return std::filesystem::path(relative).parent_path().wstring();
}

int depthOf(std::wstring const& relative) {
    return relative.empty() ? -1 : static_cast<int>(std::ranges::count(relative, L'\\'));
}

std::optional<std::wstring> relativePath(std::wstring const& absolute) {
    auto root = std::filesystem::path(currentConfig.root).lexically_normal().wstring();
    auto path = std::filesystem::path(absolute).lexically_normal().wstring();

    if (path.size() <= root.size() || _wcsnicmp(path.c_str(), root.c_str(), root.size()) != 0) {
        return std::nullopt;
    }

    auto rest = path.substr(root.size());
    while (!rest.empty() && (rest.front() == L'\\' || rest.front() == L'/')) rest.erase(rest.begin());
    return rest;
}

std::wstring identityOf(CF_CALLBACK_INFO const* info) {
    if (!info->FileIdentity || info->FileIdentityLength == 0) return {};
    return std::wstring(static_cast<wchar_t const*>(info->FileIdentity),
                        info->FileIdentityLength / sizeof(wchar_t));
}

bool isOwnProcess(CF_CALLBACK_INFO const* info) {
    return info->ProcessInfo && info->ProcessInfo->ProcessId == GetCurrentProcessId();
}

bool isIgnoredName(std::wstring const& name) {
    std::wstring lower(name);
    for (auto& c : lower) c = static_cast<wchar_t>(towlower(c));

    if (lower.starts_with(L"~$") || lower.starts_with(L".~lock")) return true;
    if (lower.ends_with(L".tmp") || lower.ends_with(L".crdownload") || lower.ends_with(L".part")) return true;
    return lower == L"desktop.ini" || lower == L"thumbs.db";
}

void reportDeleted(CF_CALLBACK_INFO const* info) {
    if (isOwnProcess(info)) return;

    std::wstring identity = identityOf(info);
    if (identity.empty()) return;

    std::wstring absolute = std::wstring(info->VolumeDosName) + info->NormalizedPath;
    if (placeholderIdentity(absolute, nullptr, nullptr) == identity) return;

    JsonObject message;
    message.SetNamedValue(L"type", JsonValue::CreateStringValue(L"delete"));
    message.SetNamedValue(L"identity", JsonValue::CreateStringValue(identity));
    message.SetNamedValue(L"path", JsonValue::CreateStringValue(relativePath(absolute).value_or(L"")));
    message.SetNamedValue(L"isFolder", JsonValue::CreateBooleanValue(identity.starts_with(L"d:")));
    send(message);
}

void askRename(CF_CALLBACK_INFO const* info, CF_CALLBACK_PARAMETERS const* parameters) {
    if (isOwnProcess(info)) {
        executeAck(PendingAck{info->TransferKey, info->RequestKey}, true);
        return;
    }

    std::wstring source = std::wstring(info->VolumeDosName) + info->NormalizedPath;
    std::wstring target = std::wstring(info->VolumeDosName) + parameters->Rename.TargetPath;
    bool isTargetInScope = (parameters->Rename.Flags & CF_CALLBACK_RENAME_FLAG_TARGET_IN_SCOPE) != 0;

    JsonObject message;
    message.SetNamedValue(L"type", JsonValue::CreateStringValue(L"rename"));
    message.SetNamedValue(L"identity", JsonValue::CreateStringValue(identityOf(info)));
    message.SetNamedValue(L"from", JsonValue::CreateStringValue(relativePath(source).value_or(L"")));
    message.SetNamedValue(L"to", isTargetInScope
                                     ? JsonValue::CreateStringValue(relativePath(target).value_or(L""))
                                     : JsonValue::CreateNullValue());
    message.SetNamedValue(L"isFolder", JsonValue::CreateBooleanValue(
        (parameters->Rename.Flags & CF_CALLBACK_RENAME_FLAG_IS_DIRECTORY) != 0));

    acknowledgeLater(info, message);
}

void renamed(CF_CALLBACK_INFO const* info, CF_CALLBACK_PARAMETERS const* parameters) {
    std::wstring volume(info->VolumeDosName);
    auto target = relativePath(volume + info->NormalizedPath);
    auto source = relativePath(volume + parameters->RenameCompletion.SourcePath);

    std::thread([target, source] {
        if (target) {
            markInSync(*target);
            settleItem(*target);
        }

        if (source) settle(parentOf(*source));
    }).detach();
}

void resolveAck(uint64_t requestId, bool ok) {
    std::optional<PendingAck> pending;

    {
        std::lock_guard lock(pendingMutex);
        auto found = pendingAcks.find(requestId);
        if (found == pendingAcks.end()) return;
        pending = found->second;
        pendingAcks.erase(found);
    }

    executeAck(*pending, ok);
}

}

int wmain(int argc, wchar_t** argv) {
    winrt::init_apartment();
    SetConsoleOutputCP(CP_UTF8);

    std::wstring command = argc > 1 ? argv[1] : L"";

    if (command == L"--unregister-all") {
        harbor::unregisterAll();
        harbor::removeShellRegistration();
        harbor::removeSession();
        return 0;
    }

    if (_wcsicmp(command.c_str(), L"-Embedding") == 0 || _wcsicmp(command.c_str(), L"/Embedding") == 0) {
        harbor::loadSession();
        harbor::registerClassObjects();
        harbor::waitUntilIdle();
        harbor::revokeClassObjects();
        return 0;
    }

    std::string line;

    while (std::getline(std::cin, line)) {
        auto text = harbor::trimLine(harbor::toWide(line));
        if (text.empty()) continue;

        harbor::JsonObject request;

        if (!harbor::JsonObject::TryParse(text, request)) {
            harbor::log(L"Invalid message: " + text);
            continue;
        }

        std::thread([request] { harbor::handle(request); }).detach();
    }

    harbor::revokeClassObjects();
    harbor::stopWatcher();
    harbor::disconnectSyncRoot();
    return 0;
}

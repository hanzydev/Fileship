#include "app.h"
#include "rpc.h"

#include <host/harbor.h>
#include <host/recording.h>
#include <shared/log.h>

#include <mutex>
#include <set>

namespace fileship::host {

namespace {

constexpr int FOLDER_PAGE = 200;

std::mutex pauseMutex;
std::set<uint64_t> pausingClients;

JsonValue none() {
    return JsonValue::CreateNullValue();
}

std::wstring stringArg(JsonArray const& args, uint32_t index) {
    if (index >= args.Size() || args.GetAt(index).ValueType() != JsonValueType::String) return {};
    return std::wstring(args.GetStringAt(index));
}

double numberArg(JsonArray const& args, uint32_t index) {
    if (index >= args.Size() || args.GetAt(index).ValueType() != JsonValueType::Number) return 0;
    return args.GetNumberAt(index);
}

bool boolArg(JsonArray const& args, uint32_t index) {
    return index < args.Size() && args.GetAt(index).ValueType() == JsonValueType::Boolean && args.GetBooleanAt(index);
}

JsonObject objectArg(JsonArray const& args, uint32_t index) {
    if (index >= args.Size() || args.GetAt(index).ValueType() != JsonValueType::Object) return JsonObject();
    return args.GetObjectAt(index);
}

std::vector<std::wstring> stringsArg(JsonArray const& args, uint32_t index) {
    std::vector<std::wstring> result;
    if (index >= args.Size() || args.GetAt(index).ValueType() != JsonValueType::Array) return result;

    for (auto const& item : args.GetArrayAt(index)) {
        if (item.ValueType() == JsonValueType::String) result.emplace_back(item.GetString());
    }

    return result;
}

IJsonValue historyUrl(std::wstring const& id) {
    auto item = getHistory(id);
    if (!item) return none();

    auto shortUrl = jsonString(*item, L"shortUrl");
    auto url = shortUrl.empty() ? jsonString(*item, L"url") : shortUrl;
    return stringOrNull(url);
}

IJsonValue listFolders() {
    struct Folder {
        std::wstring id;
        std::wstring name;
        std::wstring parentId;
    };

    std::vector<Folder> folders;

    for (int offset = 0;; offset += FOLDER_PAGE) {
        auto page = api(L"GET", L"/api/folders?limit=" + std::to_wstring(FOLDER_PAGE) + L"&offset=" +
                                    std::to_wstring(offset) + L"&sort=name:asc")
                        .GetObject();
        auto items = page.GetNamedArray(L"items");

        for (auto const& item : items) {
            auto folder = item.GetObject();
            folders.push_back({jsonString(folder, L"id"), jsonString(folder, L"name"), jsonString(folder, L"parentId")});
        }

        if (items.Size() < static_cast<uint32_t>(FOLDER_PAGE) ||
            folders.size() >= static_cast<size_t>(jsonNumber(page, L"total")))
            break;
    }

    std::map<std::wstring, Folder const*> byId;
    for (auto const& folder : folders) byId[folder.id] = &folder;

    std::function<std::wstring(Folder const&, size_t)> pathOf = [&](Folder const& folder, size_t depth) {
        auto parent = folder.parentId.empty() || depth >= folders.size() ? byId.end() : byId.find(folder.parentId);
        return parent == byId.end() ? folder.name : pathOf(*parent->second, depth + 1) + L" / " + folder.name;
    };

    std::vector<std::pair<std::wstring, std::wstring>> named;
    for (auto const& folder : folders) named.emplace_back(folder.id, pathOf(folder, 0));

    auto locale = language();
    std::sort(named.begin(), named.end(), [&](auto const& a, auto const& b) {
        return CompareStringEx(locale.c_str(), LINGUISTIC_IGNORECASE, a.second.c_str(), -1, b.second.c_str(), -1,
                               nullptr, nullptr, 0) == CSTR_LESS_THAN;
    });

    JsonArray result;
    for (auto const& [id, name] : named) {
        JsonObject folder;
        folder.SetNamedValue(L"id", JsonValue::CreateStringValue(id));
        folder.SetNamedValue(L"name", JsonValue::CreateStringValue(name));
        result.Append(folder);
    }

    return result;
}

IJsonValue setSettings(JsonObject const& patch) {
    auto previous = settings();
    auto next = updateSettings(patch);

    bool wasEnabled = jsonBool(jsonObject(previous, L"harbor"), L"enabled", true);
    bool isEnabled = jsonBool(jsonObject(next, L"harbor"), L"enabled", true);

    if (wasEnabled != isEnabled) {
        if (isEnabled) harbor::startHarbor();
        else harbor::stopHarbor({true, false});
    }

    if (jsonString(jsonObject(next, L"recording"), L"ffmpegPath") !=
        jsonString(jsonObject(previous, L"recording"), L"ffmpegPath"))
        recording::resetFfmpegProbe();

    applySettings();
    if (patch.HasKey(L"hotkeys") || patch.HasKey(L"hotkeysEnabled")) registerHotkeys();
    return next;
}

}

void applySettings() {
    auto current = settings();
    applyStartAtLogin(jsonBool(current, L"startAtLogin", true));
    applyMenuTheme();

    if (setLanguage(resolveLanguage(jsonString(current, L"language", L"system")))) {
        refreshTray();
        registerShellMenu();
        broadcast(L"app:language", JsonValue::CreateStringValue(language()));
    }
}

void registerHandlers() {
    registerHandler(L"client:hello", [](JsonArray const& args) -> IJsonValue {
        auto client = currentClient();
        handleWindowClient(client, static_cast<DWORD>(jsonNumber(objectArg(args, 0), L"pid")));
        flushWindowEvents(client);

        JsonObject result;
        result.SetNamedValue(L"version", JsonValue::CreateStringValue(appVersion()));
        result.SetNamedValue(L"isPackaged", JsonValue::CreateBooleanValue(isPackaged()));
        return result;
    });

    registerHandler(L"app:launch", [](JsonArray const& args) -> IJsonValue {
        auto arguments = stringsArg(args, 0);
        auto cwd = stringArg(args, 1);
        bool isHidden = std::find(arguments.begin(), arguments.end(), L"--hidden") != arguments.end();

        if (!handleLaunch(arguments, cwd.empty() ? std::filesystem::current_path() : std::filesystem::path(cwd)) &&
            !isHidden)
            showMainWindow();

        return none();
    });

    registerHandler(L"instance:check", [](JsonArray const& args) -> IJsonValue { return checkInstance(stringArg(args, 0)); });

    registerHandler(L"account:get", [](JsonArray const&) -> IJsonValue {
        auto account = getAccount();
        refreshTray();
        return account;
    });

    registerHandler(L"account:setSession", [](JsonArray const& args) -> IJsonValue {
        auto session = objectArg(args, 0);
        return startSession(jsonString(session, L"instanceUrl"), jsonString(session, L"token"));
    });

    registerHandler(L"account:clearSession", [](JsonArray const&) -> IJsonValue {
        endSession();
        return none();
    });

    registerHandler(L"account:token", [](JsonArray const&) -> IJsonValue { return stringOrNull(token()); });

    registerHandler(L"settings:get", [](JsonArray const&) -> IJsonValue { return settings(); });
    registerHandler(L"settings:set", [](JsonArray const& args) -> IJsonValue { return setSettings(objectArg(args, 0)); });

    registerHandler(L"app:info", [](JsonArray const&) -> IJsonValue {
        JsonObject info;
        info.SetNamedValue(L"version", JsonValue::CreateStringValue(appVersion()));
        info.SetNamedValue(L"isPackaged", JsonValue::CreateBooleanValue(isPackaged()));
        return info;
    });

    registerHandler(L"app:quit", [](JsonArray const&) -> IJsonValue {
        postToMainThread(quitApp);
        return none();
    });

    registerHandler(L"app:language", [](JsonArray const&) -> IJsonValue { return JsonValue::CreateStringValue(language()); });

    registerHandler(L"updates:get", [](JsonArray const&) -> IJsonValue { return updateStatus(); });
    registerHandler(L"updates:check", [](JsonArray const&) -> IJsonValue {
        runAsync(checkForUpdates);
        return none();
    });
    registerHandler(L"updates:install", [](JsonArray const&) -> IJsonValue {
        installUpdate();
        return none();
    });

    registerHandler(L"capture:run", [](JsonArray const& args) -> IJsonValue {
        runAction(stringArg(args, 0));
        return none();
    });

    registerHandler(L"history:list", [](JsonArray const& args) -> IJsonValue {
        return listHistory(static_cast<int64_t>(numberArg(args, 0)), static_cast<int64_t>(numberArg(args, 1)));
    });

    registerHandler(L"history:copy", [](JsonArray const& args) -> IJsonValue {
        auto url = historyUrl(stringArg(args, 0));
        if (url.ValueType() == JsonValueType::String) writeClipboardText(std::wstring(url.GetString()));
        return none();
    });

    registerHandler(L"history:open", [](JsonArray const& args) -> IJsonValue {
        auto url = historyUrl(stringArg(args, 0));
        if (url.ValueType() == JsonValueType::String) openWebUrl(std::wstring(url.GetString()));
        return none();
    });

    registerHandler(L"history:showFile", [](JsonArray const& args) -> IJsonValue {
        auto item = getHistory(stringArg(args, 0));
        auto path = item ? jsonString(*item, L"localPath") : L"";
        if (!path.empty()) showItemInFolder(path);
        return none();
    });

    registerHandler(L"history:showInHarbor", [](JsonArray const& args) -> IJsonValue {
        auto item = getHistory(stringArg(args, 0));
        auto fileId = item ? jsonString(*item, L"fileId") : L"";
        auto path = fileId.empty() ? std::nullopt : harbor::harborPathFor(fileId);

        if (path) showItemInFolder(*path);
        else invokeHandler(L"harbor:open", JsonArray());
        return none();
    });

    registerHandler(L"history:remove", [](JsonArray const& args) -> IJsonValue {
        removeHistory(stringArg(args, 0));
        return none();
    });

    registerHandler(L"history:clear", [](JsonArray const&) -> IJsonValue {
        clearHistory();
        return none();
    });

    registerHandler(L"uploads:paths", [](JsonArray const& args) -> IJsonValue {
        auto values = stringsArg(args, 0);
        std::vector<std::filesystem::path> paths(values.begin(), values.end());
        runAsync([paths] { uploadPaths(paths); });
        return none();
    });

    registerHandler(L"uploads:progress", [](JsonArray const&) -> IJsonValue { return uploadProgress(); });

    registerHandler(L"hotkeys:status", [](JsonArray const&) -> IJsonValue { return hotkeyStatus(); });

    registerHandler(L"hotkeys:pause", [](JsonArray const& args) -> IJsonValue {
        auto client = currentClient();

        {
            std::lock_guard lock(pauseMutex);
            if (boolArg(args, 0)) pausingClients.insert(client);
            else pausingClients.erase(client);
        }

        if (!boolArg(args, 0)) return registerHotkeys();

        pauseHotkeys();
        return hotkeyStatus();
    });

    registerHandler(L"hotkeys:snippingTool", [](JsonArray const&) -> IJsonValue {
        return JsonValue::CreateBooleanValue(isSnippingToolKeyEnabled());
    });

    registerHandler(L"hotkeys:setSnippingTool", [](JsonArray const& args) -> IJsonValue { return setSnippingToolKey(boolArg(args, 0)); });

    registerHandler(L"folders:list", [](JsonArray const&) -> IJsonValue { return listFolders(); });

    onClientClosed([](uint64_t client) {
        bool wasPausing = false;

        {
            std::lock_guard lock(pauseMutex);
            wasPausing = pausingClients.erase(client) > 0;
        }

        if (wasPausing) registerHotkeys();
    });
}

}

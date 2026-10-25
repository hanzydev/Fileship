#include "engine.h"

#include "helper.h"
#include "paths.h"
#include "support.h"

#include <host/rpc.h>
#include <host/services.h>
#include <shared/i18n.h>
#include <shared/log.h>
#include <shared/socketio.h>

#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>

#include <algorithm>
#include <array>
#include <deque>
#include <fstream>
#include <future>
#include <set>
#include <sstream>

#pragma comment(lib, "comctl32.lib")

#undef GetObject

namespace fileship::host::harbor {

namespace {

using Lock = std::unique_lock<std::recursive_mutex>;

constexpr int PAGE_SIZE = 200;
constexpr int APPLY_DELAY_MS = 300;
constexpr int RETRY_DELAY_MS = 2000;
constexpr int REPLACE_WINDOW_MS = 3000;
constexpr std::array<int, 3> RETRY_DELAYS_MS = {5000, 15000, 60000};
constexpr int FINISH_ATTEMPTS = 3;
constexpr size_t STATES_BATCH = 100;
constexpr int STORAGE_DELAY_MS = 2000;
constexpr int64_t DETAILS_INTERVAL_MS = 10000;
constexpr int COPY_DELAY_MS = 2000;
constexpr double NEARLY_FULL = 0.9;
constexpr size_t ACTIVITY_LIMIT = 50;
constexpr int PROPERTY_LOCKED = 1;
constexpr int PROPERTY_EXPIRES = 2;
constexpr int PROPERTY_SENSITIVE = 3;
constexpr int PROPERTY_VIEWS = 4;

struct Storage {
    int64_t used = 0;
    std::optional<int64_t> limit;

    bool operator==(Storage const&) const = default;
};

struct Status {
    std::wstring state = L"off";
    std::wstring root;
    size_t files = 0;
    size_t folders = 0;
    std::optional<std::wstring> error;
    std::optional<Storage> storage;
    std::optional<int64_t> localSize;
    std::optional<int64_t> lastSyncedAt;
};

struct LocalItem {
    std::wstring path;
    bool isFolder = false;
};

struct Activity {
    std::wstring id;
    std::wstring kind;
    std::wstring name;
    std::optional<std::wstring> detail;
    int64_t at = 0;
};

struct Failure {
    std::wstring message;
    DWORD status = 0;
    bool quotaExceeded = false;
};

struct Links {
    std::wstring copy;
    std::wstring open;
};

std::recursive_mutex guard;
std::shared_ptr<HarborHelper> helper;
std::shared_ptr<SocketIo> socket;
std::wstring root;
Status status;
bool isPaused = false;
bool isStorageFull = false;
std::string lastReported;
int64_t detailsAt = 0;
bool hasStorageApi = true;
uint64_t storageTimer = 0;
uint64_t copyTimer = 0;
uint64_t applyTimer = 0;
uint64_t retryTimer = 0;
bool isStopping = false;
bool isSynced = false;
int retries = 0;

std::map<std::wstring, RemoteFolder> folders;
std::map<std::wstring, RemoteFile> files;
std::map<std::wstring, RemoteNote> notes;
std::map<std::wstring, RemoteLink> links;
std::map<std::wstring, std::string> localHashes;
std::set<std::wstring> replacedNotes;
InsertionMap<LocalItem> local;
std::set<std::wstring> pendingPaths;
std::set<std::wstring> hiddenFiles;
std::set<std::wstring> cancelledUploads;
std::map<std::wstring, size_t> uploadRetries;
std::map<std::wstring, std::string> sentStates;
std::deque<Activity> activity;
std::vector<std::wstring> linksToCopy;

SerialQueue& queue() {
    static auto* instance = new SerialQueue(L"Harbor");
    return *instance;
}

SerialQueue& lifecycle() {
    static auto* instance = new SerialQueue(L"Harbor lifecycle");
    return *instance;
}

SerialQueue& statesQueue() {
    static auto* instance = new SerialQueue(L"Harbor states");
    return *instance;
}

SerialQueue& outbox() {
    static auto* instance = new SerialQueue(L"Harbor outbox");
    return *instance;
}

void handleHelperEvent(HelperEvent event);
void scheduleRetry(std::wstring const& error);
void resync();
void runAction(std::wstring const& action, std::vector<std::wstring> const& paths);

JsonValue str(std::wstring const& value) {
    return JsonValue::CreateStringValue(value);
}

JsonValue num(double value) {
    return JsonValue::CreateNumberValue(value);
}

JsonValue boolean(bool value) {
    return JsonValue::CreateBooleanValue(value);
}

JsonValue nul() {
    return JsonValue::CreateNullValue();
}

JsonValue optionalString(std::optional<std::wstring> const& value) {
    return value ? str(*value) : nul();
}

JsonValue toValue(JsonObject const& object) {
    return JsonValue::Parse(object.Stringify());
}

Failure describe(std::exception_ptr error) {
    try {
        std::rethrow_exception(error);
    } catch (ApiError const& failure) {
        bool quota = failure.data && boolField(failure.data, L"quotaExceeded").value_or(false);
        return {toWide(failure.what()), failure.status, quota};
    } catch (std::exception const& failure) {
        return {toWide(failure.what())};
    } catch (winrt::hresult_error const& failure) {
        return {std::wstring(failure.message())};
    } catch (...) {
        return {L"Unknown error"};
    }
}

bool isUnauthorized(std::exception_ptr error) {
    return describe(error).status == 401;
}

bool isMissing(std::exception_ptr error) {
    return describe(error).status == 404;
}

template <typename Function>
void tryLog(std::wstring const& what, Function&& function) {
    try {
        function();
    } catch (...) {
        logWarn(what + L": " + describe(std::current_exception()).message);
    }
}

void runDetached(std::function<void()> work) {
    std::thread([work = std::move(work)] {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        tryLog(L"Harbor work failed", work);
    }).detach();
}

bool isPendingIdentity(std::wstring const& identity) {
    return startsWith(identity, L"p:");
}

std::wstring idOf(std::wstring const& identity) {
    return identity.substr(2);
}

RemoteLink const* linkByCode(std::wstring const& code) {
    for (auto const& [id, link] : links) {
        if (link.code == code) return &link;
    }

    return nullptr;
}

bool isKnown(std::wstring const& identity) {
    auto id = idOf(identity);

    if (startsWith(identity, L"f:")) return files.contains(id);
    if (startsWith(identity, L"n:")) return notes.contains(id);
    if (startsWith(identity, L"l:")) return linkByCode(id) != nullptr;
    if (startsWith(identity, L"v:")) return true;
    return folders.contains(id);
}

bool can(std::wstring const& permission) {
    auto user = currentUser();
    if (!user) return false;
    if (boolField(*user, L"superAdmin").value_or(false)) return true;
    if (!user->HasKey(L"permissions")) return false;

    for (auto const& value : user->GetNamedArray(L"permissions", JsonArray())) {
        if (value.ValueType() != JsonValueType::String) continue;

        std::wstring name(value.GetString());
        if (name == L"Admin" || name == permission) return true;
    }

    return false;
}

std::string readFileText(std::filesystem::path const& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Could not read " + toUtf8(path.wstring()));

    std::stringstream content;
    content << file.rdbuf();
    return toUtf8(toWide(content.str()));
}

bool pathExists(std::filesystem::path const& path) {
    std::error_code error;
    return std::filesystem::exists(path, error);
}

bool isEmptyFolder(std::filesystem::path const& path) {
    std::error_code error;
    auto iterator = std::filesystem::directory_iterator(path, error);
    if (error) return false;
    return iterator == std::filesystem::directory_iterator();
}

std::filesystem::path homeFolder() {
    PWSTR folder = nullptr;
    SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &folder);
    std::filesystem::path result(folder);
    CoTaskMemFree(folder);
    return result;
}

JsonObject harborSettings() {
    auto current = settings();
    if (current && current.HasKey(L"harbor") &&
        current.GetNamedValue(L"harbor").ValueType() == JsonValueType::Object) {
        return current.GetNamedObject(L"harbor");
    }

    return JsonObject();
}

std::filesystem::path executablePath() {
    std::wstring path(MAX_PATH, L'\0');

    while (true) {
        DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (length < path.size()) {
            path.resize(length);
            return path;
        }
        path.resize(path.size() * 2);
    }
}

std::wstring storeString(std::wstring const& key) {
    auto value = storeValue(key);
    return value && value.ValueType() == JsonValueType::String ? std::wstring(value.GetString()) : L"";
}

std::wstring userId() {
    auto user = currentUser();
    return user ? stringField(*user, L"id").value_or(L"") : L"";
}

bool isNearlyFull() {
    auto const& storage = status.storage;
    return storage && storage->limit && static_cast<double>(storage->used) >= *storage->limit * NEARLY_FULL;
}

struct Quota {
    int64_t used = 0;
    int64_t total = 0;
    std::wstring label;
    bool isNearlyFull = false;
};

std::optional<Quota> quotaOf() {
    if (!status.storage) return std::nullopt;

    auto [used, limit] = *status.storage;
    auto label = limit ? t(L"harbor.quotaUsed", {{L"used", formatSize(used)}, {L"total", formatSize(*limit)}})
                       : t(L"harbor.quotaUnlimited", {{L"used", formatSize(used)}});

    return Quota{used, limit.value_or(0), label, isNearlyFull()};
}

std::wstring countText(std::wstring const& key, size_t count) {
    auto value = std::to_wstring(count);
    return t(key, {{L"n", value}, {L"count", value}});
}

std::wstring summaryOf() {
    auto content = t(L"harborSettings.contentSummary",
                     {{L"files", countText(L"harborSettings.fileCount", files.size())},
                      {L"folders", countText(L"harborSettings.folderCount", folders.size())}});

    return t(L"harbor.summary", {{L"content", content},
                                 {L"local", formatSize(status.localSize.value_or(0))},
                                 {L"time", relativeTime(*status.lastSyncedAt, nowMs(), t(L"home.justNow"))}});
}

std::pair<std::wstring, std::wstring> moreInfoOf() {
    if (status.state == L"error") {
        return {status.error.value_or(L""), isStorageFull ? L"storage" : L"retry"};
    }

    if (status.state == L"paused") return {t(L"harbor.pausedMessage"), L"resume"};
    if (!pendingPaths.empty()) {
        return {t(L"harbor.uploading", {{L"count", std::to_wstring(pendingPaths.size())}}), L""};
    }

    if (isNearlyFull()) return {t(L"harbor.almostFull"), L"storage"};
    if (status.state != L"ready" || !status.lastSyncedAt) return {L"", L""};

    return {summaryOf(), status.localSize.value_or(0) ? L"freeUpSpace" : L""};
}

void reportSyncStatus() {
    if (!helper || !helper->isRunning()) return;

    auto [message, action] = moreInfoOf();
    auto quota = quotaOf();

    JsonObject state;
    state.SetNamedValue(L"state", str(status.state));
    state.SetNamedValue(L"message", str(message));
    state.SetNamedValue(L"action", str(action));

    if (quota) {
        JsonObject value;
        value.SetNamedValue(L"used", num(static_cast<double>(quota->used)));
        value.SetNamedValue(L"total", num(static_cast<double>(quota->total)));
        value.SetNamedValue(L"label", str(quota->label));
        value.SetNamedValue(L"isNearlyFull", boolean(quota->isNearlyFull));
        state.SetNamedValue(L"quota", value);
    } else {
        state.SetNamedValue(L"quota", nul());
    }

    JsonObject payload;
    payload.SetNamedValue(L"error", optionalString(status.error));
    payload.SetNamedValue(L"status", state);

    auto key = stringify(payload);
    if (key == lastReported) return;

    lastReported = key;
    helper->requestAsync(L"syncStatus", payload, [](std::optional<JsonObject> const& result, std::wstring const& error) {
        if (result) return;

        Lock lock(guard);
        lastReported.clear();
        logWarn(L"Could not report the Harbor status: " + error);
    });
}

JsonObject statusJson() {
    JsonObject object;
    object.SetNamedValue(L"state", str(status.state));
    object.SetNamedValue(L"root", str(status.root));
    object.SetNamedValue(L"files", num(static_cast<double>(status.files)));
    object.SetNamedValue(L"folders", num(static_cast<double>(status.folders)));
    object.SetNamedValue(L"error", optionalString(status.error));

    if (status.storage) {
        JsonObject storage;
        storage.SetNamedValue(L"used", num(static_cast<double>(status.storage->used)));
        storage.SetNamedValue(L"limit", status.storage->limit
                                            ? num(static_cast<double>(*status.storage->limit))
                                            : nul());
        object.SetNamedValue(L"storage", storage);
    } else {
        object.SetNamedValue(L"storage", nul());
    }

    object.SetNamedValue(L"localSize", status.localSize ? num(static_cast<double>(*status.localSize)) : nul());
    object.SetNamedValue(L"lastSyncedAt",
                         status.lastSyncedAt ? num(static_cast<double>(*status.lastSyncedAt)) : nul());
    return object;
}

void setStatus(std::function<void(Status&)> const& change = {}) {
    Lock lock(guard);
    auto previous = status;

    if (change) change(status);
    status.files = files.size();
    status.folders = folders.size();

    bool isChanged = status.state != previous.state || status.storage != previous.storage;
    auto payload = stringify(statusJson());

    outbox().post([payload, isChanged] {
        broadcast(L"harbor:status", JsonValue::Parse(toWide(payload)));
        if (isChanged) refreshTray();
    });

    reportSyncStatus();
}

void setError(std::exception_ptr error) {
    auto failure = describe(error);
    Lock lock(guard);

    if (failure.quotaExceeded && !isStorageFull) {
        auto message = failure.message;
        outbox().post([message] {
            notify(t(L"tasks.storageFull"), message, [] { openDashboard(L"/dashboard/settings/storage"); });
        });
    }

    isStorageFull = failure.quotaExceeded;
    setStatus([&](Status& value) {
        value.state = L"error";
        value.error = failure.message;
    });
}

JsonArray activityJson() {
    JsonArray array;

    for (auto const& item : activity) {
        JsonObject object;
        object.SetNamedValue(L"id", str(item.id));
        object.SetNamedValue(L"kind", str(item.kind));
        object.SetNamedValue(L"name", str(item.name));
        object.SetNamedValue(L"detail", optionalString(item.detail));
        object.SetNamedValue(L"at", num(static_cast<double>(item.at)));
        array.Append(object);
    }

    return array;
}

void addActivity(std::wstring const& kind, std::wstring const& name,
                 std::optional<std::wstring> const& detail = std::nullopt) {
    Lock lock(guard);
    activity.push_front({randomHex(6), kind, name, detail, nowMs()});
    while (activity.size() > ACTIVITY_LIMIT) activity.pop_back();

    auto payload = stringify(activityJson());
    outbox().post([payload] { broadcast(L"harbor:activity", JsonValue::Parse(toWide(payload))); });
}

std::shared_future<void> enqueue(std::function<void()> task) {
    return queue().post(std::move(task));
}

JsonValue harborApi(std::wstring const& method, std::wstring const& path,
                    std::optional<JsonObject> const& body = std::nullopt) {
    try {
        return api(method, path, body ? std::optional<JsonValue>(toValue(*body)) : std::nullopt);
    } catch (ApiError const& error) {
        if (error.status == 401) std::thread([] { handleUnauthorized(); }).detach();
        throw;
    }
}

JsonObject harborGet(std::wstring const& path) {
    return harborApi(L"GET", path).GetObject();
}

int64_t toSize(JsonValue const& value) {
    try {
        if (value.ValueType() == JsonValueType::Number) return static_cast<int64_t>(value.GetNumber());
        if (value.ValueType() == JsonValueType::String) return std::stoll(std::wstring(value.GetString()));
        if (value.ValueType() == JsonValueType::Object) {
            auto raw = stringField(value.GetObject(), L"raw");
            if (raw) return std::stoll(*raw);
            auto number = numberField(value.GetObject(), L"raw");
            if (number) return static_cast<int64_t>(*number);
        }
    } catch (std::exception const&) {
    }

    return 0;
}

std::optional<std::wstring> optionalId(JsonObject const& object, std::wstring const& key) {
    return stringField(object, key);
}

RemoteFile toRemoteFile(JsonObject const& file, RemoteFile const* previous = nullptr) {
    RemoteFile result;
    result.id = stringField(file, L"id").value_or(L"");
    result.name = stringField(file, L"name").value_or(previous ? previous->name : L"File");
    result.folderId = file.HasKey(L"folderId") ? optionalId(file, L"folderId")
                                               : (previous ? previous->folderId : std::nullopt);
    result.size = file.HasKey(L"size") ? toSize(file.GetNamedValue(L"size")) : (previous ? previous->size : 0);

    auto createdAt = stringField(file, L"createdAt");
    result.createdAt = createdAt && !createdAt->empty() ? parseIsoDate(*createdAt)
                                                        : (previous ? previous->createdAt : 0);

    if (file.HasKey(L"password")) {
        auto password = file.GetNamedValue(L"password");
        result.isLocked = (password.ValueType() == JsonValueType::String && password.GetString().size() > 0) ||
                          (password.ValueType() == JsonValueType::Boolean && password.GetBoolean());
    } else {
        result.isLocked = previous && previous->isLocked;
    }

    if (file.HasKey(L"expiresAt")) {
        auto expiresAt = stringField(file, L"expiresAt");
        result.expiresAt = expiresAt && !expiresAt->empty() ? std::optional(parseIsoDate(*expiresAt))
                                                            : std::nullopt;
    } else {
        result.expiresAt = previous ? previous->expiresAt : std::nullopt;
    }

    result.maxViews = file.HasKey(L"maxViews") ? static_cast<int64_t>(numberField(file, L"maxViews").value_or(0))
                                               : (previous ? previous->maxViews : 0);
    result.isSensitive = boolField(file, L"piiDetected").value_or(previous && previous->isSensitive);

    std::optional<double> views;
    if (file.HasKey(L"views") && file.GetNamedValue(L"views").ValueType() == JsonValueType::Object) {
        views = numberField(file.GetNamedObject(L"views"), L"total");
    }
    result.views = views ? static_cast<int64_t>(*views) : (previous ? previous->views : 0);

    return result;
}

RemoteFolder toRemoteFolder(JsonObject const& folder, RemoteFolder const* previous = nullptr) {
    RemoteFolder result;
    result.id = stringField(folder, L"id").value_or(L"");
    result.name = stringField(folder, L"name").value_or(previous ? previous->name : L"");
    result.parentId = folder.HasKey(L"parentId") ? optionalId(folder, L"parentId")
                                                 : (previous ? previous->parentId : std::nullopt);

    auto createdAt = stringField(folder, L"createdAt");
    result.createdAt = createdAt && !createdAt->empty() ? parseIsoDate(*createdAt)
                                                        : (previous ? previous->createdAt : 0);
    return result;
}

RemoteNote toRemoteNote(JsonObject const& note) {
    auto content = toUtf8(stringField(note, L"content").value_or(L""));

    RemoteNote result;
    result.id = stringField(note, L"id").value_or(L"");
    result.title = stringField(note, L"title").value_or(L"");
    result.size = static_cast<int64_t>(content.size());
    result.hash = sha1Hex(content);
    result.createdAt = parseIsoDate(stringField(note, L"createdAt").value_or(L""));
    return result;
}

RemoteLink toRemoteLink(JsonObject const& link, RemoteLink const* previous = nullptr) {
    RemoteLink result;
    result.id = stringField(link, L"id").value_or(L"");
    result.code = stringField(link, L"code").value_or(previous ? previous->code : L"");
    result.shortUrl = stringField(link, L"shortUrl").value_or(previous ? previous->shortUrl : L"");

    auto createdAt = stringField(link, L"createdAt");
    result.createdAt = createdAt && !createdAt->empty() ? parseIsoDate(*createdAt)
                                                        : (previous ? previous->createdAt : 0);
    return result;
}

std::vector<JsonObject> loadAll(std::wstring const& path) {
    std::vector<JsonObject> items;
    auto separator = path.find(L'?') == std::wstring::npos ? L"?" : L"&";

    for (int offset = 0;; offset += PAGE_SIZE) {
        auto page = harborGet(path + separator + L"limit=" + std::to_wstring(PAGE_SIZE) + L"&offset=" +
                              std::to_wstring(offset));
        auto pageItems = page.GetNamedArray(L"items", JsonArray());

        for (auto const& item : pageItems) items.push_back(item.GetObject());

        auto total = numberField(page, L"total").value_or(0);
        if (pageItems.Size() < PAGE_SIZE || static_cast<double>(items.size()) >= total) return items;
    }
}

void loadRemote() {
    bool takesNotes = can(L"TakeNotes");
    bool shortensLinks = can(L"ShortenLinks");
    auto load = [](std::wstring path) {
        return std::async(std::launch::async, [path] {
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
            return loadAll(path);
        });
    };

    auto remoteFolders = load(L"/api/folders?sort=createdAt:asc");
    auto remoteFiles = load(L"/api/files?folder=all&sort=createdAt:asc");
    auto remoteNotes = takesNotes ? load(L"/api/notes?sort=createdAt:asc") : std::future<std::vector<JsonObject>>();
    auto remoteLinks = shortensLinks ? load(L"/api/links?sort=createdAt:asc") : std::future<std::vector<JsonObject>>();

    auto folderItems = remoteFolders.get();
    auto fileItems = remoteFiles.get();
    auto noteItems = takesNotes ? remoteNotes.get() : std::vector<JsonObject>();
    auto linkItems = shortensLinks ? remoteLinks.get() : std::vector<JsonObject>();

    Lock lock(guard);
    folders.clear();
    files.clear();
    notes.clear();
    links.clear();

    for (auto const& folder : folderItems) {
        auto value = toRemoteFolder(folder);
        folders[value.id] = value;
    }

    for (auto const& file : fileItems) {
        auto value = toRemoteFile(file);
        files[value.id] = value;
    }

    for (auto const& note : noteItems) {
        auto value = toRemoteNote(note);
        notes[value.id] = value;
    }

    for (auto const& link : linkItems) {
        auto value = toRemoteLink(link);
        links[value.id] = value;
    }
}

std::optional<std::wstring> folderIdAt(std::wstring const& path) {
    Lock lock(guard);
    auto target = lower(path);

    for (auto const& [identity, item] : local.entries()) {
        if (item.isFolder && startsWith(identity, L"d:") && lower(item.path) == target) return idOf(identity);
    }

    return std::nullopt;
}

std::optional<std::wstring> parentFolderIdOf(std::wstring const& path) {
    auto parent = dirnameOf(path);
    if (parent == L".") return std::nullopt;

    auto id = folderIdAt(parent);
    if (!id) throw std::runtime_error(toUtf8(t(L"harbor.onlyFileshipFolders")));

    return id;
}

std::set<std::wstring> subtreeOf(std::wstring const& id) {
    std::set<std::wstring> ids = {id};

    for (size_t size = 0; size != ids.size();) {
        size = ids.size();
        for (auto const& [folderId, folder] : folders) {
            if (folder.parentId && ids.contains(*folder.parentId)) ids.insert(folderId);
        }
    }

    return ids;
}

std::optional<std::wstring> occupantOf(std::wstring const& path) {
    Lock lock(guard);
    auto target = lower(path);

    for (auto const& [identity, item] : local.entries()) {
        if (lower(item.path) == target) return identity;
    }

    return std::nullopt;
}

JsonObject pathPayload(std::wstring const& path) {
    JsonObject payload;
    payload.SetNamedValue(L"path", str(path));
    return payload;
}

JsonObject identityPayload(std::wstring const& path, std::wstring const& identity) {
    auto payload = pathPayload(path);
    payload.SetNamedValue(L"identity", str(identity));
    return payload;
}

JsonObject movePayload(std::wstring const& from, std::wstring const& to) {
    JsonObject payload;
    payload.SetNamedValue(L"from", str(from));
    payload.SetNamedValue(L"to", str(to));
    return payload;
}

std::shared_ptr<HarborHelper> currentHelper() {
    Lock lock(guard);
    return helper;
}

std::shared_ptr<HarborHelper> requireHelper() {
    auto current = currentHelper();
    if (!current) throw std::runtime_error(toUtf8(t(L"harbor.helperNotRunning")));
    return current;
}

bool isRunning() {
    Lock lock(guard);
    return helper && helper->isRunning();
}

void parkOccupant(HarborHelper& current, std::wstring const& path, std::wstring const& identity) {
    auto occupant = occupantOf(path);
    if (!occupant || *occupant == identity) return;

    auto parked = path + L".fileship-" + randomHex(3);
    current.request(L"move", movePayload(path, parked));

    Lock lock(guard);
    if (auto item = local.find(*occupant)) item->path = parked;
}

void placeEntry(HarborHelper& current, LayoutEntry const& entry) {
    std::optional<LocalItem> placed;

    {
        Lock lock(guard);
        if (pendingPaths.contains(entry.path)) return;
        if (auto item = local.find(entry.identity)) placed = *item;
    }

    if (placed && placed->path == entry.path) return;

    parkOccupant(current, entry.path, entry.identity);

    if (placed) {
        current.request(L"move", movePayload(placed->path, entry.path));

        if (entry.isFolder) {
            Lock lock(guard);
            auto prefix = placed->path + L"\\";

            for (auto& [identity, item] : local) {
                if (startsWith(item.path, prefix)) item.path = entry.path + item.path.substr(placed->path.size());
            }
        }
    } else {
        JsonObject item;
        item.SetNamedValue(L"identity", str(entry.identity));
        item.SetNamedValue(L"path", str(entry.path));
        item.SetNamedValue(L"isFolder", boolean(entry.isFolder));
        item.SetNamedValue(L"size", num(static_cast<double>(entry.size)));
        item.SetNamedValue(L"created", num(static_cast<double>(entry.created)));

        JsonArray items;
        items.Append(item);
        JsonObject payload;
        payload.SetNamedValue(L"items", items);
        current.request(L"create", payload);

        Lock lock(guard);
        sentStates[entry.identity] = "[]";
    }

    Lock lock(guard);
    local.set(entry.identity, {entry.path, entry.isFolder});
}

std::optional<std::wstring> expiryOf(RemoteFile const& file) {
    if (file.expiresAt) return t(L"harbor.expiresOn", {{L"date", formatDateTime(*file.expiresAt)}});
    if (file.maxViews) {
        return t(L"harbor.expiresAfterViews", {{L"count", std::to_wstring(file.maxViews)}});
    }
    return std::nullopt;
}

JsonArray propertiesOf(RemoteFile const& file) {
    JsonArray properties;
    auto add = [&](int id, std::wstring const& value) {
        JsonObject property;
        property.SetNamedValue(L"id", num(id));
        property.SetNamedValue(L"value", str(value));
        properties.Append(property);
    };

    if (file.isLocked) add(PROPERTY_LOCKED, t(L"harbor.locked"));
    if (auto expiry = expiryOf(file)) add(PROPERTY_EXPIRES, *expiry);
    if (file.isSensitive) add(PROPERTY_SENSITIVE, t(L"harbor.sensitive"));
    if (file.views) add(PROPERTY_VIEWS, formatNumber(static_cast<double>(file.views)));

    return properties;
}

void sendStates(std::vector<LayoutEntry> const& entries) {
    struct Item {
        std::wstring identity;
        std::wstring path;
        JsonArray properties;
    };

    std::vector<Item> items;
    std::shared_ptr<HarborHelper> current;

    {
        Lock lock(guard);
        current = helper;
        if (!current) return;

        for (auto const& entry : entries) {
            if (entry.isFolder) continue;

            auto file = files.find(idOf(entry.identity));
            auto placed = local.find(entry.identity);
            if (!startsWith(entry.identity, L"f:") || file == files.end() || !placed || placed->path != entry.path) {
                continue;
            }

            auto properties = propertiesOf(file->second);
            auto key = stringify(properties);
            auto sent = sentStates.find(entry.identity);
            if (sent != sentStates.end() && sent->second == key) continue;

            sentStates[entry.identity] = key;
            items.push_back({entry.identity, entry.path, properties});
        }
    }

    for (size_t index = 0; index < items.size(); index += STATES_BATCH) {
        std::vector<Item> batch(items.begin() + index, items.begin() + std::min(items.size(), index + STATES_BATCH));

        statesQueue().post([current, batch] {
            JsonArray payloadItems;
            for (auto const& item : batch) {
                JsonObject value;
                value.SetNamedValue(L"path", str(item.path));
                value.SetNamedValue(L"properties", item.properties);
                payloadItems.Append(value);
            }

            JsonObject payload;
            payload.SetNamedValue(L"items", payloadItems);

            try {
                auto result = current->request(L"setStates", payload);
                std::set<std::wstring> failed;
                for (auto const& value : result.GetNamedArray(L"failed", JsonArray())) {
                    if (value.ValueType() == JsonValueType::String) failed.insert(std::wstring(value.GetString()));
                }

                Lock lock(guard);
                for (auto const& item : batch) {
                    if (failed.contains(item.path)) sentStates.erase(item.identity);
                }
            } catch (...) {
                Lock lock(guard);
                for (auto const& item : batch) sentStates.erase(item.identity);
                logWarn(L"Could not set the Harbor states: " + describe(std::current_exception()).message);
            }
        });
    }
}

void loadStorage() {
    {
        Lock lock(guard);
        if (!hasStorageApi) return;
    }

    std::optional<Storage> storage;

    try {
        auto value = harborGet(L"/api/users/@me/storage");
        storage = Storage{toSize(value.GetNamedValue(L"used", str(L"0"))), std::nullopt};
        if (value.HasKey(L"limit") && !isNullField(value, L"limit")) storage->limit = toSize(value.GetNamedValue(L"limit"));
    } catch (...) {
        auto error = std::current_exception();
        Lock lock(guard);
        if (isMissing(error)) {
            hasStorageApi = false;
        } else {
            logWarn(L"Could not load the storage usage: " + describe(error).message);
        }
    }

    if (isRunning()) setStatus([&](Status& value) { value.storage = storage; });
}

void refreshStorage() {
    Lock lock(guard);
    if (storageTimer) return;

    storageTimer = timers().set(STORAGE_DELAY_MS, [] {
        {
            Lock lock(guard);
            storageTimer = 0;
        }

        runDetached(loadStorage);
    });
}

void refreshDetails() {
    Lock lock(guard);
    if (!helper || !helper->isRunning() || nowMs() - detailsAt < DETAILS_INTERVAL_MS) return;

    detailsAt = nowMs();
    refreshStorage();
    helper->requestAsync(L"usage", JsonObject(), [](std::optional<JsonObject> const& result, std::wstring const& error) {
        if (!result) {
            logWarn(L"Could not measure Harbor: " + error);
            return;
        }

        auto size = static_cast<int64_t>(numberField(*result, L"local").value_or(0));
        setStatus([size](Status& value) { value.localSize = size; });
    });
}

void apply() {
    std::shared_ptr<HarborHelper> current;
    Layout layout;

    {
        Lock lock(guard);
        current = helper;
        if (!current || !current->isRunning() || isPaused) return;

        std::vector<RemoteFolder> folderList;
        std::vector<RemoteFile> fileList;
        for (auto const& [id, folder] : folders) folderList.push_back(folder);
        for (auto const& [id, file] : files) {
            if (!hiddenFiles.contains(id)) fileList.push_back(file);
        }

        std::optional<VirtualFolder<RemoteNote>> noteFolder;
        if (can(L"TakeNotes")) {
            noteFolder = VirtualFolder<RemoteNote>{t(L"harbor.notesFolder"), {}};
            for (auto const& [id, note] : notes) {
                if (!hiddenFiles.contains(id)) noteFolder->items.push_back(note);
            }
        }

        std::optional<VirtualFolder<RemoteLink>> linkFolder;
        if (can(L"ShortenLinks")) {
            linkFolder = VirtualFolder<RemoteLink>{t(L"harbor.linksFolder"), {}};
            for (auto const& [id, link] : links) linkFolder->items.push_back(link);
        }

        layout = buildLayout(folderList, fileList, noteFolder, linkFolder, instanceUrl());
    }

    std::vector<LayoutEntry> entries;
    for (auto const& [identity, entry] : layout.entries()) entries.push_back(entry);

    for (auto const& entry : entries) {
        if (entry.isFolder) tryLog(L"Could not place " + entry.path, [&] { placeEntry(*current, entry); });
    }

    std::set<std::wstring> placed;
    {
        Lock lock(guard);
        for (auto const& [identity, item] : local.entries()) {
            if (item.isFolder) placed.insert(item.path);
        }
    }

    for (auto const& entry : entries) {
        if (entry.isFolder) continue;

        auto parent = dirnameOf(entry.path);
        if (parent != L"." && !placed.contains(parent)) continue;

        tryLog(L"Could not place " + entry.path, [&] { placeEntry(*current, entry); });
    }

    std::vector<std::pair<std::wstring, LocalItem>> stale;
    {
        Lock lock(guard);
        for (auto const& [identity, item] : local.entries()) {
            if (!layout.has(identity) && !pendingPaths.contains(item.path)) stale.emplace_back(identity, item);
        }
    }

    std::ranges::stable_sort(stale, [](auto const& a, auto const& b) { return a.second.isFolder < b.second.isFolder; });

    for (auto const& [identity, item] : stale) {
        tryLog(L"Could not remove " + item.path, [&] {
            current->request(L"remove", pathPayload(item.path));
            Lock lock(guard);
            local.erase(identity);
        });
    }

    sendStates(entries);

    {
        Lock lock(guard);
        isStorageFull = false;
        bool isSyncing = !pendingPaths.empty();
        setStatus([&](Status& value) {
            value.state = isSyncing ? L"syncing" : L"ready";
            value.error.reset();
            if (!isSyncing) value.lastSyncedAt = nowMs();
        });
    }

    refreshStorage();
}

void scheduleApply() {
    Lock lock(guard);
    timers().clear(applyTimer);
    applyTimer = timers().set(APPLY_DELAY_MS, [] {
        {
            Lock lock(guard);
            applyTimer = 0;
        }

        enqueue(apply);
    });
}

void reloadNow() {
    {
        Lock lock(guard);
        if (!isPaused) setStatus([](Status& value) { value.state = L"syncing"; });
    }

    loadRemote();

    auto current = requireHelper();
    auto items = current->scan();
    std::vector<HelperEvent> unsynced;

    {
        Lock lock(guard);
        local.clear();
        sentStates.clear();

        int64_t localSize = 0;
        for (auto const& item : items) {
            if (!item.isFolder && item.hydrated) localSize += item.size;
        }
        status.localSize = localSize;

        for (auto const& item : items) {
            if (item.isUnreadable) {
                logWarn(L"Could not read the Harbor placeholder " + item.path);
            } else if (!item.identity || isPendingIdentity(*item.identity)) {
                HelperEvent event;
                event.type = L"localNew";
                event.path = item.path;
                event.isFolder = item.isFolder;
                event.size = item.size;
                unsynced.push_back(event);
            } else if (!item.isFolder && item.modified && item.hydrated) {
                HelperEvent event;
                event.type = L"localModified";
                event.path = item.path;
                event.identity = *item.identity;
                event.size = item.size;
                unsynced.push_back(event);

                if (files.contains(idOf(*item.identity))) local.set(*item.identity, {item.path, false});
            } else {
                local.set(*item.identity, {item.path, item.isFolder});
            }
        }
    }

    apply();

    JsonArray stuck;
    {
        Lock lock(guard);
        for (auto const& item : items) {
            if (item.inSync || !item.identity || isPendingIdentity(*item.identity) || item.modified ||
                pendingPaths.contains(item.path)) {
                continue;
            }

            auto placed = local.find(*item.identity);
            if (placed && placed->path == item.path && isKnown(*item.identity)) stuck.Append(str(item.path));
        }
    }

    JsonObject payload;
    payload.SetNamedValue(L"paths", stuck);
    tryLog(L"Could not mark Harbor in sync", [&] { current->request(L"markInSync", payload); });

    for (auto const& event : unsynced) handleHelperEvent(event);
}

std::shared_future<void> reload() {
    return enqueue(reloadNow);
}

bool isResyncQueued = false;

void resync() {
    {
        Lock lock(guard);
        if (isResyncQueued) return;
        isResyncQueued = true;
    }

    enqueue([] {
        {
            Lock lock(guard);
            isResyncQueued = false;
        }

        try {
            reloadNow();
        } catch (...) {
            auto error = std::current_exception();
            logWarn(L"Could not reload Harbor: " + describe(error).message);

            Lock lock(guard);
            isSynced = false;
            if (!isUnauthorized(error)) scheduleRetry(describe(error).message);
        }
    });
}

JsonObject trashPayload(std::vector<std::wstring> const& fileIds, std::vector<std::wstring> const& folderIds,
                        std::vector<std::wstring> const& noteIds) {
    auto array = [](std::vector<std::wstring> const& ids) {
        JsonArray result;
        for (auto const& id : ids) result.Append(str(id));
        return result;
    };

    JsonObject payload;
    payload.SetNamedValue(L"files", array(fileIds));
    payload.SetNamedValue(L"folders", array(folderIds));
    payload.SetNamedValue(L"notes", array(noteIds));
    return payload;
}

void ignoreMissing(std::function<void()> const& call) {
    try {
        call();
    } catch (...) {
        if (!isMissing(std::current_exception())) throw;
    }
}

void deleteEntry(std::wstring const& identity, bool isPermanent) {
    auto id = idOf(identity);
    bool isNote = startsWith(identity, L"n:");
    std::wstring linkId;

    {
        Lock lock(guard);
        if (auto link = linkByCode(id)) linkId = link->id;
    }

    ignoreMissing([&] {
        harborApi(L"DELETE", isNote ? L"/api/notes/" + id : L"/api/links/" + (linkId.empty() ? L"undefined" : linkId));
    });

    if (isNote && isPermanent) harborApi(L"POST", L"/api/trash/delete", trashPayload({}, {}, {id}));

    Lock lock(guard);
    notes.erase(id);
    if (!linkId.empty()) links.erase(linkId);
    local.erase(identity);
}

void deleteRemote(std::wstring const& identity, bool isPermanent = false) {
    if (startsWith(identity, L"n:") || startsWith(identity, L"l:")) {
        deleteEntry(identity, isPermanent);
        return;
    }

    auto id = idOf(identity);
    bool isFile = startsWith(identity, L"f:");

    ignoreMissing([&] {
        if (isFile) {
            harborApi(L"DELETE", L"/api/files/" + id);
        } else {
            JsonObject body;
            body.SetNamedValue(L"deleteFilesToo", boolean(true));
            harborApi(L"DELETE", L"/api/folders/" + id, body);
        }
    });

    if (isPermanent) {
        harborApi(L"POST", L"/api/trash/delete",
                  isFile ? trashPayload({id}, {}, {}) : trashPayload({}, {id}, {}));
    }

    Lock lock(guard);

    if (isFile) {
        files.erase(id);
    } else {
        auto removed = subtreeOf(id);

        for (auto const& folderId : removed) folders.erase(folderId);
        for (auto file = files.begin(); file != files.end();) {
            if (file->second.folderId && removed.contains(*file->second.folderId)) {
                file = files.erase(file);
            } else {
                ++file;
            }
        }
    }

    std::optional<LocalItem> removed;
    if (auto item = local.find(identity)) removed = *item;
    local.erase(identity);

    if (removed && removed->isFolder) {
        std::vector<std::wstring> inside;
        for (auto const& [key, item] : local.entries()) {
            if (startsWith(item.path, removed->path + L"\\")) inside.push_back(key);
        }
        for (auto const& key : inside) local.erase(key);
    }
}

std::wstring noteTitleOf(std::wstring const& path) {
    auto name = basenameOf(path);
    auto extension = extnameOf(path);
    return name.substr(0, name.size() - extension.size());
}

std::filesystem::path absoluteOf(std::wstring const& path) {
    Lock lock(guard);
    return std::filesystem::path(root) / path;
}

void saveNote(std::wstring const& identity, std::wstring const& path, bool isReplaced) {
    auto id = idOf(identity);
    auto content = readFileText(absoluteOf(path));

    JsonObject body;
    body.SetNamedValue(L"content", str(toWide(content)));
    auto note = harborApi(L"PATCH", L"/api/notes/" + id, body).GetObject();

    {
        Lock lock(guard);
        notes[id] = toRemoteNote(note);
        localHashes[id] = sha1Hex(content);
    }

    requireHelper()->request(isReplaced ? L"adopt" : L"update", identityPayload(path, identity));

    {
        Lock lock(guard);
        local.set(identity, {path, false});
    }

    addActivity(L"note", basenameOf(path));
}

class PendingPath {
public:
    explicit PendingPath(std::wstring path) : path(std::move(path)) {
        Lock lock(guard);
        pendingPaths.insert(this->path);
    }

    ~PendingPath() {
        Lock lock(guard);
        pendingPaths.erase(path);
    }

    PendingPath(PendingPath const&) = delete;
    PendingPath& operator=(PendingPath const&) = delete;

private:
    std::wstring path;
};

bool isPending(std::wstring const& path) {
    Lock lock(guard);
    return pendingPaths.contains(path);
}

void createNote(HelperEvent const& event) {
    {
        Lock lock(guard);
        if (replacedNotes.contains(event.path)) return;
    }

    auto occupant = occupantOf(event.path);

    if (occupant && startsWith(*occupant, L"n:")) {
        {
            PendingPath pending(event.path);
            saveNote(*occupant, event.path, true);
        }

        setStatus();
        scheduleApply();
        return;
    }

    auto extension = lower(extnameOf(event.path));
    if (extension != L".md" && extension != L".txt") {
        addActivity(L"error", basenameOf(event.path), t(L"harbor.notesOnlyText"));
        return;
    }

    auto content = readFileText(absoluteOf(event.path));
    if (content.empty()) return;

    {
        PendingPath pending(event.path);

        JsonObject body;
        body.SetNamedValue(L"title", str(noteTitleOf(event.path)));
        body.SetNamedValue(L"content", str(toWide(content)));
        auto note = toRemoteNote(harborApi(L"POST", L"/api/notes", body).GetObject());

        {
            Lock lock(guard);
            notes[note.id] = note;
            localHashes[note.id] = sha1Hex(content);
        }

        requireHelper()->request(L"adopt", identityPayload(event.path, noteIdentity(note.id)));

        Lock lock(guard);
        local.set(noteIdentity(note.id), {event.path, false});
    }

    addActivity(L"note", basenameOf(event.path));
    setStatus();
    scheduleApply();
}

void updateNote(HelperEvent const& event) {
    if (isPending(event.path)) return;

    {
        PendingPath pending(event.path);
        saveNote(event.identity, event.path, false);
    }

    setStatus();
}

void refreshNote(std::wstring const& id) {
    enqueue([id] {
        std::shared_ptr<HarborHelper> current;
        std::wstring path;
        int64_t size = 0;

        {
            Lock lock(guard);
            auto note = notes.find(id);
            auto entry = local.find(noteIdentity(id));
            auto hash = localHashes.find(id);
            if (note == notes.end() || !entry || !helper || !helper->isRunning() ||
                (hash != localHashes.end() && hash->second == note->second.hash)) {
                return;
            }

            current = helper;
            path = entry->path;
            size = note->second.size;
        }

        tryLog(L"Could not refresh the note " + path, [&] {
            auto payload = pathPayload(path);
            payload.SetNamedValue(L"size", num(static_cast<double>(size)));
            auto result = current->request(L"refresh", payload);

            if (boolField(result, L"refreshed").value_or(false)) {
                Lock lock(guard);
                localHashes.erase(id);
            }
        });
    });
}

void handleDelete(HelperEvent const& event) {
    if (isPendingIdentity(event.identity)) {
        Lock lock(guard);
        cancelledUploads.insert(event.identity);
        return;
    }

    if (event.isFolder) {
        if (startsWith(event.identity, L"v:")) {
            {
                Lock lock(guard);
                local.erase(event.identity);
            }
            scheduleApply();
            return;
        }

        try {
            deleteRemote(event.identity);
            addActivity(L"trash", basenameOf(event.path));
            setStatus();
        } catch (...) {
            logWarn(L"Could not delete on Fileship " + event.path + L": " +
                    describe(std::current_exception()).message);
            scheduleApply();
        }

        return;
    }

    auto id = idOf(event.identity);
    auto absolute = absoluteOf(event.path);
    bool isNote = startsWith(event.identity, L"n:");

    {
        Lock lock(guard);
        hiddenFiles.insert(id);
        local.erase(event.identity);
        if (isNote) replacedNotes.insert(event.path);
    }

    setStatus();

    timers().set(REPLACE_WINDOW_MS, [event, id, absolute, isNote] {
        enqueue([event, id, absolute, isNote] {
            try {
                if (isNote && pathExists(absolute)) {
                    saveNote(event.identity, event.path, true);
                } else {
                    deleteRemote(event.identity);
                    addActivity(startsWith(event.identity, L"l:") ? L"delete" : L"trash", basenameOf(event.path));
                }
            } catch (...) {
                logWarn(L"Could not delete on Fileship " + event.path + L": " +
                        describe(std::current_exception()).message);
            }

            {
                Lock lock(guard);
                hiddenFiles.erase(id);
                replacedNotes.erase(event.path);
            }

            setStatus();
            scheduleApply();
        });
    });
}

JsonValue optionalIdValue(std::optional<std::wstring> const& id) {
    return id ? str(*id) : nul();
}

void renameFolder(std::wstring const& identity, std::wstring const& from, std::wstring const& to) {
    auto id = idOf(identity);
    auto parentId = parentFolderIdOf(to);
    auto name = basenameOf(to).substr(0, 32);
    std::optional<RemoteFolder> folder;

    {
        Lock lock(guard);
        auto found = folders.find(id);
        if (found != folders.end()) folder = found->second;
    }

    JsonObject body;
    if (!folder || name != folder->name) body.SetNamedValue(L"name", str(name));
    if (parentId != (folder ? folder->parentId : std::nullopt)) body.SetNamedValue(L"parentId", optionalIdValue(parentId));

    if (body.Size()) {
        auto updated = harborApi(L"PATCH", L"/api/folders/" + id, body).GetObject();
        Lock lock(guard);
        folders[id] = toRemoteFolder(updated, folder ? &*folder : nullptr);
    }

    Lock lock(guard);
    local.set(identity, {to, true});

    for (auto& [key, item] : local) {
        if (startsWith(item.path, from + L"\\")) item.path = to + item.path.substr(from.size());
    }
}

void moveFile(std::wstring const& identity, std::wstring const& to) {
    auto id = idOf(identity);
    auto folderId = parentFolderIdOf(to);
    auto name = basenameOf(to);
    std::optional<RemoteFile> file;

    {
        Lock lock(guard);
        auto found = files.find(id);
        if (found != files.end()) file = found->second;
    }

    JsonObject body;
    if (!file || name != file->name) body.SetNamedValue(L"name", str(name));
    if (folderId != (file ? file->folderId : std::nullopt)) body.SetNamedValue(L"folderId", optionalIdValue(folderId));

    if (body.Size()) {
        auto updated = harborApi(L"PATCH", L"/api/files/" + id, body).GetObject();
        updated.SetNamedValue(L"folderId", optionalIdValue(folderId));

        Lock lock(guard);
        files[id] = toRemoteFile(updated, file ? &*file : nullptr);
    }

    Lock lock(guard);
    local.set(identity, {to, false});
}

void renameNote(std::wstring const& identity, std::wstring const& to) {
    auto id = idOf(identity);

    JsonObject body;
    body.SetNamedValue(L"title", str(noteTitleOf(to)));
    auto note = harborApi(L"PATCH", L"/api/notes/" + id, body).GetObject();

    Lock lock(guard);
    notes[id] = toRemoteNote(note);
    local.set(identity, {to, false});
}

bool isFixedRename(HelperEvent const& event) {
    return isPendingIdentity(event.identity) || startsWith(event.identity, L"v:") ||
           (startsWith(event.identity, L"l:") && event.to) ||
           (startsWith(event.identity, L"n:") && event.to && dirnameOf(*event.to) != dirnameOf(event.from));
}

void ack(uint64_t requestId, bool ok) {
    if (auto current = currentHelper()) current->ack(requestId, ok);
}

void handleRename(HelperEvent const& event) {
    if (isFixedRename(event)) {
        ack(event.requestId, false);
        return;
    }

    try {
        if (!event.to) {
            deleteRemote(event.identity);
            addActivity(L"trash", basenameOf(event.from));
        } else if (startsWith(event.identity, L"n:")) {
            renameNote(event.identity, *event.to);
            addActivity(L"rename", basenameOf(*event.to));
        } else if (event.isFolder) {
            renameFolder(event.identity, event.from, *event.to);
            addActivity(dirnameOf(*event.to) == dirnameOf(event.from) ? L"rename" : L"move", basenameOf(*event.to));
        } else {
            moveFile(event.identity, *event.to);
            addActivity(dirnameOf(*event.to) == dirnameOf(event.from) ? L"rename" : L"move", basenameOf(*event.to));
        }

        ack(event.requestId, true);
        setStatus();
        scheduleApply();
    } catch (...) {
        auto message = describe(std::current_exception()).message;
        logWarn(L"Could not rename on Fileship " + event.from + L": " + message);
        addActivity(L"error", basenameOf(event.from), message);
        ack(event.requestId, false);
    }
}

class ProgressReporter : public std::enable_shared_from_this<ProgressReporter> {
public:
    ProgressReporter(std::shared_ptr<HarborHelper> helper, std::wstring path)
        : helper(std::move(helper)), path(std::move(path)) {}

    void report(int64_t sent, int64_t total) {
        std::lock_guard lock(mutex);
        latest = {sent, total};
        if (!inFlight) sendNext();
    }

    void wait() {
        std::unique_lock lock(mutex);
        idle.wait_for(lock, std::chrono::seconds(30), [this] { return !inFlight; });
    }

private:
    void sendNext() {
        if (!latest || !helper) return;

        auto [sent, total] = *latest;
        latest.reset();
        inFlight = true;

        auto payload = pathPayload(path);
        payload.SetNamedValue(L"total", num(static_cast<double>(total)));
        payload.SetNamedValue(L"completed", num(static_cast<double>(sent)));

        auto self = shared_from_this();
        helper->requestAsync(L"progress", payload, [self](std::optional<JsonObject> const& result, std::wstring const& error) {
            std::lock_guard lock(self->mutex);
            if (!result) logWarn(L"Could not report progress " + self->path + L": " + error);
            self->inFlight = false;
            self->sendNext();
            if (!self->inFlight) self->idle.notify_all();
        });
    }

    std::shared_ptr<HarborHelper> helper;
    std::wstring path;
    std::mutex mutex;
    std::condition_variable idle;
    std::optional<std::pair<int64_t, int64_t>> latest;
    bool inFlight = false;
};

RemoteFile uploadLocal(std::wstring const& path, std::optional<std::wstring> const& folderId) {
    auto absolute = absoluteOf(path);
    auto size = static_cast<int64_t>(std::filesystem::file_size(absolute));
    auto reporter = std::make_shared<ProgressReporter>(currentHelper(), path);

    UploadSource source;
    source.path = absolute;
    source.name = basenameOf(path);
    source.mimeType = mimeTypeOf(path);

    UploadOptions options;
    options.fileNameType = L"Random";
    options.folderId = folderId;

    auto uploaded = uploadFile(source, options, [reporter](int64_t sent, int64_t total) { reporter->report(sent, total); });
    reporter->wait();

    RemoteFile file;
    file.id = uploaded.id;
    file.name = uploaded.name;
    file.folderId = folderId;
    file.size = size;
    file.createdAt = nowMs();
    return file;
}

std::optional<Links> fileLinksOf(std::wstring const& id) {
    auto file = harborGet(L"/api/files/" + id);
    auto embed = file.HasKey(L"embed") && file.GetNamedValue(L"embed").ValueType() == JsonValueType::Object
                     ? boolField(file.GetNamedObject(L"embed"), L"enabled").value_or(false)
                     : false;
    bool isPage = embed || startsWith(stringField(file, L"mimeType").value_or(L""), L"text/");
    auto embedUrl = stringField(file, L"embedUrl").value_or(L"");
    auto directUrl = stringField(file, L"directUrl").value_or(L"");

    return Links{isPage ? embedUrl : directUrl, embedUrl};
}

void copyUploadedLinks() {
    std::vector<std::wstring> ids;

    {
        Lock lock(guard);
        ids.swap(linksToCopy);
    }

    try {
        std::vector<Links> targets;
        for (auto const& id : ids) targets.push_back(*fileLinksOf(id));

        std::wstring text;
        for (auto const& target : targets) text += (text.empty() ? L"" : L"\n") + target.copy;

        writeClipboardText(text);
        auto open = targets[0].open;
        notify(t(L"harbor.linkCopied", {{L"count", std::to_wstring(targets.size())}}), text,
               [open] { openWebUrl(open); });
    } catch (...) {
        auto message = describe(std::current_exception()).message;
        logWarn(L"Could not copy the Harbor links: " + message);
        notify(t(L"harbor.linkFailed"), message);
    }
}

void queueLinkCopy(std::wstring const& id) {
    Lock lock(guard);
    linksToCopy.push_back(id);
    timers().clear(copyTimer);
    copyTimer = timers().set(COPY_DELAY_MS, [] {
        {
            Lock lock(guard);
            copyTimer = 0;
        }

        runDetached(copyUploadedLinks);
    });
}

using Finish = std::function<void(RemoteFile const&)>;

void finishUpload(RemoteFile const& file, Finish const& finish) {
    for (int attempt = 1;; attempt++) {
        try {
            enqueue([file, finish] {
                {
                    Lock lock(guard);
                    files[file.id] = file;
                }

                if (isRunning()) finish(file);
            }).get();
            return;
        } catch (...) {
            if (attempt >= FINISH_ATTEMPTS || isUnauthorized(std::current_exception())) throw;
            std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
        }
    }
}

void startUpload(HelperEvent const& event, std::optional<std::wstring> const& folderId, Finish finish,
                 std::function<void()> cleanup, std::function<bool()> isCancelled = [] { return false; }) {
    runDetached([event, folderId, finish, cleanup, isCancelled] {
        auto const& path = event.path;

        auto release = [&] {
            {
                Lock lock(guard);
                pendingPaths.erase(path);
            }
            cleanup();
        };

        auto fail = [&](std::exception_ptr error) {
            {
                Lock lock(guard);
                uploadRetries.erase(path);
            }
            release();

            auto message = describe(error).message;
            logWarn(L"Could not upload " + path + L": " + message);
            addActivity(L"error", basenameOf(path), message);
            setError(error);
        };

        RemoteFile file;

        try {
            file = uploadLocal(path, folderId);
        } catch (...) {
            auto error = std::current_exception();
            auto failure = describe(error);
            size_t attempt = 0;

            {
                Lock lock(guard);
                auto found = uploadRetries.find(path);
                if (found != uploadRetries.end()) attempt = found->second;
            }

            if (attempt >= RETRY_DELAYS_MS.size() || failure.status == 401 || failure.quotaExceeded ||
                isCancelled() || !isRunning() || !pathExists(absoluteOf(path))) {
                fail(error);
                return;
            }

            {
                Lock lock(guard);
                uploadRetries[path] = attempt + 1;
            }
            release();
            logWarn(L"Could not upload, trying again " + path + L": " + failure.message);

            timers().set(RETRY_DELAYS_MS[attempt], [event] {
                if (isRunning()) {
                    handleHelperEvent(event);
                } else {
                    Lock lock(guard);
                    uploadRetries.erase(event.path);
                }
            });
            return;
        }

        try {
            finishUpload(file, finish);
        } catch (...) {
            fail(std::current_exception());
            return;
        }

        bool isKept = !isCancelled();

        {
            Lock lock(guard);
            uploadRetries.erase(path);
        }
        release();

        if (isKept) {
            addActivity(event.type == L"localNew" ? L"upload" : L"update", basenameOf(path));
            if (boolField(harborSettings(), L"copyLinkOnUpload").value_or(false)) queueLinkCopy(file.id);
        }

        setStatus();
        scheduleApply();
    });
}

void handleLocalModified(HelperEvent const& event);

void handleLocalNew(HelperEvent const& event) {
    if (isPending(event.path) || event.path.find(L".fileship-") != std::wstring::npos) return;

    std::optional<std::wstring> existing;
    try {
        existing = requireHelper()->identityOf(event.path);
    } catch (...) {
        logWarn(L"Could not read " + event.path + L", trying again: " + describe(std::current_exception()).message);
        timers().set(RETRY_DELAY_MS, [event] { handleHelperEvent(event); });
        return;
    }

    if (existing && !isPendingIdentity(*existing)) {
        Lock lock(guard);
        local.set(*existing, {event.path, event.isFolder});
        return;
    }

    auto parent = dirnameOf(event.path);
    auto parentIdentity = parent == L"." ? std::nullopt : occupantOf(parent);
    auto folderId = parent == L"." ? std::nullopt : folderIdAt(parent);

    if (parent != L"." && !folderId && !(parentIdentity && startsWith(*parentIdentity, L"v:"))) {
        if (isPending(parent)) {
            timers().set(RETRY_DELAY_MS, [event] { handleHelperEvent(event); });
        }
        return;
    }

    if (event.isFolder) {
        if (parentIdentity && startsWith(*parentIdentity, L"v:")) return;

        {
            PendingPath pending(event.path);

            JsonObject body;
            body.SetNamedValue(L"name", str(basenameOf(event.path).substr(0, 32)));
            body.SetNamedValue(L"parentId", optionalIdValue(folderId));
            auto folder = toRemoteFolder(harborApi(L"POST", L"/api/folders", body).GetObject());

            {
                Lock lock(guard);
                folders[folder.id] = folder;
            }

            requireHelper()->request(L"adopt", identityPayload(event.path, folderIdentity(folder.id)));

            Lock lock(guard);
            local.set(folderIdentity(folder.id), {event.path, true});
        }

        addActivity(L"folder", basenameOf(event.path));
        setStatus();
        scheduleApply();
        return;
    }

    if (parentIdentity == LINKS_FOLDER) return;
    if (parentIdentity == NOTES_FOLDER) {
        createNote(event);
        return;
    }

    auto occupant = occupantOf(event.path);

    if (occupant && startsWith(*occupant, L"f:")) {
        HelperEvent modified;
        modified.type = L"localModified";
        modified.path = event.path;
        modified.identity = *occupant;
        modified.size = event.size;
        handleLocalModified(modified);
        return;
    }

    {
        Lock lock(guard);
        pendingPaths.insert(event.path);
    }
    setStatus([](Status& value) { value.state = L"syncing"; });

    auto generated = L"p:" + randomHex(8);
    std::wstring pending = generated;

    try {
        auto prepared = requireHelper()->request(L"prepare", identityPayload(event.path, generated));
        auto identity = stringField(prepared, L"identity");
        if (identity && isPendingIdentity(*identity)) pending = *identity;
    } catch (...) {
        logWarn(L"Could not prepare " + event.path + L": " + describe(std::current_exception()).message);
    }

    auto path = event.path;

    startUpload(
        event, folderId,
        [pending, path](RemoteFile const& file) {
            bool isCancelled = false;
            {
                Lock lock(guard);
                isCancelled = cancelledUploads.contains(pending);
            }

            if (isCancelled) {
                deleteRemote(fileIdentity(file.id), true);
                return;
            }

            requireHelper()->request(L"adopt", identityPayload(path, fileIdentity(file.id)));

            Lock lock(guard);
            local.set(fileIdentity(file.id), {path, false});
        },
        [pending] {
            Lock lock(guard);
            cancelledUploads.erase(pending);
        },
        [pending] {
            Lock lock(guard);
            return cancelledUploads.contains(pending);
        });
}

void handleLocalModified(HelperEvent const& event) {
    if (startsWith(event.identity, L"n:")) {
        updateNote(event);
        return;
    }

    if (isPending(event.path) || !startsWith(event.identity, L"f:")) return;

    auto previousId = idOf(event.identity);
    std::optional<std::wstring> previousFolder;

    {
        Lock lock(guard);
        auto previous = files.find(previousId);
        if (previous != files.end()) previousFolder = previous->second.folderId;
        pendingPaths.insert(event.path);
        hiddenFiles.insert(previousId);
    }

    setStatus([](Status& value) { value.state = L"syncing"; });

    tryLog(L"Could not prepare " + event.path,
           [&] { requireHelper()->request(L"prepare", identityPayload(event.path, event.identity)); });

    auto path = event.path;
    auto identity = event.identity;

    startUpload(
        event, previousFolder,
        [previousId, path, identity](RemoteFile const& file) {
            ignoreMissing([&] { harborApi(L"DELETE", L"/api/files/" + previousId); });

            {
                Lock lock(guard);
                files.erase(previousId);
            }

            requireHelper()->request(L"update", identityPayload(path, fileIdentity(file.id)));

            Lock lock(guard);
            local.erase(identity);
            local.set(fileIdentity(file.id), {path, false});
        },
        [previousId] {
            Lock lock(guard);
            hiddenFiles.erase(previousId);
        });
}

void runHelperEvent(HelperEvent const& event) {
    if (event.type == L"delete") return handleDelete(event);
    if (event.type == L"rename") return handleRename(event);

    {
        Lock lock(guard);
        if (isPaused) return;
    }

    if (event.type == L"localNew") return handleLocalNew(event);
    if (event.type != L"localModified") return;

    if (isPendingIdentity(event.identity)) {
        HelperEvent created = event;
        created.type = L"localNew";
        created.isFolder = false;
        return handleLocalNew(created);
    }

    handleLocalModified(event);
}

void handleHelperEvent(HelperEvent event) {
    if (event.type == L"rescan") {
        resync();
        return;
    }

    if (event.type == L"action") {
        runHarborAction(event.action, event.paths);
        return;
    }

    if (event.type == L"statusRequested") {
        refreshDetails();
        return;
    }

    enqueue([event] {
        try {
            runHelperEvent(event);
        } catch (...) {
            auto error = std::current_exception();
            auto message = describe(error).message;
            logWarn(L"Harbor " + event.type + L" failed: " + message);
            addActivity(L"error", basenameOf(event.type == L"rename" ? event.from : event.path), message);
            setError(error);
        }
    });
}

bool isOwn(JsonObject const& item) {
    auto authorId = stringField(item, L"authorId");
    return authorId && *authorId == userId();
}

JsonObject objectOf(IJsonValue const& value) {
    return value.ValueType() == JsonValueType::Object ? value.GetObject() : JsonObject();
}

std::wstring stringOf(IJsonValue const& value) {
    return value.ValueType() == JsonValueType::String ? std::wstring(value.GetString()) : L"";
}

void connectSocket() {
    auto endpoint = apiEndpoint();
    if (instanceUrl().empty() || token().empty()) return;

    endpoint.baseUrl = instanceUrl();
    endpoint.token = token();
    auto client = std::make_shared<SocketIo>(endpoint);

    client->on(L"file:create", [](IJsonValue const& payload) {
        auto file = objectOf(payload);
        if ((file.HasKey(L"deletedAt") && !isNullField(file, L"deletedAt")) || !isOwn(file)) return;

        auto value = toRemoteFile(file);
        Lock lock(guard);
        files[value.id] = value;
        scheduleApply();
    });

    client->on(L"file:update", [](IJsonValue const& payload) {
        auto file = objectOf(payload);
        auto id = stringField(file, L"id").value_or(L"");

        Lock lock(guard);
        auto previous = files.find(id);
        if (previous == files.end()) return;

        previous->second = toRemoteFile(file, &previous->second);
        scheduleApply();
    });

    client->on(L"file:delete", [](IJsonValue const& payload) {
        Lock lock(guard);
        files.erase(stringOf(payload));
        scheduleApply();
    });

    client->on(L"file:bulkDelete", [](IJsonValue const& payload) {
        Lock lock(guard);
        if (payload.ValueType() == JsonValueType::Array) {
            for (auto const& id : payload.GetArray()) files.erase(stringOf(id));
        }
        scheduleApply();
    });

    client->on(L"file:bulkMove", [](IJsonValue const& payload) {
        auto body = objectOf(payload);
        std::optional<std::wstring> folderId;
        if (body.HasKey(L"folder") && body.GetNamedValue(L"folder").ValueType() == JsonValueType::Object) {
            folderId = stringField(body.GetNamedObject(L"folder"), L"id");
        }

        Lock lock(guard);
        for (auto const& id : body.GetNamedArray(L"files", JsonArray())) {
            auto file = files.find(stringOf(id));
            if (file != files.end()) file->second.folderId = folderId;
        }
        scheduleApply();
    });

    client->on(L"folder:create", [](IJsonValue const& payload) {
        auto folder = objectOf(payload);
        if (!isOwn(folder)) return;

        auto value = toRemoteFolder(folder);
        Lock lock(guard);
        folders[value.id] = value;
        scheduleApply();
    });

    client->on(L"folder:update", [](IJsonValue const& payload) {
        auto folder = objectOf(payload);
        if (!isOwn(folder)) return;

        auto id = stringField(folder, L"id").value_or(L"");
        Lock lock(guard);

        if (folder.HasKey(L"deletedAt") && !isNullField(folder, L"deletedAt")) {
            folders.erase(id);
        } else {
            auto previous = folders.find(id);
            folders[id] = toRemoteFolder(folder, previous == folders.end() ? nullptr : &previous->second);
        }
        scheduleApply();
    });

    client->on(L"note:create", [](IJsonValue const& payload) {
        auto note = objectOf(payload);
        if (note.HasKey(L"deletedAt") && !isNullField(note, L"deletedAt")) return;

        auto value = toRemoteNote(note);
        Lock lock(guard);
        notes[value.id] = value;
        scheduleApply();
    });

    client->on(L"note:update", [](IJsonValue const& payload) {
        auto next = toRemoteNote(objectOf(payload));
        bool isChanged = false;

        {
            Lock lock(guard);
            auto previous = notes.find(next.id);
            isChanged = previous != notes.end() && previous->second.hash != next.hash;
            notes[next.id] = next;
        }

        if (isChanged) refreshNote(next.id);
        scheduleApply();
    });

    client->on(L"note:delete", [](IJsonValue const& payload) {
        Lock lock(guard);
        notes.erase(stringOf(payload));
        scheduleApply();
    });

    client->on(L"link:create", [](IJsonValue const& payload) {
        auto value = toRemoteLink(objectOf(payload));
        Lock lock(guard);
        links[value.id] = value;
        scheduleApply();
    });

    client->on(L"link:update", [](IJsonValue const& payload) {
        auto link = objectOf(payload);
        auto id = stringField(link, L"id").value_or(L"");

        Lock lock(guard);
        auto previous = links.find(id);
        if (previous == links.end() && !stringField(link, L"code")) return;

        auto value = toRemoteLink(link, previous == links.end() ? nullptr : &previous->second);
        if (previous != links.end() && previous->second.code == value.code &&
            previous->second.shortUrl == value.shortUrl) {
            return;
        }

        links[id] = value;
        scheduleApply();
    });

    client->on(L"link:delete", [](IJsonValue const& payload) {
        Lock lock(guard);
        links.erase(stringOf(payload));
        scheduleApply();
    });

    client->on(L"folder:delete", [](IJsonValue const& payload) {
        auto id = stringOf(payload);

        Lock lock(guard);
        auto folder = folders.find(id);
        if (folder == folders.end()) return;

        auto parentId = folder->second.parentId;
        auto removed = subtreeOf(id);

        for (auto const& folderId : removed) folders.erase(folderId);
        for (auto& [fileId, file] : files) {
            if (file.folderId && removed.contains(*file.folderId)) file.folderId = parentId;
        }
        scheduleApply();
    });

    for (auto event : {L"trash:restore", L"currentUser:update", L"deleteAll"}) {
        client->on(event, [](IJsonValue const&) { resync(); });
    }

    client->on(L"settings:update", [](IJsonValue const&) { resetUploadSettings(); });
    client->on(L"logout", [](IJsonValue const&) { std::thread([] { handleUnauthorized(); }).detach(); });
    client->onReconnect([] { resync(); });

    socket = client;
    client->connect();
}

std::shared_ptr<SocketIo> takeSocket() {
    Lock lock(guard);
    auto client = socket;
    socket = nullptr;
    return client;
}

std::wstring syncRootKey() {
    return toWide(sha256Hex(toUtf8(instanceUrl() + L"|" + userId())).substr(0, 16));
}

bool holdsHarbor(HarborHelper& current) {
    for (auto const& item : current.scan()) {
        if (item.identity && !isPendingIdentity(*item.identity)) return true;
    }

    return false;
}

bool isAllowed() {
    return boolField(harborSettings(), L"enabled").value_or(true) && currentUser() && !instanceUrl().empty() &&
           !token().empty();
}

void clearRetry() {
    Lock lock(guard);
    timers().clear(retryTimer);
    retryTimer = 0;
}

void scheduleRetry(std::wstring const& error) {
    clearRetry();

    Lock lock(guard);
    isStorageFull = false;
    setStatus([&](Status& value) {
        value.state = L"error";
        value.error = error;
    });

    int delay = RETRY_DELAYS_MS[std::min<size_t>(retries, RETRY_DELAYS_MS.size() - 1)];
    retries++;
    retryTimer = timers().set(delay, [] {
        {
            Lock lock(guard);
            retryTimer = 0;
        }

        startHarbor();
    });
}

std::shared_future<void> serialize(std::function<void()> task) {
    return lifecycle().post(std::move(task));
}

void shutDown(std::shared_ptr<HarborHelper> const& current) {
    {
        Lock lock(guard);
        isStopping = true;
        isSynced = false;
    }

    if (auto client = takeSocket()) client->close();
    current->stop();

    Lock lock(guard);
    if (helper == current) helper = nullptr;
}

JsonObject labelsOf() {
    std::pair<wchar_t const*, wchar_t const*> const LABELS[] = {
        {L"copyLink", L"shell.copyLink"},     {L"upToDate", L"harbor.upToDate"},
        {L"syncing", L"harbor.syncing"},      {L"paused", L"harbor.paused"},
        {L"error", L"harbor.syncError"},      {L"offline", L"harbor.offline"},
        {L"offlineMessage", L"harbor.offlineMessage"}, {L"viewOnline", L"harbor.viewOnline"},
        {L"openApp", L"tray.openFileship"},   {L"upload", L"harbor.upload"},
        {L"pause", L"harbor.pause"},          {L"resume", L"harbor.resume"},
        {L"retry", L"harbor.retry"},          {L"freeUpSpace", L"harbor.freeUpSpace"},
        {L"trash", L"harbor.trash"},          {L"storage", L"harbor.manageStorage"},
        {L"settings", L"tray.settings"},
    };

    JsonObject labels;
    for (auto const& [name, key] : LABELS) labels.SetNamedValue(name, str(t(key)));
    return labels;
}

JsonArray propertyDefinitions() {
    std::pair<int, wchar_t const*> const PROPERTIES[] = {
        {PROPERTY_LOCKED, L"harbor.locked"},
        {PROPERTY_EXPIRES, L"harbor.expires"},
        {PROPERTY_SENSITIVE, L"harbor.sensitive"},
        {PROPERTY_VIEWS, L"harbor.views"},
    };

    JsonArray properties;
    for (auto const& [id, key] : PROPERTIES) {
        JsonObject property;
        property.SetNamedValue(L"id", num(id));
        property.SetNamedValue(L"name", str(t(key)));
        properties.Append(property);
    }
    return properties;
}

bool launch() {
    auto folder = harborFolder().wstring();
    auto current = std::make_shared<HarborHelper>();
    auto listening = std::make_shared<std::atomic<bool>>(false);
    auto raw = current.get();

    {
        Lock lock(guard);
        root = folder;
        isStopping = false;
        lastReported.clear();
        detailsAt = 0;
        hasStorageApi = true;
    }

    setStatus([&](Status& value) {
        value.state = L"starting";
        value.root = folder;
        value.error.reset();
    });

    current->onEvent = [listening](HelperEvent const& event) {
        if (*listening) handleHelperEvent(event);
    };

    current->onExit = [raw](std::optional<DWORD> code) {
        std::shared_ptr<SocketIo> client;
        bool isRestarting = false;

        {
            Lock lock(guard);
            if (helper.get() != raw) return;

            helper = nullptr;
            isSynced = false;
            client = socket;
            socket = nullptr;
            isRestarting = !isStopping;
        }

        if (client) client->close();

        if (isRestarting) {
            logWarn(L"The Harbor helper stopped " + (code ? std::to_wstring(*code) : L"null"));
            scheduleRetry(t(L"harbor.restarting"));
        }
    };

    try {
        bool isUnknownRoot = storeString(L"harborRoot") != folder && pathExists(folder) && !isEmptyFolder(folder);

        std::filesystem::create_directories(folder);

        {
            Lock lock(guard);
            helper = current;
        }

        current->start();

        auto endpoint = apiEndpoint();
        auto siteName = storeString(L"siteName");

        JsonObject init;
        init.SetNamedValue(L"root", str(folder));
        init.SetNamedValue(L"syncRootId", str(syncRootKey()));
        init.SetNamedValue(L"displayName", str(siteName.empty() ? L"Fileship" : siteName));
        init.SetNamedValue(L"iconPath", str(helperPath().wstring() + L",0"));
        init.SetNamedValue(L"version", str(appVersion()));
        init.SetNamedValue(L"instanceUrl", str(instanceUrl()));
        init.SetNamedValue(L"userAgent", str(endpoint.userAgent));
        init.SetNamedValue(L"token", str(token()));
        init.SetNamedValue(L"appPath", str(executablePath().wstring()));
        init.SetNamedValue(L"properties", propertyDefinitions());
        init.SetNamedValue(L"labels", labelsOf());
        current->request(L"init", init);

        if (isUnknownRoot && !holdsHarbor(*current)) {
            tryLog(L"Could not unregister Harbor", [&] { current->request(L"unregister"); });
            shutDown(current);
            setStatus([&](Status& value) {
                value.state = L"error";
                value.error = t(L"harbor.notEmpty", {{L"root", folder}});
            });
            return false;
        }

        setStoreValue(L"harborRoot", str(folder));
        *listening = true;
        setHarborRoot(folder);
        return true;
    } catch (...) {
        auto message = describe(std::current_exception()).message;
        logError(L"Could not start Harbor: " + message);
        shutDown(current);
        scheduleRetry(message);
        return false;
    }
}

void startHarborNow() {
    clearRetry();

    {
        Lock lock(guard);
        if (helper && isSynced) return;
    }

    if (!isAllowed()) return;

    if (!isHelperAvailable()) {
        setStatus([](Status& value) {
            value.state = L"error";
            value.error = t(L"harbor.helperMissing");
        });
        return;
    }

    bool needsLaunch = false;
    {
        Lock lock(guard);
        needsLaunch = !helper;
    }

    if (needsLaunch && !launch()) return;

    try {
        reload().get();

        Lock lock(guard);
        if (!socket) connectSocket();
        isSynced = true;
        retries = 0;
    } catch (...) {
        auto error = std::current_exception();
        logError(L"Could not load Harbor: " + describe(error).message);
        if (!isUnauthorized(error)) scheduleRetry(describe(error).message);
    }
}

void stopHarborNow(StopOptions options) {
    clearRetry();

    {
        Lock lock(guard);
        retries = 0;
        isPaused = false;
        timers().clear(storageTimer);
        storageTimer = 0;
    }

    if (options.remove) setHarborRoot({});

    if (options.forget) {
        setStoreValue(L"harborRoot", nul());
        std::error_code error;
        std::filesystem::remove(harborSessionPath(), error);
    }

    std::shared_ptr<HarborHelper> current;
    std::wstring folder;

    {
        Lock lock(guard);
        current = helper;
        folder = root;

        if (!current) {
            if (status.state != L"off") {
                setStatus([](Status& value) {
                    value.state = L"off";
                    value.error.reset();
                });
            }
            return;
        }

        isStopping = true;
        isSynced = false;
        timers().clear(applyTimer);
        applyTimer = 0;
    }

    if (auto client = takeSocket()) client->close();

    queue().drain();

    if (options.remove) {
        std::vector<ScanItem> items;
        try {
            items = current->scan();
        } catch (...) {
        }

        std::set<std::wstring> synced;
        for (auto const& item : items) {
            if (item.identity && !isPendingIdentity(*item.identity) && item.inSync && !item.modified) {
                synced.insert(item.path);
            }
        }

        std::vector<LocalItem> placeholders;
        {
            Lock lock(guard);
            for (auto const& [identity, item] : local.entries()) {
                if (synced.contains(item.path)) placeholders.push_back(item);
            }
        }

        std::ranges::stable_sort(placeholders, [](auto const& a, auto const& b) {
            if (a.isFolder != b.isFolder) return a.isFolder < b.isFolder;
            return std::ranges::count(a.path, L'\\') > std::ranges::count(b.path, L'\\');
        });

        for (auto const& item : placeholders) {
            if (item.isFolder && !isEmptyFolder(std::filesystem::path(folder) / item.path)) continue;
            tryLog(L"Could not remove " + item.path, [&] { current->request(L"remove", pathPayload(item.path)); });
        }

        tryLog(L"Could not unregister", [&] { current->request(L"unregister"); });
    }

    current->stop();

    {
        Lock lock(guard);
        helper = nullptr;
        local.clear();
        sentStates.clear();
        files.clear();
        folders.clear();
        notes.clear();
        links.clear();
        localHashes.clear();
    }

    if (options.remove) {
        std::error_code error;
        if (isEmptyFolder(folder)) std::filesystem::remove(folder, error);
        if (error) logWarn(L"Could not remove the Harbor folder " + folder);
    }

    setStatus([](Status& value) {
        value.state = L"off";
        value.error.reset();
        value.storage.reset();
        value.localSize.reset();
    });
}

std::optional<std::wstring> relativeToRoot(std::wstring const& path) {
    Lock lock(guard);
    if (root.empty()) return std::nullopt;

    auto base = std::filesystem::path(root).lexically_normal().wstring();
    while (!base.empty() && (base.back() == L'\\' || base.back() == L'/')) base.pop_back();
    auto target = std::filesystem::path(path).lexically_normal().wstring();

    if (target.size() <= base.size() + 1 || lower(target.substr(0, base.size())) != lower(base) ||
        (target[base.size()] != L'\\' && target[base.size()] != L'/')) {
        return std::nullopt;
    }

    return target.substr(base.size() + 1);
}

std::optional<std::wstring> identityAt(std::wstring const& path) {
    auto relative = relativeToRoot(path);
    if (!relative) return std::nullopt;

    auto target = lower(*relative);
    Lock lock(guard);

    for (auto const& [identity, item] : local.entries()) {
        if (!item.isFolder && lower(item.path) == target) return identity;
    }

    return std::nullopt;
}

HRESULT CALLBACK dialogCallback(HWND window, UINT notification, WPARAM, LPARAM, LONG_PTR) {
    if (notification == TDN_CREATED) {
        SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetForegroundWindow(window);
    }

    return S_OK;
}

bool confirmPublic(std::wstring const& message, std::wstring const& detail) {
    constexpr int MAKE_PUBLIC = 100;
    constexpr int CANCEL = 101;

    auto makePublic = t(L"harbor.makePublic");
    auto cancel = t(L"harbor.cancel");
    TASKDIALOG_BUTTON buttons[] = {{MAKE_PUBLIC, makePublic.c_str()}, {CANCEL, cancel.c_str()}};

    TASKDIALOGCONFIG config{};
    config.cbSize = sizeof(config);
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
    config.pszWindowTitle = L"Fileship";
    config.pszMainInstruction = message.c_str();
    config.pszContent = detail.c_str();
    config.cButtons = static_cast<UINT>(std::size(buttons));
    config.pButtons = buttons;
    config.nDefaultButton = MAKE_PUBLIC;
    config.pfCallback = dialogCallback;

    int pressed = CANCEL;
    if (FAILED(TaskDialogIndirect(&config, &pressed, nullptr, nullptr))) return false;
    return pressed == MAKE_PUBLIC;
}

std::optional<Links> noteLinksOf(std::wstring const& id) {
    auto note = harborGet(L"/api/notes/" + id);

    if (!boolField(note, L"public").value_or(false)) {
        if (!confirmPublic(t(L"harbor.makeNotePublic"), t(L"harbor.makeNotePublicDescription"))) {
            return std::nullopt;
        }

        JsonObject body;
        body.SetNamedValue(L"public", boolean(true));
        note = harborApi(L"PATCH", L"/api/notes/" + id, body).GetObject();
    }

    auto url = stringField(note, L"publicUrl").value_or(L"");
    return Links{url, url};
}

std::optional<Links> linksOf(std::wstring const& identity) {
    auto id = idOf(identity);

    if (startsWith(identity, L"n:")) return noteLinksOf(id);
    if (!startsWith(identity, L"l:")) return fileLinksOf(id);

    std::wstring url;
    {
        Lock lock(guard);
        if (auto link = linkByCode(id)) url = link->shortUrl;
    }

    return Links{url, url};
}

std::wstring dashboardRouteOf(std::wstring const& identity) {
    if (startsWith(identity, L"n:")) return L"/dashboard/notes/" + idOf(identity);
    if (startsWith(identity, L"l:")) return L"/dashboard/links";
    return L"/dashboard/files?file=" + idOf(identity);
}

std::optional<std::wstring> folderIdOf(std::wstring const& path) {
    auto relative = relativeToRoot(path);
    return relative ? folderIdAt(*relative) : std::nullopt;
}

void copyFolderLink(std::wstring const& id) {
    auto folder = harborGet(L"/api/folders/" + id);

    if (!boolField(folder, L"public").value_or(false)) {
        if (!confirmPublic(t(L"harbor.makeFolderPublic"), t(L"harbor.makeFolderPublicDescription"))) return;

        JsonObject body;
        body.SetNamedValue(L"public", boolean(true));
        harborApi(L"PATCH", L"/api/folders/" + id, body);
        folder = harborGet(L"/api/folders/" + id);
    }

    auto url = stringField(folder, L"publicUrl").value_or(L"");

    writeClipboardText(url);
    notify(t(L"harbor.folderLinkCopied"), url, [url] { openWebUrl(url); });
}

void copyInboxLink(std::wstring const& id) {
    auto inbox = harborGet(L"/api/users/@me/inbox");

    if (stringField(inbox, L"folderId") != id || !boolField(inbox, L"enabled").value_or(false)) {
        notify(t(L"harbor.notInbox"), t(L"harbor.notInboxDescription"),
               [] { openDashboard(L"/dashboard/settings/inbox"); });
        return;
    }

    auto slug = stringField(inbox, L"slug").value_or(L"");
    auto url = instanceUrl() + L"/inbox/" + (slug.empty() ? stringField(inbox, L"id").value_or(L"") : slug);

    writeClipboardText(url);
    notify(t(L"harbor.inboxLinkCopied"), url, [url] { openWebUrl(url); });
}

void runFolderAction(std::wstring const& action, std::wstring const& path) {
    auto id = folderIdOf(path);

    if (!id) {
        notify(t(L"harbor.notOnFileship"), t(L"harbor.notOnFileshipDescription"));
        return;
    }

    try {
        if (action == L"folderLink") {
            copyFolderLink(*id);
        } else {
            copyInboxLink(*id);
        }
    } catch (...) {
        auto message = describe(std::current_exception()).message;
        logWarn(L"Harbor " + action + L" failed: " + message);
        notify(t(L"harbor.linkFailed"), message);
    }
}

void runStatusAction(std::wstring const& action) {
    if (action == L"pause" || action == L"resume") {
        pauseHarbor(action == L"pause");
        return;
    }

    if (action == L"retry") {
        if (isRunning()) resync();
        return;
    }

    try {
        auto freed = freeUpHarborSpace();
        notify(t(L"harbor.freedSpace", {{L"size", formatSize(freed)}}), harborFolder().wstring());
    } catch (...) {
        auto message = describe(std::current_exception()).message;
        logWarn(L"Could not free up space: " + message);
        notify(t(L"app.somethingWentWrong"), message);
    }
}

void runAction(std::wstring const& action, std::vector<std::wstring> const& paths) {
    if (action == L"settings") {
        showMainWindow(L"/settings/harbor");
        return;
    }

    if (action == L"home") {
        showMainWindow();
        return;
    }

    if (action == L"upload") {
        fileship::host::runAction(L"uploadFile");
        return;
    }

    startHarborAndWait();

    if (action == L"pause" || action == L"resume" || action == L"retry" || action == L"freeUpSpace") {
        runStatusAction(action);
        return;
    }

    if (action == L"folderLink" || action == L"inboxLink") {
        runFolderAction(action, paths.empty() ? L"" : paths[0]);
        return;
    }

    std::vector<std::wstring> identities;
    for (auto const& path : paths) {
        auto identity = identityAt(path);
        if (identity && !isPendingIdentity(*identity)) identities.push_back(*identity);
    }

    if (identities.empty()) {
        notify(t(L"harbor.notOnFileship"), t(L"harbor.notOnFileshipDescription"));
        return;
    }

    if (action == L"show") {
        openDashboard(dashboardRouteOf(identities[0]));
        return;
    }

    try {
        std::vector<Links> targets;
        for (auto const& identity : identities) {
            if (auto target = linksOf(identity)) targets.push_back(*target);
        }

        if (targets.empty()) return;

        if (action == L"open") {
            for (auto const& target : targets) openWebUrl(target.open);
            return;
        }

        std::wstring text;
        for (auto const& target : targets) text += (text.empty() ? L"" : L"\n") + target.copy;

        writeClipboardText(text);
        notify(t(L"harbor.linkCopied", {{L"count", std::to_wstring(targets.size())}}), text);
    } catch (...) {
        auto message = describe(std::current_exception()).message;
        logWarn(L"Harbor " + action + L" failed: " + message);
        notify(t(L"harbor.linkFailed"), message);
    }
}

}

std::filesystem::path harborFolder() {
    auto folder = stringField(harborSettings(), L"folder").value_or(L"");
    return folder.empty() ? homeFolder() / L"Fileship" : std::filesystem::path(folder);
}

void startHarbor() {
    serialize(startHarborNow);
}

void startHarborAndWait() {
    serialize(startHarborNow).wait();
}

void stopHarbor(StopOptions options) {
    serialize([options] { stopHarborNow(options); }).wait();
}

JsonObject harborStatus() {
    Lock lock(guard);
    return statusJson();
}

std::optional<std::wstring> harborStorageLabel() {
    Lock lock(guard);
    auto quota = quotaOf();
    return quota ? std::optional(quota->label) : std::nullopt;
}

JsonArray harborActivity() {
    Lock lock(guard);
    return activityJson();
}

void pauseHarbor(bool paused) {
    {
        Lock lock(guard);
        if (!helper || !helper->isRunning() || paused == isPaused) return;
        isPaused = paused;
    }

    if (paused) {
        setStatus([](Status& value) { value.state = L"paused"; });
    } else {
        setStatus([](Status& value) { value.state = L"syncing"; });
        resync();
    }
}

int64_t freeUpHarborSpace() {
    auto current = currentHelper();
    if (!current || !current->isRunning()) return 0;

    auto result = current->request(L"freeUpSpace", JsonObject(), 600000);
    auto freed = static_cast<int64_t>(numberField(result, L"freed").value_or(0));

    addActivity(L"freeUp", formatSize(freed));
    {
        Lock lock(guard);
        detailsAt = 0;
    }
    refreshDetails();
    return freed;
}

void openHarbor() {
    ShellExecuteW(nullptr, L"open", harborFolder().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void runHarborAction(std::wstring const& action, std::vector<std::wstring> const& paths) {
    runDetached([action, paths] { runAction(action, paths); });
}

std::optional<std::filesystem::path> harborPathFor(std::wstring const& fileId) {
    Lock lock(guard);
    auto entry = local.find(fileIdentity(fileId));
    if (status.state != L"ready" || !entry || entry->isFolder) return std::nullopt;
    return std::filesystem::path(root) / entry->path;
}

}

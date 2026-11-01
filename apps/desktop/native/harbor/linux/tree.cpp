#include "harbor.h"

#include <shared/utf8.h>

#include <sys/stat.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace harbor {

namespace {

std::recursive_mutex mutex;
std::unique_ptr<Node> root = [] {
    auto node = std::make_unique<Node>();
    node->isFolder = true;
    return node;
}();
std::atomic<uint64_t> nextBlob = 1;

std::mutex saveMutex;
std::condition_variable saveSignal;
bool isDirty = false;
bool isSaverRunning = false;

std::filesystem::path statePath() {
    return config().cache / "state.json";
}

int64_t fileTimeCount(int64_t nanoseconds) {
    using namespace std::chrono;
    auto time = system_clock::time_point(duration_cast<system_clock::duration>(std::chrono::nanoseconds(nanoseconds)));
    return file_clock::from_sys(time).time_since_epoch().count();
}

bool statBlob(uint64_t blob, struct stat& info) {
    return blob && ::stat(blobPath(blob).c_str(), &info) == 0;
}

void runSaver() {
    std::unique_lock lock(saveMutex);

    for (;;) {
        saveSignal.wait(lock, [] { return isDirty; });
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        lock.lock();
        isDirty = false;
        lock.unlock();
        saveState();
        lock.lock();
    }
}

bool removeEntry(Node* node) {
    bool owned = isOwned(node);

    if (!node->isFolder && !owned && isIgnoredName(node->name)) {
        forget(relativeOf(node));
        dropContent(node);
        detach(node);
        return false;
    }

    bool isKept = !owned || (!node->isFolder && (!node->inSync || isModified(node)));

    if (node->isFolder) {
        std::vector<Node*> children;
        for (auto const& [name, child] : node->children) children.push_back(child.get());
        for (auto* child : children) {
            if (removeEntry(child)) isKept = true;
        }
    }

    if (isKept) {
        if (node->isFolder && owned) node->identity.clear();
        node->inSync = false;
        return true;
    }

    forget(relativeOf(node));
    dropContent(node);
    detach(node);
    return false;
}

Node* require(std::string const& relative) {
    auto node = find(relative);
    if (!node) throw std::runtime_error("The path was not found");
    return node;
}

JsonValue identityValue(Node const* node) {
    return node->identity.empty() ? JsonValue::CreateNullValue() : JsonValue::CreateStringValue(node->identity);
}

}

std::recursive_mutex& treeMutex() {
    return mutex;
}

Node* rootNode() {
    return root.get();
}

Node* find(std::string const& relative) {
    std::lock_guard lock(mutex);
    Node* node = root.get();
    std::stringstream parts(relative);

    for (std::string part; std::getline(parts, part, '/');) {
        if (part.empty()) continue;
        auto found = node->children.find(part);
        if (found == node->children.end()) return nullptr;
        node = found->second.get();
    }

    return node;
}

std::string relativeOf(Node const* node) {
    std::string path;
    for (; node && node->parent; node = node->parent) path = path.empty() ? node->name : node->name + "/" + path;
    return path;
}

std::string parentOf(std::string const& relative) {
    auto slash = relative.find_last_of('/');
    return slash == std::string::npos ? std::string() : relative.substr(0, slash);
}

std::string nameOf(std::string const& relative) {
    auto slash = relative.find_last_of('/');
    return slash == std::string::npos ? relative : relative.substr(slash + 1);
}

std::filesystem::path blobPath(uint64_t blob) {
    return config().cache / "blobs" / std::to_string(blob);
}

uint64_t newBlob() {
    return nextBlob++;
}

int64_t blobMtime(uint64_t blob) {
    struct stat info {};
    if (!statBlob(blob, info)) return 0;
    return static_cast<int64_t>(info.st_mtim.tv_sec) * 1'000'000'000 + info.st_mtim.tv_nsec;
}

int64_t blobSize(uint64_t blob) {
    struct stat info {};
    return statBlob(blob, info) ? static_cast<int64_t>(info.st_size) : 0;
}

bool isModified(Node const* node) {
    return !node->isFolder && node->isHydrated && node->blob && blobMtime(node->blob) != node->baseline;
}

bool isOwned(Node const* node) {
    return !node->identity.empty() && !node->identity.starts_with(L"p:");
}

bool isIgnoredName(std::string const& name) {
    std::string lower(name);
    for (auto& character : lower) character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

    if (lower.starts_with("~$") || lower.starts_with(".~lock") || lower.starts_with(".goutputstream-") ||
        lower.starts_with(".#")) {
        return true;
    }
    for (auto suffix : {".tmp", ".crdownload", ".part", ".swp", ".swx", ".kate-swp", "~"}) {
        if (lower.ends_with(suffix)) return true;
    }
    return lower == "desktop.ini" || lower == "thumbs.db" || lower == ".directory";
}

bool childrenInSync(Node const* node) {
    for (auto const& [name, child] : node->children) {
        if (isIgnoredName(name)) continue;
        if (!isOwned(child.get())) return false;
        if (child->isFolder ? !childrenInSync(child.get()) : !child->inSync || isModified(child.get())) return false;
    }
    return true;
}

std::wstring stampOf(Node const* node) {
    if (node->isHydrated && node->blob) {
        return std::to_wstring(blobSize(node->blob)) + L":" + std::to_wstring(fileTimeCount(blobMtime(node->blob)));
    }
    return std::to_wstring(node->size) + L":" + std::to_wstring(fileTimeCount(node->created * 1'000'000));
}

Node* attach(Node* parent, std::unique_ptr<Node> node) {
    node->parent = parent;
    auto* raw = node.get();
    parent->children[node->name] = std::move(node);
    markDirty();
    return raw;
}

std::unique_ptr<Node> detach(Node* node) {
    auto* parent = node->parent;
    auto found = parent->children.find(node->name);
    auto owned = std::move(found->second);
    parent->children.erase(found);
    owned->parent = nullptr;
    markDirty();
    return owned;
}

void dropContent(Node* node) {
    if (node->blob) {
        std::error_code error;
        std::filesystem::remove(blobPath(node->blob), error);
    }
    node->blob = 0;
    node->isHydrated = false;
    node->baseline = 0;
    markDirty();
}

void forEachNode(Node* node, std::function<void(Node*)> const& visit) {
    for (auto const& [name, child] : node->children) {
        visit(child.get());
        if (child->isFolder) forEachNode(child.get(), visit);
    }
}

void markDirty() {
    std::lock_guard lock(saveMutex);
    isDirty = true;
    if (!isSaverRunning) {
        isSaverRunning = true;
        std::thread(runSaver).detach();
    }
    saveSignal.notify_one();
}

void saveState() {
    JsonArray nodes;

    {
        std::lock_guard lock(mutex);
        forEachNode(root.get(), [&](Node* node) {
            JsonObject item;
            item.SetNamedValue(L"path", JsonValue::CreateStringValue(fileship::toWide(relativeOf(node))));
            item.SetNamedValue(L"isFolder", JsonValue::CreateBooleanValue(node->isFolder));
            item.SetNamedValue(L"identity", JsonValue::CreateStringValue(node->identity));
            item.SetNamedValue(L"inSync", JsonValue::CreateBooleanValue(node->inSync));
            item.SetNamedValue(L"hydrated", JsonValue::CreateBooleanValue(node->isHydrated));
            item.SetNamedValue(L"pinned", JsonValue::CreateBooleanValue(node->isPinned));
            item.SetNamedValue(L"size", JsonValue::CreateNumberValue(static_cast<double>(node->size)));
            item.SetNamedValue(L"created", JsonValue::CreateNumberValue(static_cast<double>(node->created)));
            item.SetNamedValue(L"baseline", JsonValue::CreateStringValue(std::to_wstring(node->baseline)));
            item.SetNamedValue(L"blob", JsonValue::CreateNumberValue(static_cast<double>(node->blob)));
            nodes.Append(item);
        });
    }

    JsonObject state;
    state.SetNamedValue(L"nextBlob", JsonValue::CreateNumberValue(static_cast<double>(nextBlob.load())));
    state.SetNamedValue(L"nodes", nodes);

    auto path = statePath();
    auto temporary = path;
    temporary += ".tmp";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        file << fileship::toUtf8(state.Stringify());
        if (!file) return log("Could not save the Harbor state");
    }
    std::error_code error;
    std::filesystem::rename(temporary, path, error);
}

void loadState() {
    std::ifstream file(statePath(), std::ios::binary);
    if (!file) return;

    std::stringstream content;
    content << file.rdbuf();
    JsonObject state;
    if (!JsonObject::TryParse(fileship::toWide(content.str()), state)) return log("The Harbor state could not be read");

    std::lock_guard lock(mutex);
    nextBlob = static_cast<uint64_t>(state.GetNamedNumber(L"nextBlob", 1));

    for (auto const& value : state.GetNamedArray(L"nodes", JsonArray())) {
        auto item = value.GetObject();
        auto path = fileship::toUtf8(item.GetNamedString(L"path"));
        auto parent = find(parentOf(path));
        if (!parent || !parent->isFolder) continue;

        auto node = std::make_unique<Node>();
        node->name = nameOf(path);
        node->isFolder = item.GetNamedBoolean(L"isFolder", false);
        node->identity = item.GetNamedString(L"identity", L"");
        node->inSync = item.GetNamedBoolean(L"inSync", false);
        node->isHydrated = item.GetNamedBoolean(L"hydrated", false);
        node->isPinned = item.GetNamedBoolean(L"pinned", false);
        node->size = static_cast<int64_t>(item.GetNamedNumber(L"size", 0));
        node->created = static_cast<int64_t>(item.GetNamedNumber(L"created", 0));
        node->baseline = std::stoll(fileship::toUtf8(item.GetNamedString(L"baseline", L"0")));
        node->blob = static_cast<uint64_t>(item.GetNamedNumber(L"blob", 0));

        std::error_code error;
        if (node->blob && !std::filesystem::exists(blobPath(node->blob), error)) {
            node->blob = 0;
            node->isHydrated = false;
        }
        attach(parent, std::move(node));
    }
}

void createPlaceholders(std::vector<PlaceholderItem> const& items) {
    for (auto const& item : items) {
        std::vector<std::wstring> downloads;

        {
            std::lock_guard lock(mutex);
            auto parent = find(parentOf(item.path));
            if (!parent || !parent->isFolder) throw std::runtime_error("The parent folder was not found");

            auto name = nameOf(item.path);
            auto existing = parent->children.find(name);

            if (existing != parent->children.end()) {
                auto* node = existing->second.get();
                if (node->identity == item.identity) {
                    markInSync(item.path);
                    continue;
                }
                if (item.isFolder && node->isFolder && node->identity.empty()) {
                    adoptPath(item.path, item.identity, false);
                    continue;
                }
                throw std::runtime_error("The item already exists");
            }

            auto node = std::make_unique<Node>();
            node->name = name;
            node->isFolder = item.isFolder;
            node->identity = item.identity;
            node->inSync = true;
            node->size = item.isFolder ? 0 : item.size;
            node->created = item.created;
            node->isPinned = parent->isPinned;
            if (node->isPinned && !node->isFolder) downloads.push_back(node->identity);
            attach(parent, std::move(node));
        }

        for (auto const& identity : downloads) startDownload(identity);
    }
}

void movePlaceholder(std::string const& from, std::string const& to) {
    std::lock_guard lock(mutex);
    auto node = require(from);
    auto parent = find(parentOf(to));
    if (!parent || !parent->isFolder) throw std::runtime_error("The target folder was not found");
    if (parent->children.contains(nameOf(to))) throw std::runtime_error("The target already exists");

    forget(from);
    auto owned = detach(node);
    owned->name = nameOf(to);
    attach(parent, std::move(owned));
    markInSync(to);
}

void updatePlaceholder(std::string const& path, std::optional<std::wstring> const& identity) {
    std::lock_guard lock(mutex);
    auto node = require(path);
    if (identity) node->identity = *identity;
    if (node->isHydrated) node->baseline = blobMtime(node->blob);
    node->inSync = true;
    markDirty();
}

bool refreshPlaceholder(std::string const& path, int64_t size) {
    std::wstring download;

    {
        std::lock_guard lock(mutex);
        auto node = find(path);
        if (!node || node->isFolder || node->identity.empty() || isModified(node) || node->openCount) return false;

        dropContent(node);
        node->size = size;
        node->inSync = true;
        if (node->isPinned) download = node->identity;
    }

    if (!download.empty()) startDownload(download);
    return true;
}

bool markInSync(std::string const& path) {
    std::lock_guard lock(mutex);
    auto node = find(path);
    if (!node) return false;
    if (!node->parent) return childrenInSync(node);
    if (!isOwned(node)) return false;
    if (node->isFolder) return childrenInSync(node);
    if (isModified(node)) return false;

    node->inSync = true;
    markDirty();
    return true;
}

bool removePath(std::string const& path) {
    std::lock_guard lock(mutex);
    auto node = find(path);
    if (!node || !node->parent) return false;
    return removeEntry(node);
}

void adoptPath(std::string const& path, std::wstring const& identity, bool isChanged) {
    std::lock_guard lock(mutex);
    auto node = require(path);
    bool isPlaceholder = !node->identity.empty();

    if (isChanged) {
        if (!isPlaceholder) return;
        node->identity = identity;
        node->inSync = false;
        markDirty();
        return;
    }

    node->identity = identity;
    node->inSync = true;
    if (node->isHydrated) node->baseline = blobMtime(node->blob);
    markDirty();
}

JsonArray unsyncedChildren(std::string const& path) {
    std::lock_guard lock(mutex);
    JsonArray items;
    auto node = find(path);
    if (!node || !node->isFolder) return items;

    for (auto const& [name, child] : node->children) {
        if (isIgnoredName(name)) continue;

        bool isSynced = isOwned(child.get()) && (child->isFolder || (child->inSync && !isModified(child.get())));
        if (isSynced || (!child->isFolder && child->openCount)) continue;

        JsonObject item;
        item.SetNamedValue(L"path", JsonValue::CreateStringValue(fileship::toWide(relativeOf(child.get()))));
        item.SetNamedValue(L"isFolder", JsonValue::CreateBooleanValue(child->isFolder));
        item.SetNamedValue(L"identity", identityValue(child.get()));
        item.SetNamedValue(L"size", JsonValue::CreateNumberValue(static_cast<double>(
                                        child->isHydrated ? blobSize(child->blob) : child->size)));
        items.Append(item);
    }

    return items;
}

std::wstring preparePath(std::string const& path, std::wstring const& identity) {
    std::lock_guard lock(mutex);
    auto node = require(path);
    node->inSync = false;
    if (node->identity.empty()) node->identity = identity;
    markDirty();
    return node->identity;
}

JsonArray scanRoot() {
    std::lock_guard lock(mutex);
    JsonArray items;

    forEachNode(root.get(), [&](Node* node) {
        bool isSynced = node->isFolder ? isOwned(node) && childrenInSync(node) : node->inSync;

        JsonObject item;
        item.SetNamedValue(L"path", JsonValue::CreateStringValue(fileship::toWide(relativeOf(node))));
        item.SetNamedValue(L"isFolder", JsonValue::CreateBooleanValue(node->isFolder));
        item.SetNamedValue(L"identity", identityValue(node));
        item.SetNamedValue(L"inSync", JsonValue::CreateBooleanValue(isSynced));
        item.SetNamedValue(L"modified", JsonValue::CreateBooleanValue(!node->identity.empty() && isModified(node)));
        item.SetNamedValue(L"hydrated", JsonValue::CreateBooleanValue(node->isFolder || node->isHydrated));
        item.SetNamedValue(L"size", JsonValue::CreateNumberValue(static_cast<double>(
                                        node->isFolder ? 0 : node->isHydrated ? blobSize(node->blob) : node->size)));
        items.Append(item);
    });

    return items;
}

int64_t localSize() {
    std::lock_guard lock(mutex);
    int64_t total = 0;
    forEachNode(root.get(), [&](Node* node) {
        if (node->isHydrated) total += blobSize(node->blob);
    });
    return total;
}

int64_t freeUpSpace(Node* folder) {
    std::lock_guard lock(mutex);
    int64_t freed = 0;

    auto release = [&](Node* node) {
        if (node->isFolder || !node->isHydrated || node->openCount || node->isPinned || !node->inSync) return;
        if (!node->identity.starts_with(L"f:") && !node->identity.starts_with(L"n:")) return;
        if (isModified(node)) return;

        freed += blobSize(node->blob);
        dropContent(node);
    };

    if (folder && !folder->isFolder) {
        release(folder);
    } else {
        forEachNode(folder ? folder : root.get(), release);
    }

    return freed;
}

void materialize() {
    std::lock_guard lock(mutex);

    forEachNode(root.get(), [&](Node* node) {
        auto target = config().root / relativeOf(node);
        std::error_code error;

        if (node->isFolder) {
            if (!isOwned(node)) std::filesystem::create_directories(target, error);
            return;
        }

        bool isKept = !isOwned(node) || !node->inSync || isModified(node);
        if (!isKept || !node->isHydrated || !node->blob || std::filesystem::exists(target, error)) return;

        std::filesystem::create_directories(target.parent_path(), error);
        std::filesystem::copy_file(blobPath(node->blob), target, error);
        if (error) log("Could not keep " + relativeOf(node) + ": " + error.message());
    });
}

}

#include "common.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <stdexcept>

namespace harbor {

namespace {

namespace fs = std::filesystem;

constexpr std::array<char const*, 4> TEMPORARY_ENDINGS = {".tmp", ".part", ".crdownload", ".swp"};

std::optional<std::string> attribute(std::string const& absolute, char const* name) {
    char buffer[512];
    ssize_t size = lgetxattr(absolute.c_str(), name, buffer, sizeof(buffer));
    if (size < 0) return std::nullopt;
    return std::string(buffer, static_cast<size_t>(size));
}

bool isIgnoredName(std::string const& name) {
    if (name.starts_with(".") || name.ends_with("~")) return true;

    std::string lower(name);
    std::ranges::transform(lower, lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return std::ranges::any_of(TEMPORARY_ENDINGS, [&](char const* ending) { return lower.ends_with(ending); });
}

std::string childOf(std::string const& folder, std::string const& name) {
    return folder.empty() ? name : folder + "/" + name;
}

std::vector<std::string> childrenOf(std::string const& relative) {
    std::vector<std::string> names;
    std::error_code error;

    for (fs::directory_iterator iterator(backingOf(relative), error), end; !error && iterator != end;
         iterator.increment(error)) {
        names.push_back(iterator->path().filename().string());
    }

    if (error) throw std::runtime_error(error.message());
    std::ranges::sort(names);
    return names;
}

void setTime(std::string const& absolute, int64_t milliseconds) {
    timespec time{milliseconds / 1000, (milliseconds % 1000) * 1000000};
    timespec times[2] = {time, time};
    utimensat(AT_FDCWD, absolute.c_str(), times, AT_SYMLINK_NOFOLLOW);
}

void requireFolder(std::string const& relative) {
    auto entry = entryOf(relative);
    if (!entry.exists || !entry.isFolder) throw std::runtime_error("The folder " + relative + " does not exist");
}

void dehydrate(std::string const& relative, int64_t size) {
    auto absolute = backingOf(relative);
    struct stat info {};
    lstat(absolute.c_str(), &info);

    if (truncate(absolute.c_str(), 0) || truncate(absolute.c_str(), size)) {
        throw std::runtime_error("Could not free up " + relative);
    }

    timespec times[2] = {info.st_atim, info.st_mtim};
    utimensat(AT_FDCWD, absolute.c_str(), times, AT_SYMLINK_NOFOLLOW);
    forgetBlocks(relative);

    if (size) {
        setAttribute(relative, REMOTE, std::to_string(size));
        clearAttribute(relative, BASE);
    } else {
        clearAttribute(relative, REMOTE);
        setAttribute(relative, BASE, stampOf(absolute));
    }
}

bool childrenInSync(std::string const& relative) {
    for (auto const& name : childrenOf(relative)) {
        if (isIgnoredName(name)) continue;

        auto entry = entryOf(childOf(relative, name));
        if (!entry.isOwned() || !entry.inSync) return false;
    }

    return true;
}

void claimExisting(Item const& item, Entry const& existing) {
    if (existing.identity == item.identity) {
        markInSync(item.path);
        return;
    }

    if (item.isFolder && !existing.identity && existing.isFolder) {
        adoptPath(item.path, item.identity, false);
        return;
    }

    throw std::runtime_error(item.path + " already exists");
}

bool removeEntry(std::string const& relative) {
    auto entry = entryOf(relative);
    if (!entry.exists) return false;

    auto absolute = backingOf(relative);
    std::error_code error;

    if (!entry.isFolder && !entry.isOwned() && isIgnored(relative)) {
        fs::remove(absolute, error);
        return false;
    }

    bool isKept = !entry.isOwned() || (!entry.isFolder && (!entry.inSync || isModified(relative, entry)));

    if (entry.isFolder) {
        for (auto const& name : childrenOf(relative)) {
            if (removeEntry(childOf(relative, name))) isKept = true;
        }
    }

    if (isKept) {
        if (entry.isFolder && entry.isOwned()) forgetEntry(relative);
        return true;
    }

    forgetBlocks(relative);
    fs::remove(absolute, error);
    if (error) throw std::runtime_error(error.message());
    return false;
}

JsonObject describe(std::string const& relative, Entry const& entry) {
    JsonObject item;
    item.SetNamedValue(L"path", text(relative));
    item.SetNamedValue(L"isFolder", JsonValue::CreateBooleanValue(entry.isFolder));
    item.SetNamedValue(L"identity", entry.identity ? text(*entry.identity) : JsonValue::CreateNullValue());
    item.SetNamedValue(L"size", number(entry.isFolder ? 0 : entry.size));
    return item;
}

template <typename Visit>
void walk(std::string const& folder, Visit const& visit) {
    for (auto const& name : childrenOf(folder)) {
        if (isIgnoredName(name)) continue;

        auto relative = childOf(folder, name);
        auto entry = entryOf(relative);
        if (!entry.exists) continue;

        bool isListed = true;

        if (entry.isFolder) {
            try {
                walk(relative, visit);
            } catch (std::exception const&) {
                isListed = false;
            }
        }

        visit(relative, entry, isListed);
    }
}

}

std::string backingOf(std::string const& relative) {
    return relative.empty() ? config().backing : config().backing + "/" + relative;
}

std::string parentOf(std::string const& relative) {
    auto slash = relative.find_last_of('/');
    return slash == std::string::npos ? std::string() : relative.substr(0, slash);
}

bool isIgnored(std::string const& relative) {
    size_t start = 0;

    while (start <= relative.size()) {
        auto end = relative.find('/', start);
        if (end == std::string::npos) end = relative.size();
        if (isIgnoredName(relative.substr(start, end - start))) return true;
        start = end + 1;
    }

    return false;
}

Entry entryOf(std::string const& relative) {
    auto absolute = backingOf(relative);
    struct stat info {};
    Entry entry;

    if (lstat(absolute.c_str(), &info)) return entry;

    entry.exists = true;
    entry.isFolder = S_ISDIR(info.st_mode);
    entry.size = info.st_size;
    entry.identity = attribute(absolute, IDENTITY);
    entry.inSync = attribute(absolute, IN_SYNC) == "1";
    entry.base = attribute(absolute, BASE).value_or("");
    if (auto remote = attribute(absolute, REMOTE)) entry.remote = std::stoll(*remote);
    return entry;
}

std::string stampOf(std::string const& absolute) {
    struct stat info {};
    if (stat(absolute.c_str(), &info)) return {};

    auto time = static_cast<int64_t>(info.st_mtim.tv_sec) * 1000000000 + info.st_mtim.tv_nsec;
    return std::to_string(info.st_size) + ":" + std::to_string(time);
}

bool isModified(std::string const& relative, Entry const& entry) {
    return entry.identity && !entry.isFolder && !entry.remote && stampOf(backingOf(relative)) != entry.base;
}

void setAttribute(std::string const& relative, char const* name, std::string const& value) {
    if (lsetxattr(backingOf(relative).c_str(), name, value.data(), value.size(), 0)) {
        throw std::runtime_error("Could not mark " + relative);
    }
}

void clearAttribute(std::string const& relative, char const* name) {
    lremovexattr(backingOf(relative).c_str(), name);
}

void forgetEntry(std::string const& relative) {
    for (auto name : {IDENTITY, IN_SYNC, REMOTE, BASE}) clearAttribute(relative, name);
}

void copyIdentity(std::string const& from, std::string const& to) {
    auto entry = entryOf(from);
    setAttribute(to, IDENTITY, *entry.identity);
    setAttribute(to, BASE, entry.base);
    clearAttribute(to, IN_SYNC);
    clearAttribute(to, REMOTE);
}

void createPlaceholders(std::vector<Item> const& items) {
    for (auto const& item : items) {
        requireFolder(parentOf(item.path));

        auto absolute = backingOf(item.path);
        auto existing = entryOf(item.path);

        if (existing.exists) {
            claimExisting(item, existing);
            continue;
        }

        if (item.isFolder) {
            if (mkdir(absolute.c_str(), 0755)) throw std::runtime_error("Could not create " + item.path);
        } else {
            int file = open(absolute.c_str(), O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC, 0644);
            if (file < 0 || ftruncate(file, item.size)) {
                if (file >= 0) close(file);
                throw std::runtime_error("Could not create " + item.path);
            }
            close(file);
        }

        setTime(absolute, item.created);
        setAttribute(item.path, IDENTITY, item.identity);
        setAttribute(item.path, IN_SYNC, "1");

        if (item.isFolder) continue;

        if (item.size) {
            setAttribute(item.path, REMOTE, std::to_string(item.size));
            queueThumbnail(item.path);
        } else {
            setAttribute(item.path, BASE, stampOf(absolute));
        }
    }
}

void movePlaceholder(std::string const& from, std::string const& to) {
    requireFolder(parentOf(to));

    if (rename(backingOf(from).c_str(), backingOf(to).c_str())) {
        throw std::runtime_error("Could not move " + from + " to " + to);
    }

    forgetBlocks(from);
    markInSync(to);
    queueThumbnail(to);
}

void updatePlaceholder(std::string const& path, std::optional<std::string> const& identity) {
    auto entry = entryOf(path);
    if (!entry.exists) throw std::runtime_error(path + " does not exist");

    if (identity) setAttribute(path, IDENTITY, *identity);
    if (!entry.isFolder && !entry.remote) setAttribute(path, BASE, stampOf(backingOf(path)));
    setAttribute(path, IN_SYNC, "1");
}

bool refreshPlaceholder(std::string const& path, int64_t size) {
    auto entry = entryOf(path);
    if (!entry.exists || !entry.identity || isModified(path, entry)) return false;

    dehydrate(path, size);
    setAttribute(path, IN_SYNC, "1");
    return true;
}

bool removePath(std::string const& path) {
    return removeEntry(path);
}

void adoptPath(std::string const& path, std::string const& identity, bool isChanged) {
    auto entry = entryOf(path);
    if (!entry.exists) throw std::runtime_error(path + " does not exist");

    if (isChanged) {
        if (!entry.identity) return;
        setAttribute(path, IDENTITY, identity);
        clearAttribute(path, IN_SYNC);
        return;
    }

    updatePlaceholder(path, identity);
}

JsonArray unsyncedChildren(std::string const& path) {
    JsonArray items;
    if (!entryOf(path).isFolder) return items;

    for (auto const& name : childrenOf(path)) {
        if (isIgnoredName(name)) continue;

        auto relative = childOf(path, name);
        auto entry = entryOf(relative);
        if (!entry.exists) continue;

        bool isSynced = entry.isOwned() && (entry.isFolder || (entry.inSync && !isModified(relative, entry)));
        if (isSynced || (!entry.isFolder && isOpenForWriting(relative))) continue;

        items.Append(describe(relative, entry));
    }

    return items;
}

std::string preparePath(std::string const& path, std::string const& identity) {
    auto entry = entryOf(path);
    if (!entry.exists) throw std::runtime_error(path + " does not exist");

    clearAttribute(path, IN_SYNC);
    if (entry.identity && !entry.identity->empty()) return *entry.identity;

    setAttribute(path, IDENTITY, identity);
    return identity;
}

bool markInSync(std::string const& path) {
    try {
        if (path.empty()) return childrenInSync(path);

        auto entry = entryOf(path);
        if (!entry.isOwned()) return false;
        if (entry.isFolder ? !childrenInSync(path) : isModified(path, entry)) return false;

        setAttribute(path, IN_SYNC, "1");
        return true;
    } catch (std::exception const&) {
        return false;
    }
}

JsonArray scanRoot() {
    JsonArray items;

    walk("", [&](std::string const& relative, Entry const& entry, bool isListed) {
        auto item = describe(relative, entry);
        if (!isListed) item.SetNamedValue(L"unreadable", JsonValue::CreateBooleanValue(true));
        item.SetNamedValue(L"inSync", JsonValue::CreateBooleanValue(entry.inSync));
        item.SetNamedValue(L"modified", JsonValue::CreateBooleanValue(isModified(relative, entry)));
        item.SetNamedValue(L"hydrated", JsonValue::CreateBooleanValue(!entry.remote));
        items.Append(item);
    });

    return items;
}

int64_t localSize() {
    int64_t total = 0;

    walk("", [&](std::string const&, Entry const& entry, bool) {
        if (!entry.isFolder && !entry.remote) total += entry.size;
    });

    return total;
}

int64_t freeUpSpace() {
    int64_t freed = 0;

    walk("", [&](std::string const& relative, Entry const& entry, bool) {
        if (entry.isFolder || entry.remote || !entry.size || !entry.inSync || !entry.identity) return;
        if (!entry.identity->starts_with("f:") && !entry.identity->starts_with("n:")) return;
        if (isModified(relative, entry) || isOpenForWriting(relative)) return;

        try {
            dehydrate(relative, entry.size);
            freed += entry.size;
        } catch (std::exception const& error) {
            log(error.what());
        }
    });

    return freed;
}

void finishHydration(std::string const& relative) {
    clearAttribute(relative, REMOTE);
    setAttribute(relative, BASE, stampOf(backingOf(relative)));
}

void releaseLocal() {
    std::error_code error;
    fs::path backing = config().backing;

    for (fs::recursive_directory_iterator iterator(backing, error), end; !error && iterator != end;
         iterator.increment(error)) {
        auto relative = fs::relative(iterator->path(), backing).string();
        auto target = fs::path(config().root) / relative;
        auto entry = entryOf(relative);
        std::error_code ignored;

        if (entry.isFolder) {
            fs::create_directories(target, ignored);
        } else if (!entry.remote) {
            forgetEntry(relative);
            fs::create_directories(target.parent_path(), ignored);
            fs::rename(iterator->path(), target, ignored);
            if (ignored && fs::copy_file(iterator->path(), target, ignored)) fs::remove(iterator->path(), ignored);
        }
    }

    fs::remove_all(backing, error);
}

}

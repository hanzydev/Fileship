#define FUSE_USE_VERSION 31

#include "common.h"

#include <fuse3/fuse.h>

#include <dirent.h>
#include <fcntl.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>

extern char** environ;

namespace harbor {

namespace {

using Clock = std::chrono::steady_clock;

constexpr auto SETTLE_TIME = std::chrono::milliseconds(1000);

struct Handle {
    int descriptor = -1;
    bool isWriter = false;
};

fuse* session = nullptr;
std::thread loop;

std::mutex stateMutex;
std::condition_variable stateSignal;
std::map<std::string, int> writers;
std::map<std::string, Clock::time_point> changes;
std::map<std::string, std::string> reported;
bool isStopping = false;
std::thread reporter;

std::string relativeOf(char const* path) {
    return path + 1;
}

Handle* handleOf(fuse_file_info* info) {
    return reinterpret_cast<Handle*>(info->fh);
}

bool isWriting(int flags) {
    return (flags & O_ACCMODE) != O_RDONLY;
}

void report(std::string const& type, std::string const& relative, JsonObject message) {
    auto stamp = stampOf(backingOf(relative));

    {
        std::lock_guard lock(stateMutex);
        auto& last = reported[type + "|" + relative];
        if (last == stamp) return;
        last = stamp;
    }

    message.SetNamedValue(L"path", text(relative));
    sendEvent(type, message);
}

void inspect(std::string const& relative) {
    auto entry = entryOf(relative);
    if (!entry.exists || isIgnored(relative)) return;

    JsonObject message;
    message.SetNamedValue(L"size", number(entry.isFolder ? 0 : entry.size));

    if (!entry.identity || entry.identity->empty()) {
        message.SetNamedValue(L"isFolder", JsonValue::CreateBooleanValue(entry.isFolder));
        report("localNew", relative, message);
    } else if (isModified(relative, entry)) {
        message.SetNamedValue(L"identity", text(*entry.identity));
        report("localModified", relative, message);
    }
}

void runReporter() {
    std::unique_lock lock(stateMutex);

    while (!isStopping) {
        auto now = Clock::now();
        auto next = now + std::chrono::hours(1);
        std::vector<std::string> due;

        for (auto iterator = changes.begin(); iterator != changes.end();) {
            if (writers.contains(iterator->first)) {
                ++iterator;
            } else if (iterator->second <= now) {
                due.push_back(iterator->first);
                iterator = changes.erase(iterator);
            } else {
                next = std::min(next, iterator->second);
                ++iterator;
            }
        }

        if (due.empty()) {
            stateSignal.wait_until(lock, next);
            continue;
        }

        lock.unlock();
        for (auto const& relative : due) {
            try {
                inspect(relative);
            } catch (std::exception const& error) {
                log(error.what());
            }
        }
        lock.lock();
    }
}

void moveState(std::string const& from, std::string const& to) {
    std::lock_guard lock(stateMutex);

    if (auto found = writers.extract(from)) {
        found.key() = to;
        writers.insert(std::move(found));
    }

    changes.erase(from);
    std::erase_if(reported, [&](auto const& item) { return item.first.ends_with("|" + from); });
}

void sendDelete(std::string const& relative, Entry const& entry) {
    if (!entry.isOwned() || isIgnored(relative)) return;

    JsonObject message;
    message.SetNamedValue(L"identity", text(*entry.identity));
    message.SetNamedValue(L"path", text(relative));
    message.SetNamedValue(L"isFolder", JsonValue::CreateBooleanValue(entry.isFolder));
    sendEvent("delete", message);
}

void forgetTree(std::string const& relative) {
    std::error_code error;
    std::filesystem::path backing = config().backing;
    forgetEntry(relative);

    for (std::filesystem::recursive_directory_iterator iterator(backingOf(relative), error), end;
         !error && iterator != end; iterator.increment(error)) {
        forgetEntry(std::filesystem::relative(iterator->path(), backing).string());
    }
}

int getAttributes(char const* path, struct stat* info, fuse_file_info*) {
    return lstat(backingOf(relativeOf(path)).c_str(), info) ? -errno : 0;
}

int readFolder(char const* path, void* buffer, fuse_fill_dir_t fill, off_t, fuse_file_info*, fuse_readdir_flags) {
    DIR* folder = opendir(backingOf(relativeOf(path)).c_str());
    if (!folder) return -errno;

    while (auto entry = readdir(folder)) fill(buffer, entry->d_name, nullptr, 0, static_cast<fuse_fill_dir_flags>(0));

    closedir(folder);
    return 0;
}

int openFile(char const* path, fuse_file_info* info) {
    auto relative = relativeOf(path);
    int descriptor = open(backingOf(relative).c_str(), (info->flags & ~(O_CREAT | O_EXCL | O_TRUNC)) | O_CLOEXEC);
    if (descriptor < 0) return -errno;

    auto handle = new Handle{descriptor, isWriting(info->flags)};
    info->fh = reinterpret_cast<uint64_t>(handle);

    if (handle->isWriter) {
        std::lock_guard lock(stateMutex);
        writers[relative]++;
    }

    return 0;
}

int createFile(char const* path, mode_t mode, fuse_file_info* info) {
    auto relative = relativeOf(path);
    int descriptor = open(backingOf(relative).c_str(), info->flags | O_CREAT | O_CLOEXEC, mode);
    if (descriptor < 0) return -errno;

    info->fh = reinterpret_cast<uint64_t>(new Handle{descriptor, true});

    std::lock_guard lock(stateMutex);
    writers[relative]++;
    changes[relative] = Clock::now() + SETTLE_TIME;
    return 0;
}

int readFile(char const* path, char* buffer, size_t size, off_t offset, fuse_file_info* info) {
    try {
        fetchRange(relativeOf(path), offset, static_cast<int64_t>(size));
    } catch (std::exception const& error) {
        log(error.what());
        return -EIO;
    }

    ssize_t count = pread(handleOf(info)->descriptor, buffer, size, offset);
    return count < 0 ? -errno : static_cast<int>(count);
}

int writeFile(char const* path, char const* buffer, size_t size, off_t offset, fuse_file_info* info) {
    auto relative = relativeOf(path);

    try {
        fetchAll(relative);
    } catch (std::exception const& error) {
        log(error.what());
        return -EIO;
    }

    ssize_t count = pwrite(handleOf(info)->descriptor, buffer, size, offset);
    if (count < 0) return -errno;

    scheduleChange(relative);
    return static_cast<int>(count);
}

int truncateFile(char const* path, off_t size, fuse_file_info*) {
    auto relative = relativeOf(path);

    try {
        if (size) {
            fetchAll(relative);
        } else if (entryOf(relative).remote) {
            forgetBlocks(relative);
            clearAttribute(relative, REMOTE);
        }
    } catch (std::exception const& error) {
        log(error.what());
        return -EIO;
    }

    if (truncate(backingOf(relative).c_str(), size)) return -errno;

    scheduleChange(relative);
    return 0;
}

int releaseFile(char const* path, fuse_file_info* info) {
    std::unique_ptr<Handle> handle(handleOf(info));
    close(handle->descriptor);

    if (handle->isWriter && path) {
        auto relative = relativeOf(path);
        std::lock_guard lock(stateMutex);

        if (auto found = writers.find(relative); found != writers.end() && --found->second == 0) {
            writers.erase(found);
            if (auto change = changes.find(relative); change != changes.end()) change->second = Clock::now() + SETTLE_TIME;
            stateSignal.notify_all();
        }
    }

    return 0;
}

int syncFile(char const*, int, fuse_file_info* info) {
    return fsync(handleOf(info)->descriptor) ? -errno : 0;
}

int removeFile(char const* path) {
    auto relative = relativeOf(path);
    auto entry = entryOf(relative);
    if (unlink(backingOf(relative).c_str())) return -errno;

    forgetBlocks(relative);
    sendDelete(relative, entry);
    return 0;
}

int removeFolder(char const* path) {
    auto relative = relativeOf(path);
    auto entry = entryOf(relative);
    if (rmdir(backingOf(relative).c_str())) return -errno;

    sendDelete(relative, entry);
    return 0;
}

int makeFolder(char const* path, mode_t mode) {
    auto relative = relativeOf(path);
    if (mkdir(backingOf(relative).c_str(), mode)) return -errno;

    scheduleChange(relative);
    return 0;
}

int renameEntry(char const* fromPath, char const* toPath, unsigned int flags) {
    auto from = relativeOf(fromPath);
    auto to = relativeOf(toPath);
    auto source = entryOf(from);
    auto target = entryOf(to);
    bool isTracked = source.identity && !source.identity->empty() && !isIgnored(from);
    bool isInScope = !isIgnored(to);

    if (isTracked) {
        JsonObject message;
        message.SetNamedValue(L"identity", text(*source.identity));
        message.SetNamedValue(L"from", text(from));
        message.SetNamedValue(L"to", isInScope ? text(to) : JsonValue::CreateNullValue());
        message.SetNamedValue(L"isFolder", JsonValue::CreateBooleanValue(source.isFolder));
        if (!askHost("rename", message)) return -EACCES;

        if (!isInScope && source.remote) {
            forgetBlocks(from);
            return unlink(backingOf(from).c_str()) ? -errno : 0;
        }
    }

    bool isReplacing = !isTracked && isInScope && target.exists && target.identity && !target.isFolder && !source.isFolder;
    if (isReplacing) copyIdentity(to, from);

    if (renameat2(AT_FDCWD, backingOf(from).c_str(), AT_FDCWD, backingOf(to).c_str(), flags)) {
        int error = errno;
        if (isReplacing) forgetEntry(from);
        return -error;
    }

    forgetBlocks(from);
    forgetBlocks(to);
    moveState(from, to);

    if (isTracked && !isInScope) forgetTree(to);
    if (isTracked && isInScope) markInSync(to);
    if (!isTracked && isInScope) scheduleChange(to);
    return 0;
}

int setTimes(char const* path, timespec const times[2], fuse_file_info*) {
    auto relative = relativeOf(path);
    if (utimensat(AT_FDCWD, backingOf(relative).c_str(), times, AT_SYMLINK_NOFOLLOW)) return -errno;

    auto entry = entryOf(relative);
    if (!entry.isFolder && !entry.remote) scheduleChange(relative);
    return 0;
}

int setMode(char const* path, mode_t mode, fuse_file_info*) {
    return chmod(backingOf(relativeOf(path)).c_str(), mode) ? -errno : 0;
}

int checkAccess(char const* path, int mask) {
    return access(backingOf(relativeOf(path)).c_str(), mask) ? -errno : 0;
}

int fileSystem(char const*, struct statvfs* info) {
    return statvfs(config().backing.c_str(), info) ? -errno : 0;
}

void* start(fuse_conn_info*, fuse_config* settings) {
    settings->attr_timeout = 1;
    settings->entry_timeout = 1;
    settings->negative_timeout = 0;
    return nullptr;
}

fuse_operations operations() {
    fuse_operations value{};
    value.getattr = getAttributes;
    value.readdir = readFolder;
    value.open = openFile;
    value.create = createFile;
    value.read = readFile;
    value.write = writeFile;
    value.truncate = truncateFile;
    value.release = releaseFile;
    value.fsync = syncFile;
    value.unlink = removeFile;
    value.rmdir = removeFolder;
    value.mkdir = makeFolder;
    value.rename = renameEntry;
    value.utimens = setTimes;
    value.chmod = setMode;
    value.access = checkAccess;
    value.statfs = fileSystem;
    value.init = start;
    return value;
}

void detachStaleMount() {
    struct stat info {};
    if (!stat(config().root.c_str(), &info) || errno != ENOTCONN) return;

    std::string program = "fusermount3";
    std::string unmount = "-u";
    std::string lazy = "-z";
    char* arguments[] = {program.data(), unmount.data(), lazy.data(), config().root.data(), nullptr};
    pid_t process = 0;

    if (!posix_spawnp(&process, program.c_str(), nullptr, nullptr, arguments, environ)) {
        waitpid(process, nullptr, 0);
    }
}

}

void mountRoot() {
    detachStaleMount();

    static fuse_operations const handlers = operations();
    fuse_args arguments = FUSE_ARGS_INIT(0, nullptr);
    fuse_opt_add_arg(&arguments, "fileship-harbor");
    fuse_opt_add_arg(&arguments, "-o");
    fuse_opt_add_arg(&arguments, "fsname=fileship,subtype=fileship,auto_unmount,default_permissions,noatime");

    session = fuse_new(&arguments, &handlers, sizeof(handlers), nullptr);
    fuse_opt_free_args(&arguments);

    if (!session || fuse_mount(session, config().root.c_str())) {
        throw std::runtime_error("Could not mount the Harbor folder");
    }

    isStopping = false;
    loop = std::thread([] { fuse_loop_mt(session, 0); });
    reporter = std::thread(runReporter);
}

void unmountRoot() {
    if (!session) return;

    stopThumbnails();

    {
        std::lock_guard lock(stateMutex);
        isStopping = true;
        stateSignal.notify_all();
    }

    fuse_exit(session);
    fuse_unmount(session);
    loop.join();
    reporter.join();
    fuse_destroy(session);
    session = nullptr;
}

bool isOpenForWriting(std::string const& relative) {
    std::lock_guard lock(stateMutex);
    return writers.contains(relative);
}

void scheduleChange(std::string const& relative) {
    std::lock_guard lock(stateMutex);
    changes[relative] = Clock::now() + SETTLE_TIME;
    stateSignal.notify_all();
}

void recheck(std::string const& relative) {
    {
        std::lock_guard lock(stateMutex);
        std::erase_if(reported, [&](auto const& item) { return item.first.ends_with("|" + relative); });
    }

    scheduleChange(relative);
}

}

#define FUSE_USE_VERSION 312

#include "harbor.h"

#include <shared/http.h>
#include <shared/utf8.h>

#include <fuse.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/xattr.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <thread>

namespace harbor {

namespace {

constexpr char const* PIN_ATTRIBUTE = "user.fileship.pinned";
constexpr int TIMEOUT_MS = 60000;
constexpr auto ACK_TIMEOUT = std::chrono::seconds(60);

struct Download {
    std::mutex mutex;
    std::condition_variable progress;
    int64_t received = 0;
    int64_t expected = 0;
    bool isDone = false;
    bool isFailed = false;
    uint64_t blob = 0;
};

struct Ack {
    bool isResolved = false;
    bool ok = false;
};

std::mutex downloadsMutex;
std::map<std::wstring, std::shared_ptr<Download>> downloads;

std::mutex acksMutex;
std::condition_variable ackSignal;
std::map<uint64_t, Ack> acks;
std::atomic<uint64_t> nextRequestId = 1;

fuse* session = nullptr;
std::thread loopThread;

fileship::HttpEndpoint endpoint() {
    return {config().instanceUrl, token(), config().userAgent, {}};
}

std::string relative(char const* path) {
    std::string value(path);
    while (!value.empty() && value.front() == '/') value.erase(value.begin());
    return value;
}

int64_t nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

timespec timeOf(int64_t milliseconds) {
    return {static_cast<time_t>(milliseconds / 1000), static_cast<long>(milliseconds % 1000) * 1'000'000};
}

std::string contentOf(std::wstring const& identity) {
    if (identity.starts_with(L"l:")) {
        return fileship::toUtf8(L"[InternetShortcut]\r\nURL=" + config().instanceUrl + L"/s/" + identity.substr(2) +
                                L"\r\n");
    }

    auto response = fileship::httpSend(endpoint(), {L"GET", L"/api/notes/" + identity.substr(2), {}, {}, TIMEOUT_MS});
    if (response.status != 200) throw std::runtime_error("HTTP " + std::to_string(response.status));

    auto note = JsonObject::Parse(fileship::toWide(std::string(response.body.begin(), response.body.end())));
    return fileship::toUtf8(note.GetNamedString(L"content"));
}

Node* findIdentity(std::wstring const& identity) {
    Node* found = nullptr;
    forEachNode(rootNode(), [&](Node* node) {
        if (!found && node->identity == identity) found = node;
    });
    return found;
}

void finishDownload(std::wstring const& identity, std::shared_ptr<Download> const& download, bool ok) {
    if (ok) {
        std::lock_guard lock(treeMutex());
        auto node = findIdentity(identity);

        if (node && !node->isHydrated) {
            auto path = blobPath(download->blob);
            timespec times[] = {timeOf(node->created), timeOf(node->created)};
            utimensat(AT_FDCWD, path.c_str(), times, 0);
            node->blob = download->blob;
            node->isHydrated = true;
            node->baseline = blobMtime(node->blob);
            markDirty();
        } else {
            std::error_code error;
            std::filesystem::remove(blobPath(download->blob), error);
        }
    }

    {
        std::lock_guard lock(download->mutex);
        download->isDone = true;
        download->isFailed = !ok;
    }
    download->progress.notify_all();

    std::lock_guard lock(downloadsMutex);
    downloads.erase(identity);
}

void runDownload(std::wstring identity, std::shared_ptr<Download> download) {
    auto path = blobPath(download->blob);
    bool ok = false;

    try {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        auto append = [&](char const* data, size_t size) {
            file.write(data, static_cast<std::streamsize>(size));
            file.flush();
            {
                std::lock_guard lock(download->mutex);
                download->received += static_cast<int64_t>(size);
            }
            download->progress.notify_all();
        };

        if (identity.starts_with(L"f:")) {
            fileship::HttpStream stream(endpoint(), {L"GET", L"/u/" + identity.substr(2), {}, {}, TIMEOUT_MS});
            if (stream.status != 200) throw std::runtime_error("HTTP " + std::to_string(stream.status));

            std::vector<char> buffer(256 * 1024);
            for (size_t read; (read = stream.read(buffer.data(), buffer.size())) > 0;) append(buffer.data(), read);
        } else {
            auto content = contentOf(identity);

            if (static_cast<int64_t>(content.size()) != download->expected) {
                log("The content of " + fileship::toUtf8(identity) + " does not match its placeholder");
                if (identity.starts_with(L"n:")) {
                    JsonObject message;
                    message.SetNamedValue(L"identity", JsonValue::CreateStringValue(identity));
                    sendEvent(L"noteChanged", message);
                }
                throw std::runtime_error("size mismatch");
            }
            append(content.data(), content.size());
        }

        file.close();
        ok = file.good() && download->received == download->expected;
        if (!ok) log("The download of " + fileship::toUtf8(identity) + " ended early");
    } catch (std::exception const& error) {
        log("Download failed: " + std::string(error.what()));
    }

    if (!ok) {
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    finishDownload(identity, download, ok);
}

std::shared_ptr<Download> downloadOf(Node* node) {
    std::lock_guard lock(downloadsMutex);
    auto found = downloads.find(node->identity);
    if (found != downloads.end()) return found->second;

    auto download = std::make_shared<Download>();
    download->blob = newBlob();
    download->expected = node->size;
    downloads[node->identity] = download;
    std::thread(runDownload, node->identity, download).detach();
    return download;
}

bool waitFor(std::shared_ptr<Download> const& download, int64_t until) {
    std::unique_lock lock(download->mutex);
    download->progress.wait(lock, [&] { return download->isDone || download->received >= until; });
    return !download->isFailed;
}

int requireHydrated(std::string const& path) {
    std::shared_ptr<Download> download;

    {
        std::lock_guard lock(treeMutex());
        auto node = find(path);
        if (!node) return -ENOENT;
        if (node->isFolder) return -EISDIR;
        if (node->isHydrated) return 0;
        if (node->identity.empty()) return -EIO;
        download = downloadOf(node);
    }

    std::unique_lock lock(download->mutex);
    download->progress.wait(lock, [&] { return download->isDone; });
    return download->isFailed ? -EIO : 0;
}

int fillStat(Node const* node, struct stat* info) {
    std::memset(info, 0, sizeof(*info));
    info->st_uid = getuid();
    info->st_gid = getgid();
    info->st_atim = info->st_mtim = info->st_ctim = timeOf(node->created);

    if (node->isFolder) {
        info->st_mode = S_IFDIR | 0755;
        info->st_nlink = 2;
        return 0;
    }

    info->st_mode = S_IFREG | (node->identity.starts_with(L"l:") ? 0444 : 0644);
    info->st_nlink = 1;
    info->st_size = node->size;

    struct stat blob {};
    if (node->isHydrated && ::stat(blobPath(node->blob).c_str(), &blob) == 0) {
        info->st_size = blob.st_size;
        info->st_blocks = blob.st_blocks;
        info->st_mtim = blob.st_mtim;
        info->st_ctim = blob.st_ctim;
        info->st_atim = blob.st_atim;
    }
    return 0;
}

int handleGetattr(char const* path, struct stat* info, fuse_file_info*) {
    std::lock_guard lock(treeMutex());
    auto node = find(relative(path));
    return node ? fillStat(node, info) : -ENOENT;
}

int handleReaddir(char const* path, void* buffer, fuse_fill_dir_t fill, off_t, fuse_file_info*, fuse_readdir_flags) {
    std::lock_guard lock(treeMutex());
    auto node = find(relative(path));
    if (!node) return -ENOENT;
    if (!node->isFolder) return -ENOTDIR;

    fill(buffer, ".", nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
    fill(buffer, "..", nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
    for (auto const& [name, child] : node->children) {
        struct stat info {};
        fillStat(child.get(), &info);
        fill(buffer, name.c_str(), &info, 0, static_cast<fuse_fill_dir_flags>(0));
    }
    return 0;
}

int handleMkdir(char const* path, mode_t) {
    auto target = relative(path);

    {
        std::lock_guard lock(treeMutex());
        auto parent = find(parentOf(target));
        if (!parent || !parent->isFolder) return -ENOENT;
        if (parent->children.contains(nameOf(target))) return -EEXIST;

        auto node = std::make_unique<Node>();
        node->name = nameOf(target);
        node->isFolder = true;
        node->created = nowMs();
        node->isPinned = parent->isPinned;
        attach(parent, std::move(node));
    }

    touch(target);
    return 0;
}

int openBlob(Node* node, int flags) {
    int fd = ::open(blobPath(node->blob).c_str(), (flags & O_ACCMODE) | O_CLOEXEC);
    return fd < 0 ? -errno : fd;
}

int handleCreate(char const* path, mode_t, fuse_file_info* info) {
    auto target = relative(path);
    std::lock_guard lock(treeMutex());
    auto parent = find(parentOf(target));
    if (!parent || !parent->isFolder) return -ENOENT;

    auto existing = parent->children.find(nameOf(target));
    Node* node = existing == parent->children.end() ? nullptr : existing->second.get();
    if (node && node->isFolder) return -EISDIR;

    if (!node) {
        auto created = std::make_unique<Node>();
        created->name = nameOf(target);
        created->created = nowMs();
        created->isPinned = parent->isPinned;
        node = attach(parent, std::move(created));
    } else {
        dropContent(node);
    }

    node->blob = newBlob();
    node->isHydrated = true;
    std::ofstream(blobPath(node->blob), std::ios::binary | std::ios::trunc);
    markDirty();

    int fd = openBlob(node, O_RDWR);
    if (fd < 0) return fd;
    info->fh = static_cast<uint64_t>(fd);
    node->openCount++;
    return 0;
}

int handleOpen(char const* path, fuse_file_info* info) {
    auto target = relative(path);
    bool isWrite = (info->flags & O_ACCMODE) != O_RDONLY;

    {
        std::lock_guard lock(treeMutex());
        auto node = find(target);
        if (!node) return -ENOENT;
        if (node->isFolder) return -EISDIR;
        if (isWrite && node->identity.starts_with(L"l:")) return -EACCES;

        if (isWrite && (info->flags & O_TRUNC) && !node->isHydrated) {
            node->blob = newBlob();
            node->isHydrated = true;
            std::ofstream(blobPath(node->blob), std::ios::binary | std::ios::trunc);
            markDirty();
        }

        if (!node->isHydrated && !isWrite) {
            info->fh = 0;
            node->openCount++;
            downloadOf(node);
            return 0;
        }
    }

    if (int result = requireHydrated(target); result < 0) return result;

    std::lock_guard lock(treeMutex());
    auto node = find(target);
    if (!node || !node->isHydrated) return -EIO;

    int fd = openBlob(node, isWrite ? O_RDWR : O_RDONLY);
    if (fd < 0) return fd;
    if (isWrite && (info->flags & O_TRUNC) && ftruncate(fd, 0) != 0) {
        ::close(fd);
        return -errno;
    }
    info->fh = static_cast<uint64_t>(fd);
    node->openCount++;
    return 0;
}

int handleRead(char const* path, char* buffer, size_t size, off_t offset, fuse_file_info* info) {
    if (info->fh) {
        auto count = pread(static_cast<int>(info->fh), buffer, size, offset);
        return count < 0 ? -errno : static_cast<int>(count);
    }

    std::shared_ptr<Download> download;
    uint64_t blob = 0;
    int64_t fileSize = 0;

    {
        std::lock_guard lock(treeMutex());
        auto node = find(relative(path));
        if (!node) return -ENOENT;
        fileSize = node->isHydrated ? blobSize(node->blob) : node->size;
        if (node->isHydrated) blob = node->blob;
        else download = downloadOf(node);
    }

    if (offset >= fileSize) return 0;

    if (download) {
        if (!waitFor(download, std::min<int64_t>(fileSize, offset + static_cast<int64_t>(size)))) return -EIO;
        blob = download->blob;
    }

    int fd = ::open(blobPath(blob).c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        std::lock_guard lock(treeMutex());
        auto node = find(relative(path));
        if (!node || !node->isHydrated) return -EIO;
        fd = ::open(blobPath(node->blob).c_str(), O_RDONLY | O_CLOEXEC);
        if (fd < 0) return -errno;
    }

    auto count = pread(fd, buffer, size, offset);
    int error = errno;
    ::close(fd);
    return count < 0 ? -error : static_cast<int>(count);
}

int handleWrite(char const* path, char const* buffer, size_t size, off_t offset, fuse_file_info* info) {
    if (!info->fh) return -EBADF;
    auto count = pwrite(static_cast<int>(info->fh), buffer, size, offset);
    if (count < 0) return -errno;
    touch(relative(path));
    return static_cast<int>(count);
}

int handleTruncate(char const* path, off_t size, fuse_file_info* info) {
    auto target = relative(path);

    if (info && info->fh) {
        if (ftruncate(static_cast<int>(info->fh), size) != 0) return -errno;
        touch(target);
        return 0;
    }

    if (size != 0) {
        if (int result = requireHydrated(target); result < 0) return result;
    }

    std::lock_guard lock(treeMutex());
    auto node = find(target);
    if (!node) return -ENOENT;
    if (node->isFolder) return -EISDIR;
    if (node->identity.starts_with(L"l:")) return -EACCES;

    if (!node->isHydrated) {
        node->blob = newBlob();
        node->isHydrated = true;
        std::ofstream(blobPath(node->blob), std::ios::binary | std::ios::trunc);
        markDirty();
    }
    if (::truncate(blobPath(node->blob).c_str(), size) != 0) return -errno;

    touch(target);
    return 0;
}

int handleRelease(char const* path, fuse_file_info* info) {
    if (info->fh) ::close(static_cast<int>(info->fh));

    {
        std::lock_guard lock(treeMutex());
        auto node = find(relative(path));
        if (node && node->openCount) node->openCount--;
    }

    if ((info->flags & O_ACCMODE) != O_RDONLY) touch(relative(path));
    return 0;
}

int handleFsync(char const*, int, fuse_file_info* info) {
    return info->fh && fsync(static_cast<int>(info->fh)) != 0 ? -errno : 0;
}

void reportDelete(std::wstring const& identity, std::string const& path, bool isFolder) {
    if (identity.empty()) return;

    JsonObject message;
    message.SetNamedValue(L"identity", JsonValue::CreateStringValue(identity));
    message.SetNamedValue(L"path", JsonValue::CreateStringValue(fileship::toWide(path)));
    message.SetNamedValue(L"isFolder", JsonValue::CreateBooleanValue(isFolder));
    sendEvent(L"delete", message);
}

int handleUnlink(char const* path) {
    auto target = relative(path);
    std::wstring identity;

    {
        std::lock_guard lock(treeMutex());
        auto node = find(target);
        if (!node) return -ENOENT;
        if (node->isFolder) return -EISDIR;

        identity = node->identity;
        forget(target);
        dropContent(node);
        detach(node);
    }

    reportDelete(identity, target, false);
    return 0;
}

int handleRmdir(char const* path) {
    auto target = relative(path);
    std::wstring identity;

    {
        std::lock_guard lock(treeMutex());
        auto node = find(target);
        if (!node) return -ENOENT;
        if (!node->isFolder) return -ENOTDIR;
        if (!node->parent || node->identity.starts_with(L"v:")) return -EPERM;
        if (!node->children.empty()) return -ENOTEMPTY;

        identity = node->identity;
        forget(target);
        detach(node);
    }

    reportDelete(identity, target, true);
    return 0;
}

bool askRename(Node const* node, std::string const& from, std::string const& to) {
    uint64_t requestId = nextRequestId++;

    {
        std::lock_guard lock(acksMutex);
        acks[requestId] = Ack{};
    }

    JsonObject message;
    message.SetNamedValue(L"identity", JsonValue::CreateStringValue(node->identity));
    message.SetNamedValue(L"from", JsonValue::CreateStringValue(fileship::toWide(from)));
    message.SetNamedValue(L"to", JsonValue::CreateStringValue(fileship::toWide(to)));
    message.SetNamedValue(L"isFolder", JsonValue::CreateBooleanValue(node->isFolder));
    message.SetNamedValue(L"requestId", JsonValue::CreateNumberValue(static_cast<double>(requestId)));
    sendEvent(L"rename", message);

    std::unique_lock lock(acksMutex);
    ackSignal.wait_for(lock, ACK_TIMEOUT, [&] { return acks[requestId].isResolved; });
    bool ok = acks[requestId].ok;
    acks.erase(requestId);
    return ok;
}

int handleRename(char const* fromPath, char const* toPath, unsigned int flags) {
    auto from = relative(fromPath);
    auto to = relative(toPath);
    if (flags & RENAME_EXCHANGE) return -EINVAL;

    std::wstring identity;
    bool isFolder = false;

    {
        std::lock_guard lock(treeMutex());
        auto node = find(from);
        auto parent = find(parentOf(to));
        if (!node || !parent) return -ENOENT;
        if (!parent->isFolder) return -ENOTDIR;
        if (node->identity.starts_with(L"v:")) return -EPERM;

        auto existing = parent->children.find(nameOf(to));
        if (existing != parent->children.end()) {
            auto* target = existing->second.get();
            if (target == node) return 0;
            if (flags & RENAME_NOREPLACE) return -EEXIST;
            if (target->isFolder && !node->isFolder) return -EISDIR;
            if (!target->isFolder && node->isFolder) return -ENOTDIR;
            if (target->isFolder && !target->children.empty()) return -ENOTEMPTY;
        }
        if (node->isFolder && (to == from || to.starts_with(from + "/"))) return -EINVAL;

        identity = node->identity;
        isFolder = node->isFolder;
    }

    if (!identity.empty()) {
        Node snapshot;
        snapshot.identity = identity;
        snapshot.isFolder = isFolder;
        if (!askRename(&snapshot, from, to)) return -EACCES;
    }

    {
        std::lock_guard lock(treeMutex());
        auto node = find(from);
        auto parent = find(parentOf(to));
        if (!node || !parent) return -ENOENT;

        auto existing = parent->children.find(nameOf(to));
        if (existing != parent->children.end()) {
            forget(to);
            dropContent(existing->second.get());
            parent->children.erase(existing);
        }

        forget(from);
        auto owned = detach(node);
        owned->name = nameOf(to);
        if (!owned->identity.empty()) owned->inSync = true;
        attach(parent, std::move(owned));
    }

    touch(to);
    return 0;
}

int handleUtimens(char const* path, timespec const times[2], fuse_file_info*) {
    std::lock_guard lock(treeMutex());
    auto node = find(relative(path));
    if (!node) return -ENOENT;
    if (!node->isHydrated) return 0;
    if (utimensat(AT_FDCWD, blobPath(node->blob).c_str(), times, 0) != 0) return -errno;
    touch(relative(path));
    return 0;
}

int handleChmod(char const*, mode_t, fuse_file_info*) {
    return 0;
}

int handleStatfs(char const*, struct statvfs* info) {
    return statvfs(config().cache.c_str(), info) == 0 ? 0 : -errno;
}

int handleSetxattr(char const* path, char const* name, char const* value, size_t size, int) {
    if (std::strcmp(name, PIN_ATTRIBUTE) != 0) return -ENOTSUP;

    auto target = relative(path);
    bool isPinned = size > 0 && value[0] == '1';
    std::vector<std::wstring> hydrate;
    std::lock_guard unpin(treeMutex());

    {
        std::lock_guard lock(treeMutex());
        auto node = find(target);
        if (!node) return -ENOENT;

        auto apply = [&](Node* item) {
            item->isPinned = isPinned;
            if (isPinned && !item->isFolder && !item->isHydrated && !item->identity.empty()) {
                hydrate.push_back(item->identity);
            }
        };
        apply(node);
        if (node->isFolder) forEachNode(node, apply);
        markDirty();
    }

    for (auto const& identity : hydrate) startDownload(identity);
    if (!isPinned) freeUpSpace(find(target));
    return 0;
}

int handleGetxattr(char const* path, char const* name, char* value, size_t size) {
    if (std::strcmp(name, PIN_ATTRIBUTE) != 0) return -ENODATA;

    std::lock_guard lock(treeMutex());
    auto node = find(relative(path));
    if (!node) return -ENOENT;
    if (size == 0) return 1;
    value[0] = node->isPinned ? '1' : '0';
    return 1;
}

int handleListxattr(char const* path, char* list, size_t size) {
    std::lock_guard lock(treeMutex());
    if (!find(relative(path))) return -ENOENT;

    auto length = static_cast<int>(std::strlen(PIN_ATTRIBUTE) + 1);
    if (size == 0) return length;
    if (size < static_cast<size_t>(length)) return -ERANGE;
    std::memcpy(list, PIN_ATTRIBUTE, static_cast<size_t>(length));
    return length;
}

void* handleInit(fuse_conn_info*, fuse_config* settings) {
    settings->entry_timeout = 0;
    settings->attr_timeout = 0;
    settings->negative_timeout = 0;
    settings->kernel_cache = 0;
    settings->hard_remove = 1;
    return nullptr;
}

fuse_operations const& operations() {
    static fuse_operations const value = [] {
        fuse_operations result{};
        result.getattr = handleGetattr;
        result.readdir = handleReaddir;
        result.mkdir = handleMkdir;
        result.create = handleCreate;
        result.open = handleOpen;
        result.read = handleRead;
        result.write = handleWrite;
        result.truncate = handleTruncate;
        result.release = handleRelease;
        result.fsync = handleFsync;
        result.unlink = handleUnlink;
        result.rmdir = handleRmdir;
        result.rename = handleRename;
        result.utimens = handleUtimens;
        result.chmod = handleChmod;
        result.statfs = handleStatfs;
        result.setxattr = handleSetxattr;
        result.getxattr = handleGetxattr;
        result.listxattr = handleListxattr;
        result.init = handleInit;
        return result;
    }();
    return value;
}

}

void startDownload(std::wstring const& identity) {
    std::lock_guard lock(treeMutex());
    auto node = findIdentity(identity);
    if (node && !node->isFolder && !node->isHydrated) downloadOf(node);
}

void hydrateNow(std::string const& path) {
    requireHydrated(path);
}

void resolveAck(uint64_t requestId, bool ok) {
    {
        std::lock_guard lock(acksMutex);
        auto found = acks.find(requestId);
        if (found == acks.end()) return;
        found->second = Ack{true, ok};
    }
    ackSignal.notify_all();
}

void mountRoot() {
    if (session) return;

    std::filesystem::create_directories(config().root);
    std::filesystem::create_directories(config().cache / "blobs");

    auto mountpoint = config().root.string();
    std::string program = "fileship-harbor";
    std::string option = "-o";
    std::string options = "fsname=fileship,subtype=harbor,default_permissions,auto_unmount";
    char* arguments[] = {program.data(), option.data(), options.data(), nullptr};
    fuse_args args = FUSE_ARGS_INIT(3, arguments);

    session = fuse_new(&args, &operations(), sizeof(fuse_operations), nullptr);
    if (!session) throw std::runtime_error("Could not create the Harbor file system");

    if (fuse_mount(session, mountpoint.c_str()) != 0) {
        fuse_destroy(session);
        session = nullptr;
        throw std::runtime_error("Could not mount the Harbor folder");
    }

    loopThread = std::thread([] {
        fuse_loop_config* loop = fuse_loop_cfg_create();
        fuse_loop_cfg_set_max_threads(loop, 16);
        fuse_loop_mt(session, loop);
        fuse_loop_cfg_destroy(loop);
    });
}

void unmountRoot() {
    if (!session) return;

    fuse_exit(session);

    auto isFinished = std::make_shared<std::atomic<bool>>(false);
    std::thread waker([isFinished, root = config().root] {
        for (int attempt = 0; attempt < 50 && !*isFinished; attempt++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (*isFinished) break;
            struct stat info {};
            ::stat(root.c_str(), &info);
        }
    });

    if (loopThread.joinable()) loopThread.join();
    *isFinished = true;
    fuse_unmount(session);
    waker.join();
    fuse_destroy(session);
    session = nullptr;
}

}

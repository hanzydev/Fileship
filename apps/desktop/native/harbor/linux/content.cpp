#include "common.h"

#include <shared/http.h>
#include <shared/utf8.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace harbor {

namespace {

using fileship::toUtf8;
using fileship::toWide;

constexpr int64_t BLOCK_SIZE = 1024 * 1024;
constexpr int64_t READ_AHEAD = 4;
constexpr int TIMEOUT_MS = 60000;

struct Download {
    std::mutex mutex;
    std::vector<bool> blocks;
    timespec times[2]{};
};

std::mutex downloadsMutex;
std::map<std::string, std::shared_ptr<Download>> downloads;

std::shared_ptr<Download> downloadOf(std::string const& relative, int64_t size) {
    std::lock_guard lock(downloadsMutex);
    auto& download = downloads[relative];

    if (!download) {
        struct stat info {};
        lstat(backingOf(relative).c_str(), &info);

        download = std::make_shared<Download>();
        download->blocks.assign(static_cast<size_t>((size + BLOCK_SIZE - 1) / BLOCK_SIZE), false);
        download->times[0] = info.st_atim;
        download->times[1] = info.st_mtim;
    }

    return download;
}

struct File {
    int descriptor;

    explicit File(std::string const& relative) : descriptor(open(backingOf(relative).c_str(), O_WRONLY | O_CLOEXEC)) {
        if (descriptor < 0) throw std::runtime_error("Could not open " + relative);
    }

    ~File() {
        close(descriptor);
    }

    void write(char const* data, size_t size, int64_t offset) {
        while (size) {
            ssize_t count = pwrite(descriptor, data, size, offset);
            if (count <= 0) throw std::runtime_error("Could not write the download");

            data += count;
            offset += count;
            size -= static_cast<size_t>(count);
        }
    }
};

void downloadBytes(File& file, std::string const& id, int64_t offset, int64_t length) {
    auto range = L"Range: bytes=" + std::to_wstring(offset) + L"-" + std::to_wstring(offset + length - 1) + L"\r\n";
    fileship::HttpStream stream(endpoint(), {L"GET", L"/u/" + toWide(id), range, {}, TIMEOUT_MS, {}});
    if (stream.status != 200 && stream.status != 206) {
        throw std::runtime_error("HTTP " + std::to_string(stream.status));
    }

    int64_t skip = stream.status == 200 ? offset : 0;
    int64_t position = offset;
    int64_t remaining = length;
    std::vector<char> buffer(64 * 1024);

    while (remaining > 0) {
        auto read = static_cast<int64_t>(stream.read(buffer.data(), buffer.size()));
        if (read == 0) break;

        int64_t start = std::min(skip, read);
        skip -= start;

        int64_t usable = std::min(read - start, remaining);
        file.write(buffer.data() + start, static_cast<size_t>(usable), position);
        position += usable;
        remaining -= usable;
    }

    if (remaining > 0) throw std::runtime_error("The download ended early");
}

std::string contentOf(std::string const& identity) {
    if (identity.starts_with("l:")) {
        return toUtf8(L"[InternetShortcut]\r\nURL=" + config().instanceUrl + L"/s/" + toWide(identity.substr(2)) +
                      L"\r\n");
    }

    auto response =
        fileship::httpSend(endpoint(), {L"GET", L"/api/notes/" + toWide(identity.substr(2)), {}, {}, TIMEOUT_MS, {}});
    if (response.status != 200) throw std::runtime_error("HTTP " + std::to_string(response.status));

    return toUtf8(JsonObject::Parse(toWide(response.text())).GetNamedString(L"content"));
}

void finish(std::string const& relative, Download const& download) {
    utimensat(AT_FDCWD, backingOf(relative).c_str(), download.times, AT_SYMLINK_NOFOLLOW);
    finishHydration(relative);
    forgetBlocks(relative);
}

}

fileship::HttpEndpoint endpoint() {
    return {config().instanceUrl, config().token, config().userAgent, {}};
}

void fetchRange(std::string const& relative, int64_t offset, int64_t length) {
    auto entry = entryOf(relative);
    if (!entry.remote || length <= 0 || offset >= *entry.remote) return;

    auto download = downloadOf(relative, *entry.remote);
    std::lock_guard lock(download->mutex);

    entry = entryOf(relative);
    if (!entry.remote) return;

    int64_t size = *entry.remote;
    auto const& identity = *entry.identity;
    File file(relative);

    if (!identity.starts_with("f:")) {
        auto content = contentOf(identity);

        if (static_cast<int64_t>(content.size()) != size) {
            if (identity.starts_with("n:")) {
                JsonObject message;
                message.SetNamedValue(L"identity", text(identity));
                sendEvent("noteChanged", message);
            }

            throw std::runtime_error("The content of " + relative + " does not match its placeholder");
        }

        file.write(content.data(), content.size(), 0);
        finish(relative, *download);
        return;
    }

    auto count = static_cast<int64_t>(download->blocks.size());
    int64_t last = std::min((offset + length - 1) / BLOCK_SIZE, count - 1);

    for (int64_t block = offset / BLOCK_SIZE; block <= last; block++) {
        if (download->blocks[block]) continue;

        int64_t ahead = block && download->blocks[block - 1] ? READ_AHEAD : 1;
        int64_t end = block;
        while (end + 1 < count && !download->blocks[end + 1] && (end < last || end + 1 - block < ahead)) end++;

        int64_t start = block * BLOCK_SIZE;
        downloadBytes(file, identity.substr(2), start, std::min((end + 1) * BLOCK_SIZE, size) - start);
        std::fill(download->blocks.begin() + block, download->blocks.begin() + end + 1, true);
        block = end;
    }

    if (std::ranges::all_of(download->blocks, [](bool isPresent) { return isPresent; })) finish(relative, *download);
}

void fetchAll(std::string const& relative) {
    auto entry = entryOf(relative);
    if (entry.remote) fetchRange(relative, 0, *entry.remote);
}

void forgetBlocks(std::string const& relative) {
    std::lock_guard lock(downloadsMutex);
    downloads.erase(relative);
}

}

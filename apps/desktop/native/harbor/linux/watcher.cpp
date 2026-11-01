#include "harbor.h"

#include <shared/utf8.h>

#include <atomic>
#include <chrono>
#include <thread>

namespace harbor {

namespace {

using Clock = std::chrono::steady_clock;

constexpr auto SETTLE_TIME = std::chrono::milliseconds(1500);

std::atomic<bool> isWatching = false;
std::thread worker;

std::mutex changesMutex;
std::map<std::string, Clock::time_point> changes;
std::map<std::string, std::string> reported;

void report(std::wstring const& type, std::string const& relative, std::string const& signature,
            JsonObject message) {
    {
        std::lock_guard lock(changesMutex);
        auto key = fileship::toUtf8(type) + "|" + relative;
        if (reported[key] == signature) return;
        reported[key] = signature;
    }

    message.SetNamedValue(L"path", JsonValue::CreateStringValue(fileship::toWide(relative)));
    sendEvent(type, message);
}

void inspect(std::string const& relative) {
    if (isIgnoredName(nameOf(relative))) return;

    std::wstring type;
    std::wstring download;
    std::string signature;
    bool isRetry = false;
    JsonObject message;

    {
        std::lock_guard lock(treeMutex());
        auto node = find(relative);
        if (!node || !node->parent) return;

        int64_t size = node->isFolder ? 0 : node->isHydrated ? blobSize(node->blob) : node->size;
        signature = std::to_string(size) + ":" +
                    std::to_string(node->isHydrated ? blobMtime(node->blob) : node->created);

        if (node->identity.empty()) {
            if (!node->isFolder && node->openCount) {
                isRetry = true;
            } else {
                type = L"localNew";
                message.SetNamedValue(L"isFolder", JsonValue::CreateBooleanValue(node->isFolder));
                message.SetNamedValue(L"size", JsonValue::CreateNumberValue(static_cast<double>(size)));
            }
        } else if (!node->isFolder && isModified(node)) {
            if (node->openCount) {
                isRetry = true;
            } else {
                type = L"localModified";
                message.SetNamedValue(L"identity", JsonValue::CreateStringValue(node->identity));
                message.SetNamedValue(L"size", JsonValue::CreateNumberValue(static_cast<double>(size)));
            }
        } else if (!node->isFolder && node->isPinned && !node->isHydrated) {
            download = node->identity;
        }
    }

    if (isRetry) {
        std::lock_guard lock(changesMutex);
        changes[relative] = Clock::now();
        return;
    }

    if (!download.empty()) startDownload(download);
    if (!type.empty()) report(type, relative, signature, message);
}

void processChanges() {
    while (isWatching) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::vector<std::string> ready;
        auto now = Clock::now();

        {
            std::lock_guard lock(changesMutex);
            for (auto iterator = changes.begin(); iterator != changes.end();) {
                if (now - iterator->second >= SETTLE_TIME) {
                    ready.push_back(iterator->first);
                    iterator = changes.erase(iterator);
                } else {
                    ++iterator;
                }
            }
        }

        for (auto const& relative : ready) {
            try {
                inspect(relative);
            } catch (std::exception const& error) {
                log("Could not inspect " + relative + ": " + error.what());
            }
        }
    }
}

}

void startWatcher() {
    if (isWatching) return;
    isWatching = true;
    worker = std::thread(processChanges);
}

void stopWatcher() {
    if (!isWatching) return;
    isWatching = false;
    if (worker.joinable()) worker.join();
}

void touch(std::string const& relative) {
    std::lock_guard lock(changesMutex);
    changes[relative] = Clock::now();
}

void forget(std::string const& relative) {
    std::lock_guard lock(changesMutex);

    for (auto iterator = reported.begin(); iterator != reported.end();) {
        auto path = iterator->first.substr(iterator->first.find('|') + 1);
        bool isSame = path == relative || path.starts_with(relative + "/");
        iterator = isSame ? reported.erase(iterator) : std::next(iterator);
    }
}

void recheck(std::string const& relative) {
    forget(relative);
    touch(relative);
}

}

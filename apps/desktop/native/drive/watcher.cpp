#include "common.h"

#include <chrono>
#include <map>
#include <thread>

using namespace winrt;
using Clock = std::chrono::steady_clock;

namespace drive {

namespace {

constexpr auto SETTLE_TIME = std::chrono::milliseconds(1500);

std::atomic<bool> isWatching = false;
HANDLE directory = INVALID_HANDLE_VALUE;
std::thread readerThread;
std::thread workerThread;

std::mutex changesMutex;
std::map<std::wstring, Clock::time_point> changes;
std::map<std::wstring, std::wstring> reported;

bool isClosed(std::wstring const& absolute) {
    HANDLE handle = CreateFileW(absolute.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return false;
    CloseHandle(handle);
    return true;
}

void report(std::wstring const& type, std::wstring const& relative, std::wstring const& signature,
            JsonObject message) {
    {
        std::lock_guard lock(changesMutex);
        auto key = type + L"|" + relative;
        if (reported[key] == signature) return;
        reported[key] = signature;
    }

    message.SetNamedValue(L"path", JsonValue::CreateStringValue(relative));
    sendEvent(type, message);
}

void retryLater(std::wstring const& relative) {
    std::lock_guard lock(changesMutex);
    changes[relative] = Clock::now();
}

void inspect(std::wstring const& relative) {
    auto absolute = fullPath(relative);
    auto name = std::filesystem::path(absolute).filename().wstring();
    if (isIgnoredName(name)) return;

    WIN32_FILE_ATTRIBUTE_DATA data{};
    if (!GetFileAttributesExW(absolute.c_str(), GetFileExInfoStandard, &data)) return;

    bool isFolder = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    bool isHydrated = (data.dwFileAttributes & FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS) == 0;
    int64_t size = (static_cast<int64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
    auto signature = std::to_wstring(size) + L":" + std::to_wstring(data.ftLastWriteTime.dwLowDateTime) +
                     L":" + std::to_wstring(data.ftLastWriteTime.dwHighDateTime);

    CF_PIN_STATE pin = CF_PIN_STATE_UNSPECIFIED;
    CF_IN_SYNC_STATE sync = CF_IN_SYNC_STATE_NOT_IN_SYNC;
    auto identity = placeholderIdentity(absolute, &pin, &sync);

    if (!identity || identity->empty()) {
        if (!isFolder && !isClosed(absolute)) return retryLater(relative);

        JsonObject message;
        message.SetNamedValue(L"isFolder", JsonValue::CreateBooleanValue(isFolder));
        message.SetNamedValue(L"size", JsonValue::CreateNumberValue(static_cast<double>(size)));
        report(L"localNew", relative, signature, message);
        return;
    }

    if (isFolder) return;

    if (isHydrated && isModified(absolute)) {
        if (!isClosed(absolute)) return retryLater(relative);

        JsonObject message;
        message.SetNamedValue(L"identity", JsonValue::CreateStringValue(*identity));
        message.SetNamedValue(L"size", JsonValue::CreateNumberValue(static_cast<double>(size)));
        report(L"localModified", relative, signature, message);
        return;
    }

    try {
        if (pin == CF_PIN_STATE_PINNED && !isHydrated) {
            hydratePath(absolute);
        } else if (pin == CF_PIN_STATE_UNPINNED && isHydrated) {
            dehydratePath(absolute);
        }
    } catch (hresult_error const& error) {
        log(L"Could not change " + relative + L": " + std::wstring(error.message()));
    }
}

void processChanges() {
    while (isWatching) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::vector<std::wstring> ready;
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
                log(L"Could not inspect " + relative + L": " + toWide(error.what()));
            } catch (hresult_error const& error) {
                log(L"Could not inspect " + relative + L": " + std::wstring(error.message()));
            }
        }
    }
}

void readChanges() {
    std::vector<BYTE> buffer(64 * 1024);

    while (isWatching) {
        DWORD returned = 0;
        BOOL ok = ReadDirectoryChangesW(
            directory, buffer.data(), static_cast<DWORD>(buffer.size()), TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES |
                FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE,
            &returned, nullptr, nullptr);

        if (!ok || !isWatching) break;
        if (returned == 0) continue;

        auto now = Clock::now();
        auto entry = reinterpret_cast<FILE_NOTIFY_INFORMATION const*>(buffer.data());
        std::lock_guard lock(changesMutex);

        for (;;) {
            std::wstring relative(entry->FileName, entry->FileNameLength / sizeof(wchar_t));
            if (entry->Action != FILE_ACTION_REMOVED && entry->Action != FILE_ACTION_RENAMED_OLD_NAME) {
                changes[relative] = now;
            }

            if (entry->NextEntryOffset == 0) break;
            entry = reinterpret_cast<FILE_NOTIFY_INFORMATION const*>(
                reinterpret_cast<BYTE const*>(entry) + entry->NextEntryOffset);
        }
    }
}

}

void startWatcher() {
    if (isWatching) return;

    directory = CreateFileW(config().root.c_str(), FILE_LIST_DIRECTORY,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (directory == INVALID_HANDLE_VALUE) throw_last_error();

    isWatching = true;
    readerThread = std::thread(readChanges);
    workerThread = std::thread(processChanges);
}

void stopWatcher() {
    if (!isWatching) return;

    isWatching = false;
    CancelIoEx(directory, nullptr);
    if (readerThread.joinable()) readerThread.join();
    if (workerThread.joinable()) workerThread.join();
    CloseHandle(directory);
    directory = INVALID_HANDLE_VALUE;
}

}

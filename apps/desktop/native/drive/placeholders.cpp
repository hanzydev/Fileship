#include "common.h"

#include <thread>

using namespace winrt;

namespace drive {

namespace {

LARGE_INTEGER fileTime(int64_t milliseconds) {
    LARGE_INTEGER value{};
    value.QuadPart = (milliseconds + 11644473600000LL) * 10000LL;
    return value;
}

struct Handle {
    HANDLE value = INVALID_HANDLE_VALUE;

    explicit Handle(std::wstring const& path, DWORD access) {
        value = CreateFileW(path.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                            nullptr);
        if (value == INVALID_HANDLE_VALUE) throw_last_error();
    }

    ~Handle() {
        if (value != INVALID_HANDLE_VALUE) CloseHandle(value);
    }

    Handle(Handle const&) = delete;
    Handle& operator=(Handle const&) = delete;
};

bool isDirectory(std::wstring const& path) {
    DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

std::optional<std::wstring> readIdentity(std::wstring const& path, CF_PIN_STATE* pin, CF_IN_SYNC_STATE* sync) {
    std::vector<BYTE> buffer(sizeof(CF_PLACEHOLDER_BASIC_INFO) + 1024);
    DWORD returned = 0;

    try {
        Handle handle(path, FILE_READ_ATTRIBUTES);
        HRESULT result = CfGetPlaceholderInfo(handle.value, CF_PLACEHOLDER_INFO_BASIC, buffer.data(),
                                              static_cast<DWORD>(buffer.size()), &returned);
        if (FAILED(result)) return std::nullopt;
    } catch (hresult_error const&) {
        return std::nullopt;
    }

    auto info = reinterpret_cast<CF_PLACEHOLDER_BASIC_INFO const*>(buffer.data());
    if (pin) *pin = info->PinState;
    if (sync) *sync = info->InSyncState;

    return std::wstring(reinterpret_cast<wchar_t const*>(info->FileIdentity),
                        info->FileIdentityLength / sizeof(wchar_t));
}

bool childrenInSync(std::wstring const& directory) {
    WIN32_FIND_DATAW data{};
    HANDLE find = FindFirstFileExW((directory + L"\\*").c_str(), FindExInfoBasic, &data, FindExSearchNameMatch,
                                   nullptr, FIND_FIRST_EX_LARGE_FETCH);
    if (find == INVALID_HANDLE_VALUE) return GetLastError() == ERROR_FILE_NOT_FOUND;

    bool inSync = true;

    do {
        std::wstring name(data.cFileName);
        if (name == L"." || name == L".." || isIgnoredName(name)) continue;

        CF_IN_SYNC_STATE sync = CF_IN_SYNC_STATE_NOT_IN_SYNC;
        auto identity = readIdentity(directory + L"\\" + name, nullptr, &sync);
        inSync = identity && !identity->empty() && !identity->starts_with(L"p:") &&
                 sync == CF_IN_SYNC_STATE_IN_SYNC;
    } while (inSync && FindNextFileW(find, &data));

    FindClose(find);
    return inSync;
}

void claimExisting(PlaceholderItem const& item, std::wstring const& absolute) {
    auto existing = readIdentity(absolute, nullptr, nullptr);

    if (existing == item.identity) {
        markInSync(item.path);
        return;
    }

    if (item.isFolder && !existing && isDirectory(absolute)) {
        adoptPath(item.path, item.identity);
        return;
    }

    throw_hresult(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS));
}

void requireDirectory(std::wstring const& path) {
    if (!isDirectory(path)) throw_hresult(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND));
}

void inheritPin(std::wstring const& parent, std::wstring const& path) {
    CF_PIN_STATE pin = CF_PIN_STATE_UNSPECIFIED;
    if (!readIdentity(parent, &pin, nullptr) || pin != CF_PIN_STATE_PINNED) return;

    setPinState(path, CF_PIN_STATE_PINNED);
    if (isDirectory(path)) return;

    std::thread([path] {
        try {
            hydratePath(path);
        } catch (hresult_error const& error) {
            log(L"Could not hydrate " + path + L": " + std::wstring(error.message()));
        }
    }).detach();
}

}

void createPlaceholders(std::vector<PlaceholderItem> const& items) {
    for (auto const& item : items) {
        auto absolute = std::filesystem::path(fullPath(item.path));
        auto parent = absolute.parent_path().wstring();
        auto name = absolute.filename().wstring();

        requireDirectory(parent);

        CF_PLACEHOLDER_CREATE_INFO info{};
        info.RelativeFileName = name.c_str();
        info.FsMetadata.FileSize.QuadPart = item.isFolder ? 0 : item.size;
        info.FsMetadata.BasicInfo.FileAttributes =
            item.isFolder ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
        info.FsMetadata.BasicInfo.CreationTime = fileTime(item.created);
        info.FsMetadata.BasicInfo.LastWriteTime = fileTime(item.created);
        info.FsMetadata.BasicInfo.LastAccessTime = fileTime(item.created);
        info.FsMetadata.BasicInfo.ChangeTime = fileTime(item.created);
        info.FileIdentity = item.identity.c_str();
        info.FileIdentityLength = static_cast<DWORD>(item.identity.size() * sizeof(wchar_t));
        info.Flags = CF_PLACEHOLDER_CREATE_FLAG_MARK_IN_SYNC;
        if (item.isFolder) info.Flags |= CF_PLACEHOLDER_CREATE_FLAG_DISABLE_ON_DEMAND_POPULATION;

        DWORD processed = 0;
        HRESULT result = CfCreatePlaceholders(parent.c_str(), &info, 1, CF_CREATE_FLAG_NONE, &processed);

        if (result == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) ||
            info.Result == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) ||
            info.Result == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS)) {
            claimExisting(item, absolute.wstring());
            continue;
        }

        check_hresult(result);
        check_hresult(info.Result);
        inheritPin(parent, absolute.wstring());
    }
}

void movePlaceholder(std::wstring const& from, std::wstring const& to) {
    auto target = std::filesystem::path(fullPath(to));
    requireDirectory(target.parent_path().wstring());

    if (!MoveFileExW(fullPath(from).c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH)) throw_last_error();
    markInSync(to);
}

std::optional<CF_FS_METADATA> baselineOf(HANDLE handle) {
    FILE_BASIC_INFO basic{};
    LARGE_INTEGER size{};

    if (!GetFileInformationByHandleEx(handle, FileBasicInfo, &basic, sizeof(basic))) throw_last_error();
    if (basic.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) return std::nullopt;
    if (!GetFileSizeEx(handle, &size)) throw_last_error();

    CF_FS_METADATA metadata{};
    metadata.BasicInfo = basic;
    metadata.BasicInfo.LastWriteTime = basic.CreationTime;
    metadata.FileSize = size;
    return metadata;
}

void updatePlaceholder(std::wstring const& path, std::optional<std::wstring> const& identity,
                       bool resetBaseline) {
    Handle handle(fullPath(path), FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES);
    auto metadata = resetBaseline ? baselineOf(handle.value) : std::nullopt;

    check_hresult(CfUpdatePlaceholder(
        handle.value,
        metadata ? &*metadata : nullptr,
        identity ? identity->c_str() : nullptr,
        identity ? static_cast<DWORD>(identity->size() * sizeof(wchar_t)) : 0,
        nullptr,
        0,
        CF_UPDATE_FLAG_MARK_IN_SYNC,
        nullptr,
        nullptr));
}

void removePath(std::wstring const& path) {
    std::error_code error;
    std::filesystem::remove_all(fullPath(path), error);
    if (error) throw std::runtime_error(error.message());
}

void adoptPath(std::wstring const& path, std::wstring const& identity) {
    auto absolute = fullPath(path);
    bool directory = isDirectory(absolute);

    if (!directory && readIdentity(absolute, nullptr, nullptr)) {
        updatePlaceholder(path, identity, true);
        return;
    }

    Handle handle(absolute, FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES);

    CF_CONVERT_FLAGS flags = directory ? CF_CONVERT_FLAG_ALWAYS_FULL : CF_CONVERT_FLAG_MARK_IN_SYNC;

    check_hresult(CfConvertToPlaceholder(handle.value, identity.c_str(),
                                         static_cast<DWORD>(identity.size() * sizeof(wchar_t)), flags,
                                         nullptr, nullptr));

    if (!directory) updatePlaceholder(path, std::nullopt, true);
}

std::wstring preparePath(std::wstring const& path, std::wstring const& identity) {
    auto absolute = fullPath(path);
    auto existing = readIdentity(absolute, nullptr, nullptr);
    Handle handle(absolute, FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES);

    if (existing) {
        check_hresult(CfSetInSyncState(handle.value, CF_IN_SYNC_STATE_NOT_IN_SYNC, CF_SET_IN_SYNC_FLAG_NONE,
                                       nullptr));
        return existing->empty() ? identity : *existing;
    }

    check_hresult(CfConvertToPlaceholder(handle.value, identity.c_str(),
                                         static_cast<DWORD>(identity.size() * sizeof(wchar_t)),
                                         CF_CONVERT_FLAG_NONE, nullptr, nullptr));
    return identity;
}

bool markInSync(std::wstring const& path) {
    try {
        auto absolute = path.empty() ? config().root : fullPath(path);
        CF_IN_SYNC_STATE sync = CF_IN_SYNC_STATE_NOT_IN_SYNC;
        auto identity = readIdentity(absolute, nullptr, &sync);

        if (!identity || identity->starts_with(L"p:")) return false;
        if (isDirectory(absolute) ? !childrenInSync(absolute) : identity->empty() || isModified(absolute)) {
            return false;
        }
        if (sync == CF_IN_SYNC_STATE_IN_SYNC) return true;

        Handle handle(absolute, FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES);
        return SUCCEEDED(
            CfSetInSyncState(handle.value, CF_IN_SYNC_STATE_IN_SYNC, CF_SET_IN_SYNC_FLAG_NONE, nullptr));
    } catch (...) {
        return false;
    }
}

void reportProgress(std::wstring const& path, int64_t total, int64_t completed) {
    Handle handle(fullPath(path), FILE_READ_ATTRIBUTES);
    CF_TRANSFER_KEY key{};
    check_hresult(CfGetTransferKey(handle.value, &key));

    LARGE_INTEGER totalBytes{};
    LARGE_INTEGER completedBytes{};
    totalBytes.QuadPart = total;
    completedBytes.QuadPart = completed;

    HRESULT result = CfReportProviderProgress(connectionKey(), key, totalBytes, completedBytes);
    CfReleaseTransferKey(handle.value, &key);
    check_hresult(result);
}

bool isModified(std::wstring const& absolute) {
    try {
        Handle handle(absolute, FILE_READ_ATTRIBUTES);
        FILE_BASIC_INFO basic{};
        if (!GetFileInformationByHandleEx(handle.value, FileBasicInfo, &basic, sizeof(basic))) return false;
        if (basic.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) return false;

        return std::llabs(basic.LastWriteTime.QuadPart - basic.CreationTime.QuadPart) > 20000000LL;
    } catch (hresult_error const&) {
        return false;
    }
}

JsonArray scanRoot() {
    JsonArray items;
    std::error_code error;
    auto options = std::filesystem::directory_options::skip_permission_denied;

    for (auto iterator = std::filesystem::recursive_directory_iterator(config().root, options, error);
         iterator != std::filesystem::recursive_directory_iterator(); iterator.increment(error)) {
        if (error) break;

        auto absolute = iterator->path().wstring();
        auto relative = relativePath(absolute);
        if (!relative) continue;

        DWORD attributes = GetFileAttributesW(absolute.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES) continue;

        CF_IN_SYNC_STATE sync = CF_IN_SYNC_STATE_NOT_IN_SYNC;
        auto identity = readIdentity(absolute, nullptr, &sync);
        bool directory = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        JsonObject item;
        item.SetNamedValue(L"path", JsonValue::CreateStringValue(*relative));
        item.SetNamedValue(L"isFolder", JsonValue::CreateBooleanValue(directory));
        item.SetNamedValue(L"identity", identity ? JsonValue::CreateStringValue(*identity)
                                                 : JsonValue::CreateNullValue());
        item.SetNamedValue(L"inSync", JsonValue::CreateBooleanValue(sync == CF_IN_SYNC_STATE_IN_SYNC));
        item.SetNamedValue(L"modified", JsonValue::CreateBooleanValue(identity && isModified(absolute)));
        item.SetNamedValue(L"hydrated",
                           JsonValue::CreateBooleanValue((attributes & FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS) == 0));
        item.SetNamedValue(L"size", JsonValue::CreateNumberValue(
            directory ? 0 : static_cast<double>(std::filesystem::file_size(iterator->path(), error))));
        items.Append(item);
    }

    return items;
}

std::optional<std::wstring> placeholderIdentity(std::wstring const& absolute, CF_PIN_STATE* pin,
                                                CF_IN_SYNC_STATE* sync) {
    return readIdentity(absolute, pin, sync);
}

void hydratePath(std::wstring const& absolute) {
    Handle handle(absolute, FILE_READ_ATTRIBUTES);
    LARGE_INTEGER offset{};
    LARGE_INTEGER length{};
    length.QuadPart = -1;
    check_hresult(CfHydratePlaceholder(handle.value, offset, length, CF_HYDRATE_FLAG_NONE, nullptr));
}

void setPinState(std::wstring const& absolute, CF_PIN_STATE pin) {
    Handle handle(absolute, FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES);
    check_hresult(CfSetPinState(handle.value, pin, CF_SET_PIN_FLAG_NONE, nullptr));
}

void dehydratePath(std::wstring const& absolute) {
    Handle handle(absolute, FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES);
    LARGE_INTEGER offset{};
    LARGE_INTEGER length{};
    length.QuadPart = -1;
    check_hresult(CfDehydratePlaceholder(handle.value, offset, length, CF_DEHYDRATE_FLAG_NONE, nullptr));
}

}

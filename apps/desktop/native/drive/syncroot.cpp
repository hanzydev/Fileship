#include "common.h"

#include <sddl.h>

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Storage.Provider.h>
#include <winrt/Windows.Storage.h>

using namespace winrt;
using namespace Windows::Security::Cryptography;
using namespace Windows::Storage;
using namespace Windows::Storage::Provider;

namespace drive {

namespace {

constexpr guid PROVIDER_ID{0x6f1c2b7e, 0x3d4a, 0x4e8b, {0x9a, 0x51, 0x2c, 0x7d, 0x80, 0xe4, 0x13, 0x5f}};

CF_CONNECTION_KEY currentKey{};
bool isConnected = false;

std::wstring userSid() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) throw_last_error();

    DWORD size = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    std::vector<BYTE> buffer(size);

    if (!GetTokenInformation(token, TokenUser, buffer.data(), size, &size)) {
        CloseHandle(token);
        throw_last_error();
    }

    CloseHandle(token);

    LPWSTR text = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid, &text)) {
        throw_last_error();
    }

    std::wstring sid(text);
    LocalFree(text);
    return sid;
}

void CALLBACK onFetchData(CF_CALLBACK_INFO const* info, CF_CALLBACK_PARAMETERS const* parameters) {
    fetchData(info, parameters);
}

void CALLBACK onCancelFetchData(CF_CALLBACK_INFO const* info, CF_CALLBACK_PARAMETERS const*) {
    cancelFetch(info);
}

void CALLBACK onNotifyDeleteCompletion(CF_CALLBACK_INFO const* info, CF_CALLBACK_PARAMETERS const*) {
    reportDeleted(info);
}

void CALLBACK onNotifyRename(CF_CALLBACK_INFO const* info, CF_CALLBACK_PARAMETERS const* parameters) {
    askRename(info, parameters);
}

void CALLBACK onNotifyRenameCompletion(CF_CALLBACK_INFO const* info, CF_CALLBACK_PARAMETERS const* parameters) {
    renamed(info, parameters);
}

CF_CALLBACK_REGISTRATION callbacks[] = {
    {CF_CALLBACK_TYPE_FETCH_DATA, onFetchData},
    {CF_CALLBACK_TYPE_CANCEL_FETCH_DATA, onCancelFetchData},
    {CF_CALLBACK_TYPE_NOTIFY_DELETE_COMPLETION, onNotifyDeleteCompletion},
    {CF_CALLBACK_TYPE_NOTIFY_RENAME, onNotifyRename},
    {CF_CALLBACK_TYPE_NOTIFY_RENAME_COMPLETION, onNotifyRenameCompletion},
    CF_CALLBACK_REGISTRATION_END,
};

}

CF_CONNECTION_KEY connectionKey() {
    return currentKey;
}

std::wstring syncRootId() {
    return L"Fileship!" + userSid() + L"!" + config().syncRootId;
}

namespace {

std::optional<std::wstring> folderSyncRootId(StorageFolder const& folder) {
    try {
        return std::wstring(StorageProviderSyncRootManager::GetSyncRootInformationForFolder(folder).Id());
    } catch (hresult_error const&) {
        return std::nullopt;
    }
}

void unregisterStaleRoot(StorageFolder const& folder) {
    auto const other = folderSyncRootId(folder);

    if (other && *other != syncRootId() && other->starts_with(L"Fileship!" + userSid() + L"!")) {
        log(L"Removing the stale sync root " + *other);
        unregisterSyncRoot(*other);
    }
}

}

void registerSyncRoot() {
    auto const& settings = config();

    auto const folder = StorageFolder::GetFolderFromPathAsync(settings.root).get();
    unregisterStaleRoot(folder);

    StorageProviderSyncRootInfo info;
    info.Id(syncRootId());
    info.Path(folder);
    info.DisplayNameResource(settings.displayName);
    info.IconResource(settings.iconPath);
    info.HydrationPolicy(StorageProviderHydrationPolicy::Full);
    info.HydrationPolicyModifier(StorageProviderHydrationPolicyModifier::AutoDehydrationAllowed);
    info.PopulationPolicy(StorageProviderPopulationPolicy::AlwaysFull);
    info.InSyncPolicy(StorageProviderInSyncPolicy::FileCreationTime |
                      StorageProviderInSyncPolicy::FileLastWriteTime |
                      StorageProviderInSyncPolicy::DirectoryCreationTime |
                      StorageProviderInSyncPolicy::PreserveInsyncForSyncEngine);
    info.HardlinkPolicy(StorageProviderHardlinkPolicy::None);
    info.ShowSiblingsAsGroup(false);
    info.AllowPinning(true);
    info.Version(settings.version);
    info.ProviderId(PROVIDER_ID);
    info.RecycleBinUri(Windows::Foundation::Uri(settings.instanceUrl + L"/dashboard/trash"));
    info.Context(CryptographicBuffer::ConvertStringToBinary(syncRootId(), BinaryStringEncoding::Utf8));

    StorageProviderSyncRootManager::Register(info);
}

void unregisterSyncRoot(std::wstring const& id) {
    try {
        StorageProviderSyncRootManager::Unregister(id);
    } catch (hresult_error const& error) {
        log(L"Unregister failed: " + std::wstring(error.message()));
    }
}

void unregisterAll() {
    for (auto const& root : StorageProviderSyncRootManager::GetCurrentSyncRoots()) {
        std::wstring id(root.Id());
        if (id.starts_with(L"Fileship!")) unregisterSyncRoot(id);
    }
}

void connectSyncRoot() {
    if (isConnected) return;

    check_hresult(CfConnectSyncRoot(
        config().root.c_str(),
        callbacks,
        nullptr,
        CF_CONNECT_FLAG_REQUIRE_PROCESS_INFO | CF_CONNECT_FLAG_REQUIRE_FULL_FILE_PATH,
        &currentKey));

    isConnected = true;
}

void disconnectSyncRoot() {
    if (!isConnected) return;

    CfDisconnectSyncRoot(currentKey);
    isConnected = false;
}

void reportSyncStatus(std::optional<std::wstring> const& error) {
    if (!error) {
        check_hresult(CfReportSyncStatus(config().root.c_str(), nullptr));
        return;
    }

    auto size = static_cast<ULONG>((error->size() + 1) * sizeof(wchar_t));
    std::vector<BYTE> buffer(sizeof(CF_SYNC_STATUS) + size);
    auto status = reinterpret_cast<CF_SYNC_STATUS*>(buffer.data());
    status->StructSize = static_cast<ULONG>(buffer.size());
    status->Code = 1;
    status->DescriptionOffset = sizeof(CF_SYNC_STATUS);
    status->DescriptionLength = size;
    memcpy(buffer.data() + sizeof(CF_SYNC_STATUS), error->c_str(), size);

    check_hresult(CfReportSyncStatus(config().root.c_str(), status));
}

}

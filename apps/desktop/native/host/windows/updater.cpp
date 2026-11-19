#include "host.h"

#include <host/updater.h>
#include <shared/log.h>

#include <bcrypt.h>
#include <shlobj.h>

namespace fileship::host {

namespace {

class BcryptSha512 : public Sha512 {
public:
    BcryptSha512() {
        BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA512_ALGORITHM, nullptr, 0);
        BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0);
    }

    ~BcryptSha512() override {
        if (hash) BCryptDestroyHash(hash);
        if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    }

    void update(char const* data, size_t size) override {
        BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<char*>(data)), static_cast<ULONG>(size), 0);
    }

    std::wstring finish() override {
        uint8_t digest[64]{};
        BCryptFinishHash(hash, digest, sizeof(digest), 0);
        return base64(digest, sizeof(digest));
    }

private:
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
};

}

std::unique_ptr<Sha512> Sha512::create() {
    return std::make_unique<BcryptSha512>();
}

std::wstring updateFeedFile() {
    return L"latest.yml";
}

std::wstring updatePackageSuffix() {
    return {};
}

std::filesystem::path updateDownloadFolder() {
    if (!isPackaged()) return userDataPath() / L"updates";

    PWSTR folder = nullptr;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &folder);
    std::filesystem::path result = std::filesystem::path(folder) / L"fileship-updater" / L"pending";
    CoTaskMemFree(folder);
    return result;
}

InstallResult runUpdateInstaller(std::filesystem::path const& path, bool isRestarting) {
    std::wstring command = L"\"" + path.wstring() + L"\" --updated /S" + (isRestarting ? L" --force-run" : L"");
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION process{};

    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
                        nullptr, path.parent_path().c_str(), &startup, &process)) {
        logError(L"Could not start the update installer: " + std::to_wstring(GetLastError()));
        return InstallResult::Started;
    }

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return InstallResult::Started;
}

void installOnQuit() {
    auto file = pendingUpdate();
    if (!file.empty() && jsonString(updateStatus(), L"state") == L"ready") runUpdateInstaller(file, false);
}

}

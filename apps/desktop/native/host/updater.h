#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

namespace fileship::host {

enum class InstallResult { Started, Refused, Failed };

class Sha512 {
public:
    static std::unique_ptr<Sha512> create();

    virtual ~Sha512() = default;

    virtual void update(char const* data, size_t size) = 0;
    virtual std::wstring finish() = 0;
};

std::wstring updateFeedFile();
std::wstring updatePackageSuffix();
std::filesystem::path updateDownloadFolder();
InstallResult runUpdateInstaller(std::filesystem::path const& file, bool isRestarting);
std::filesystem::path pendingUpdate();

}

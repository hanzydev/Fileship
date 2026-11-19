#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace fileship::host {

struct Release {
    std::wstring version;
    std::wstring url;
    std::wstring sha512;
    int64_t size = 0;
};

struct SemVer {
    int major = 0;
    int minor = 0;
    int patch = 0;
    std::wstring prerelease;
};

std::optional<SemVer> parseVersion(std::wstring text);
bool isNewer(SemVer const& remote, SemVer const& local);
Release parseRelease(std::string const& text, std::wstring const& suffix);

}

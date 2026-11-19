#include "updateFeed.h"

#include <shared/utf8.h>

#include <cwchar>
#include <sstream>
#include <vector>

namespace fileship::host {

namespace {

std::wstring unquote(std::wstring value) {
    auto start = value.find_first_not_of(L" \t");
    if (start == std::wstring::npos) return {};

    value = value.substr(start, value.find_last_not_of(L" \t\r") - start + 1);
    if (value.size() >= 2 && (value.front() == L'\'' || value.front() == L'"') && value.back() == value.front())
        return value.substr(1, value.size() - 2);
    return value;
}

std::optional<int> numberOf(std::wstring const& text) {
    if (text.empty() || text.find_first_not_of(L"0123456789") != std::wstring::npos) return std::nullopt;
    return static_cast<int>(std::wcstol(text.c_str(), nullptr, 10));
}

bool endsWith(std::wstring const& text, std::wstring const& suffix) {
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

}

std::optional<SemVer> parseVersion(std::wstring text) {
    if (!text.empty() && (text[0] == L'v' || text[0] == L'V')) text.erase(0, 1);

    SemVer version;
    auto plus = text.find(L'+');
    if (plus != std::wstring::npos) text.resize(plus);

    auto hyphen = text.find(L'-');
    if (hyphen != std::wstring::npos) {
        version.prerelease = text.substr(hyphen + 1);
        text.resize(hyphen);
    }

    auto first = text.find(L'.');
    auto second = first == std::wstring::npos ? first : text.find(L'.', first + 1);
    if (second == std::wstring::npos) return std::nullopt;

    auto major = numberOf(text.substr(0, first));
    auto minor = numberOf(text.substr(first + 1, second - first - 1));
    auto patch = numberOf(text.substr(second + 1));
    if (!major || !minor || !patch) return std::nullopt;

    version.major = *major;
    version.minor = *minor;
    version.patch = *patch;
    return version;
}

bool isNewer(SemVer const& remote, SemVer const& local) {
    if (remote.major != local.major) return remote.major > local.major;
    if (remote.minor != local.minor) return remote.minor > local.minor;
    if (remote.patch != local.patch) return remote.patch > local.patch;
    if (remote.prerelease == local.prerelease) return false;
    if (remote.prerelease.empty()) return true;
    if (local.prerelease.empty()) return false;
    return remote.prerelease > local.prerelease;
}

Release parseRelease(std::string const& text, std::wstring const& suffix) {
    Release top;
    std::vector<Release> files;
    std::wistringstream lines(toWide(text));
    std::wstring line;
    bool inFiles = false;

    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == L'\r') line.pop_back();

        auto first = line.find_first_not_of(L' ');
        if (first == std::wstring::npos || line[first] == L'#') continue;

        bool isIndented = first > 0 || line[first] == L'-';
        bool isEntry = line[first] == L'-';
        auto trimmed = line.substr(line.find_first_not_of(L" -"));
        auto colon = trimmed.find(L':');
        if (colon == std::wstring::npos) continue;

        auto key = trimmed.substr(0, colon);
        auto value = unquote(trimmed.substr(colon + 1));

        if (!isIndented) {
            inFiles = key == L"files";
            if (key == L"version") top.version = value;
            if (key == L"path") top.url = value;
            if (key == L"sha512") top.sha512 = value;
            continue;
        }

        if (!inFiles) continue;
        if (isEntry) files.emplace_back();
        if (files.empty()) continue;

        if (key == L"url") files.back().url = value;
        if (key == L"sha512") files.back().sha512 = value;
        if (key == L"size") files.back().size = std::wcstoll(value.c_str(), nullptr, 10);
    }

    Release release;
    release.version = top.version;

    for (auto const& file : files) {
        if (!endsWith(file.url, suffix)) continue;

        release.url = file.url;
        release.sha512 = file.sha512;
        release.size = file.size;
        return release;
    }

    if (files.empty() && endsWith(top.url, suffix)) {
        release.url = top.url;
        release.sha512 = top.sha512;
    }

    return release;
}

}

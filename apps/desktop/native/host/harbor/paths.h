#pragma once

#include "support.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace fileship::host::harbor {

struct RemoteFolder {
    std::wstring id;
    std::wstring name;
    std::optional<std::wstring> parentId;
    int64_t createdAt = 0;
};

struct RemoteFile {
    std::wstring id;
    std::wstring name;
    std::optional<std::wstring> folderId;
    int64_t size = 0;
    int64_t createdAt = 0;
    bool isLocked = false;
    std::optional<int64_t> expiresAt;
    int64_t maxViews = 0;
    bool isSensitive = false;
    int64_t views = 0;
};

struct RemoteNote {
    std::wstring id;
    std::wstring title;
    int64_t size = 0;
    std::string hash;
    int64_t createdAt = 0;
};

struct RemoteLink {
    std::wstring id;
    std::wstring code;
    std::wstring shortUrl;
    int64_t createdAt = 0;
};

template <typename T>
struct VirtualFolder {
    std::wstring name;
    std::vector<T> items;
};

struct LayoutEntry {
    std::wstring identity;
    std::wstring path;
    bool isFolder = false;
    int64_t size = 0;
    int64_t created = 0;
};

using Layout = InsertionMap<LayoutEntry>;

inline std::wstring const NOTES_FOLDER = L"v:notes";
inline std::wstring const LINKS_FOLDER = L"v:links";

inline std::wstring folderIdentity(std::wstring const& id) {
    return L"d:" + id;
}

inline std::wstring fileIdentity(std::wstring const& id) {
    return L"f:" + id;
}

inline std::wstring noteIdentity(std::wstring const& id) {
    return L"n:" + id;
}

inline std::wstring linkIdentity(std::wstring const& code) {
    return L"l:" + code;
}

std::wstring linkShortcut(std::wstring const& instanceUrl, std::wstring const& code);
std::wstring safeName(std::wstring const& name, std::wstring const& fallback);

Layout buildLayout(std::vector<RemoteFolder> const& folders, std::vector<RemoteFile> const& files,
                   std::optional<VirtualFolder<RemoteNote>> const& notes,
                   std::optional<VirtualFolder<RemoteLink>> const& links, std::wstring const& instanceUrl);

}

#include "paths.h"

#include <algorithm>
#include <cwctype>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace fileship::host::harbor {

namespace {

using NameSet = std::unordered_set<std::wstring>;

bool isInvalid(wchar_t character) {
    return std::wstring_view(L"\\/:*?\"<>|").find(character) != std::wstring_view::npos ||
           character <= 0x1f || (character >= 0x7f && character <= 0x9f);
}

bool isSpace(wchar_t character) {
    return std::iswspace(character) || character == 0xfeff || character == 0xa0;
}

bool isReserved(std::wstring const& value) {
    auto stem = lower(value.substr(0, value.find(L'.')));

    if (stem == L"con" || stem == L"prn" || stem == L"aux" || stem == L"nul") return true;
    return stem.size() == 4 && (startsWith(stem, L"com") || startsWith(stem, L"lpt")) &&
           stem[3] >= L'0' && stem[3] <= L'9';
}

std::pair<std::wstring, std::wstring> splitExtension(std::wstring const& name) {
    auto dot = name.find_last_of(L'.');
    if (dot == std::wstring::npos || dot == 0) return {name, L""};
    return {name.substr(0, dot), name.substr(dot)};
}

std::wstring uniqueName(std::wstring const& name, NameSet& taken) {
    auto [base, extension] = splitExtension(name);
    std::wstring candidate = name;

    for (int index = 2; taken.contains(lower(candidate)); index++) {
        candidate = base + L" (" + std::to_wstring(index) + L")" + extension;
    }

    taken.insert(lower(candidate));
    return candidate;
}

template <typename T>
bool byAge(T const& a, T const& b) {
    if (a.createdAt != b.createdAt) return a.createdAt < b.createdAt;
    return a.id < b.id;
}

template <typename T>
std::vector<T> sortedByAge(std::vector<T> items) {
    std::ranges::stable_sort(items, byAge<T>);
    return items;
}

}

std::wstring linkShortcut(std::wstring const& instanceUrl, std::wstring const& code) {
    return L"[InternetShortcut]\r\nURL=" + instanceUrl + L"/s/" + code + L"\r\n";
}

std::wstring safeName(std::wstring const& name, std::wstring const& fallback) {
    std::wstring value;
    for (auto character : name) {
        if (!isInvalid(character)) value += character;
    }

    while (!value.empty() && isSpace(value.front())) value.erase(value.begin());
    while (!value.empty() && isSpace(value.back())) value.pop_back();
    while (!value.empty() && (value.back() == L'.' || value.back() == L' ')) value.pop_back();

    if (value.empty()) value = fallback;
    if (isReserved(value)) value = L"_" + value;

    return value.substr(0, 200);
}

Layout buildLayout(std::vector<RemoteFolder> const& folders, std::vector<RemoteFile> const& files,
                   std::optional<VirtualFolder<RemoteNote>> const& notes,
                   std::optional<VirtualFolder<RemoteLink>> const& links, std::wstring const& instanceUrl) {
    Layout layout;
    NameSet rootNames;
    std::unordered_map<std::wstring, NameSet> folderNames;
    std::unordered_map<std::wstring, std::wstring> folderPaths;
    std::unordered_set<std::wstring> known;
    std::map<std::wstring, std::vector<RemoteFolder>> children;
    std::vector<RemoteFolder> topLevel;

    for (auto const& folder : folders) known.insert(folder.id);

    for (auto const& folder : sortedByAge(folders)) {
        if (folder.parentId && known.contains(*folder.parentId)) {
            children[*folder.parentId].push_back(folder);
        } else {
            topLevel.push_back(folder);
        }
    }

    std::function<void(std::vector<RemoteFolder> const&, std::wstring const&, NameSet&)> addFolders =
        [&](std::vector<RemoteFolder> const& list, std::wstring const& parentPath, NameSet& names) {
            for (auto const& folder : list) {
                auto name = uniqueName(safeName(folder.name, L"Folder"), names);
                auto path = parentPath.empty() ? name : parentPath + L"\\" + name;

                folderPaths[folder.id] = path;
                folderNames[folder.id] = {};
                layout.set(folderIdentity(folder.id),
                           {folderIdentity(folder.id), path, true, 0, folder.createdAt});

                auto found = children.find(folder.id);
                if (found != children.end()) addFolders(found->second, path, folderNames[folder.id]);
            }
        };

    addFolders(topLevel, L"", rootNames);

    for (auto const& file : sortedByAge(files)) {
        auto parent = file.folderId ? folderPaths.find(*file.folderId) : folderPaths.end();
        bool hasParent = parent != folderPaths.end();
        auto& names = hasParent ? folderNames[*file.folderId] : rootNames;
        auto name = uniqueName(safeName(file.name, L"File"), names);

        layout.set(fileIdentity(file.id), {fileIdentity(file.id),
                                           hasParent ? parent->second + L"\\" + name : name, false,
                                           file.size, file.createdAt});
    }

    if (notes) {
        auto path = uniqueName(safeName(notes->name, L"Folder"), rootNames);
        NameSet names;

        layout.set(NOTES_FOLDER, {NOTES_FOLDER, path, true, 0, nowMs()});

        for (auto const& note : sortedByAge(notes->items)) {
            auto name = uniqueName(safeName(note.title, L"Note") + L".md", names);
            layout.set(noteIdentity(note.id),
                       {noteIdentity(note.id), path + L"\\" + name, false, note.size, note.createdAt});
        }
    }

    if (links) {
        auto path = uniqueName(safeName(links->name, L"Folder"), rootNames);
        NameSet names;

        layout.set(LINKS_FOLDER, {LINKS_FOLDER, path, true, 0, nowMs()});

        for (auto const& link : sortedByAge(links->items)) {
            auto name = uniqueName(safeName(link.code, L"Link") + L".url", names);
            auto size = static_cast<int64_t>(toUtf8(linkShortcut(instanceUrl, link.code)).size());
            layout.set(linkIdentity(link.code),
                       {linkIdentity(link.code), path + L"\\" + name, false, size, link.createdAt});
        }
    }

    return layout;
}

}

#include <host/app.h>

#include <host/harbor.h>
#include <shared/log.h>

#include <mutex>

namespace fileship::host {

namespace {

constexpr wchar_t const* SHELL_KEY = L"Software\\Classes\\*\\shell";
constexpr wchar_t const* FOLDER_SHELL_KEY = L"Software\\Classes\\Directory\\shell";
constexpr wchar_t const* UPLOAD_KEY = L"Software\\Classes\\*\\shell\\FileshipUpload";
constexpr wchar_t const* RUN_KEY = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t const* RUN_VALUE = L"Fileship";
constexpr wchar_t const* ELECTRON_RUN_VALUE = L"dev.hanzy.fileship";

struct HarborVerb {
    wchar_t const* key;
    wchar_t const* name;
    wchar_t const* action;
    wchar_t const* label;
    bool many;
};

constexpr HarborVerb HARBOR_VERBS[] = {
    {SHELL_KEY, L"FileshipHarborCopyLink", L"copyLink", L"shell.copyLink", true},
    {SHELL_KEY, L"FileshipHarborOpen", L"open", L"shell.openInBrowser", true},
    {SHELL_KEY, L"FileshipHarborShow", L"show", L"shell.showInFileship", false},
    {FOLDER_SHELL_KEY, L"FileshipHarborFolderLink", L"folderLink", L"shell.copyFolderLink", false},
    {FOLDER_SHELL_KEY, L"FileshipHarborInboxLink", L"inboxLink", L"shell.copyInboxLink", false},
};

std::mutex mutex;
std::optional<std::filesystem::path> shellRoot;

void setValue(std::wstring const& key, wchar_t const* name, std::wstring const& data) {
    auto result = RegSetKeyValueW(HKEY_CURRENT_USER, key.c_str(), name, REG_SZ, data.c_str(),
                                  static_cast<DWORD>((data.size() + 1) * sizeof(wchar_t)));
    if (result != ERROR_SUCCESS) throw std::runtime_error("Could not write " + toUtf8(key));
}

void deleteValue(std::wstring const& key, wchar_t const* name) {
    RegDeleteKeyValueW(HKEY_CURRENT_USER, key.c_str(), name);
}

std::wstring insideRoot(std::filesystem::path const& root) {
    auto text = root.wstring();
    while (!text.empty() && text.back() == L'\\') text.pop_back();
    return L"System.ItemPathDisplay:~<\"" + text + L"\\\"";
}

void writeHarborVerbs(std::wstring const& exe, std::filesystem::path const& root) {
    for (auto const& verb : HARBOR_VERBS) {
        auto key = std::wstring(verb.key) + L"\\" + verb.name;

        setValue(key, L"MUIVerb", t(verb.label));
        setValue(key, L"Icon", exe);
        setValue(key, L"AppliesTo", insideRoot(root));
        setValue(key, L"MultiSelectModel", verb.many ? L"Player" : L"Single");
        setValue(key + L"\\command", nullptr, L"\"" + exe + L"\" --harbor-action " + verb.action + L" \"%1\"");
    }
}

}

void registerShellMenu() {
    if (!isPackaged()) return;

    auto exe = hostExecutable().wstring();
    std::optional<std::filesystem::path> root;

    {
        std::lock_guard lock(mutex);
        root = shellRoot;
    }

    try {
        setValue(UPLOAD_KEY, L"MUIVerb", t(L"shell.upload"));
        setValue(UPLOAD_KEY, L"Icon", exe);
        setValue(UPLOAD_KEY, L"MultiSelectModel", L"Player");
        setValue(std::wstring(UPLOAD_KEY) + L"\\command", nullptr, L"\"" + exe + L"\" --upload \"%1\"");

        if (root) {
            setValue(UPLOAD_KEY, L"AppliesTo", L"NOT " + insideRoot(*root));
            writeHarborVerbs(exe, *root);
        } else {
            deleteValue(UPLOAD_KEY, L"AppliesTo");
        }
    } catch (std::exception const& error) {
        logError(L"Shell menu registration failed: " + errorMessage(error));
    }
}

void setShellHarborRoot(std::optional<std::filesystem::path> const& root) {
    {
        std::lock_guard lock(mutex);
        shellRoot = root;
    }

    if (!isPackaged()) return;

    if (!root) {
        for (auto const& verb : HARBOR_VERBS) {
            RegDeleteTreeW(HKEY_CURRENT_USER, (std::wstring(verb.key) + L"\\" + verb.name).c_str());
        }
    }

    registerShellMenu();
}

void applyStartAtLogin(bool enabled) {
    if (!isPackaged()) return;

    deleteValue(RUN_KEY, ELECTRON_RUN_VALUE);

    if (!enabled) {
        deleteValue(RUN_KEY, RUN_VALUE);
        return;
    }

    try {
        setValue(RUN_KEY, RUN_VALUE, L"\"" + hostExecutable().wstring() + L"\" --hidden");
    } catch (std::exception const& error) {
        logError(L"Could not update the startup entry: " + errorMessage(error));
    }
}

}

#include <host/harbor/platform.h>
#include <host/harbor/support.h>

#include <host/app.h>
#include <shared/i18n.h>

#include <windows.h>

#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>

#pragma comment(lib, "comctl32.lib")

namespace fileship::host::harbor {

namespace {

std::filesystem::path knownFolder(KNOWNFOLDERID const& id) {
    PWSTR folder = nullptr;
    SHGetKnownFolderPath(id, 0, nullptr, &folder);
    std::filesystem::path result(folder);
    CoTaskMemFree(folder);
    return result;
}

HRESULT CALLBACK dialogCallback(HWND window, UINT notification, WPARAM, LPARAM, LONG_PTR) {
    if (notification == TDN_CREATED) {
        SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetForegroundWindow(window);
    }

    return S_OK;
}

}

wchar_t const SEPARATOR = L'\\';

std::filesystem::path helperPath() {
    return resourcesPath().parent_path() / L"harbor" / L"fileship-harbor.exe";
}

std::filesystem::path homeFolder() {
    return knownFolder(FOLDERID_Profile);
}

std::wstring stampOf(std::filesystem::path const& path) {
    WIN32_FILE_ATTRIBUTE_DATA data{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data)) return L"";

    auto size = (static_cast<uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
    auto time = (static_cast<uint64_t>(data.ftLastWriteTime.dwHighDateTime) << 32) |
                data.ftLastWriteTime.dwLowDateTime;
    return std::to_wstring(size) + L":" + std::to_wstring(time);
}

std::wstring pathKey(std::wstring const& path) {
    return lower(path);
}

bool confirmPublic(std::wstring const& message, std::wstring const& detail) {
    constexpr int MAKE_PUBLIC = 100;
    constexpr int CANCEL = 101;

    auto makePublic = t(L"harbor.makePublic");
    auto cancel = t(L"harbor.cancel");
    TASKDIALOG_BUTTON buttons[] = {{MAKE_PUBLIC, makePublic.c_str()}, {CANCEL, cancel.c_str()}};

    TASKDIALOGCONFIG config{};
    config.cbSize = sizeof(config);
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
    config.pszWindowTitle = L"Fileship";
    config.pszMainInstruction = message.c_str();
    config.pszContent = detail.c_str();
    config.cButtons = static_cast<UINT>(std::size(buttons));
    config.pButtons = buttons;
    config.nDefaultButton = MAKE_PUBLIC;
    config.pfCallback = dialogCallback;

    int pressed = CANCEL;
    if (FAILED(TaskDialogIndirect(&config, &pressed, nullptr, nullptr))) return false;
    return pressed == MAKE_PUBLIC;
}

void openFolder(std::filesystem::path const& folder) {
    ShellExecuteW(nullptr, L"open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void forgetHelperSession() {
    std::error_code error;
    std::filesystem::remove(knownFolder(FOLDERID_LocalAppData) / L"Fileship" / L"harbor.json", error);
}

}

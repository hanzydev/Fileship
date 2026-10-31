#include "win32.h"

#include <host/app.h>
#include <host/tray.h>

#include <commctrl.h>
#include <shellapi.h>

namespace fileship::host {

namespace {

constexpr UINT TRAY_MESSAGE = WM_APP + 1;
constexpr UINT TRAY_ID = 1;
constexpr UINT_PTR CLICK_TIMER = 1;
constexpr UINT FIRST_COMMAND = 100;

UINT taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
HICON icon = nullptr;
bool isAdded = false;
ULONGLONG lastDoubleClick = 0;
std::vector<std::function<void()>> commands;

HMENU createMenu(std::vector<MenuItem> const& items) {
    HMENU menu = CreatePopupMenu();

    for (auto const& item : items) {
        if (item.separator) {
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            continue;
        }

        UINT flags = MF_STRING | (item.enabled ? 0 : MF_GRAYED);

        if (!item.submenu.empty()) {
            AppendMenuW(menu, flags | MF_POPUP, reinterpret_cast<UINT_PTR>(createMenu(item.submenu)), item.label.c_str());
            continue;
        }

        commands.push_back(item.click);
        AppendMenuW(menu, flags, FIRST_COMMAND + commands.size() - 1, item.label.c_str());
    }

    return menu;
}

void showMenu() {
    commands.clear();
    HMENU menu = createMenu(trayMenu());

    POINT cursor{};
    GetCursorPos(&cursor);
    SetForegroundWindow(mainWindow());

    UINT command = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON, cursor.x, cursor.y,
                                    mainWindow(), nullptr);
    PostMessageW(mainWindow(), WM_NULL, 0, 0);
    DestroyMenu(menu);

    if (command >= FIRST_COMMAND && command - FIRST_COMMAND < commands.size()) {
        auto click = commands[command - FIRST_COMMAND];
        if (click) click();
    }
}

NOTIFYICONDATAW iconData() {
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = mainWindow();
    data.uID = TRAY_ID;
    return data;
}

void addIcon() {
    if (!icon) {
        LoadIconMetric(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1), LIM_SMALL, &icon);
    }

    auto data = iconData();
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    data.uCallbackMessage = TRAY_MESSAGE;
    data.hIcon = icon;
    wcsncpy_s(data.szTip, trayTooltip().c_str(), _TRUNCATE);

    isAdded = Shell_NotifyIconW(NIM_ADD, &data) != FALSE;
    data.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &data);
}


}

void applyMenuTheme() {
    enum class AppMode { Default, AllowDark, ForceDark, ForceLight };
    using SetPreferredAppMode = AppMode(WINAPI*)(AppMode);
    using FlushMenuThemes = void(WINAPI*)();

    static HMODULE uxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!uxtheme) return;

    static auto setMode = reinterpret_cast<SetPreferredAppMode>(GetProcAddress(uxtheme, MAKEINTRESOURCEA(135)));
    static auto flush = reinterpret_cast<FlushMenuThemes>(GetProcAddress(uxtheme, MAKEINTRESOURCEA(136)));
    if (!setMode || !flush) return;

    auto theme = jsonString(settings(), L"theme", L"system");
    setMode(theme == L"dark" ? AppMode::ForceDark : theme == L"light" ? AppMode::ForceLight : AppMode::AllowDark);
    flush();
}

void createTray() {
    applyMenuTheme();
    addIcon();
}

void removeTray() {
    if (!isAdded) return;

    auto data = iconData();
    Shell_NotifyIconW(NIM_DELETE, &data);
    isAdded = false;
}

void refreshTray() {
    postToMainThread([] {
        if (!isAdded) return;

        auto data = iconData();
        data.uFlags = NIF_TIP | NIF_SHOWTIP;
        wcsncpy_s(data.szTip, trayTooltip().c_str(), _TRUNCATE);
        Shell_NotifyIconW(NIM_MODIFY, &data);
    });
}

bool handleTrayMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == taskbarCreated) {
        isAdded = false;
        addIcon();
        return true;
    }

    if (message == WM_TIMER && wParam == CLICK_TIMER) {
        KillTimer(mainWindow(), CLICK_TIMER);
        handleTrayClick();
        return true;
    }

    if (message != TRAY_MESSAGE) return false;

    switch (LOWORD(lParam)) {
    case NIN_SELECT:
    case NIN_KEYSELECT:
        if (GetTickCount64() - lastDoubleClick < GetDoubleClickTime()) break;
        SetTimer(mainWindow(), CLICK_TIMER, GetDoubleClickTime(), nullptr);
        break;
    case WM_LBUTTONDBLCLK:
        lastDoubleClick = GetTickCount64();
        KillTimer(mainWindow(), CLICK_TIMER);
        showMainWindow();
        break;
    case WM_MBUTTONUP:
        runAction(L"uploadClipboard");
        break;
    case WM_CONTEXTMENU:
        showMenu();
        break;
    }

    return true;
}

}

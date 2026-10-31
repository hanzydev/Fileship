#include "win32.h"

#include <host/app.h>
#include <host/rpc.h>
#include <shared/i18n.h>
#include <shared/log.h>

#include <objbase.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <winrt/base.h>

namespace fileship::host {

namespace {

HWND window = nullptr;

std::vector<std::wstring> commandLine() {
    int count = 0;
    auto values = CommandLineToArgvW(GetCommandLineW(), &count);
    std::vector<std::wstring> arguments(values + 1, values + count);
    LocalFree(values);
    return arguments;
}

LRESULT CALLBACK handleMessage(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) {
    if (handleTrayMessage(message, wParam, lParam)) return 0;

    switch (message) {
    case WM_APP:
        runMainQueue();
        return 0;
    case WM_HOTKEY:
        handleHotkey(static_cast<int>(wParam));
        return 0;
    case WM_SETTINGCHANGE:
        if (lParam && wcscmp(reinterpret_cast<wchar_t const*>(lParam), L"intl") == 0) applySettings();
        if (lParam && wcscmp(reinterpret_cast<wchar_t const*>(lParam), L"ImmersiveColorSet") == 0) applyMenuTheme();
        break;
    case WM_CLOSE:
        quitApp();
        return 0;
    case WM_QUERYENDSESSION:
        return TRUE;
    case WM_ENDSESSION:
        if (wParam) quitApp();
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(handle, message, wParam, lParam);
}

void createMainWindow() {
    WNDCLASSEXW windowClass{sizeof(windowClass)};
    windowClass.lpfnWndProc = handleMessage;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = L"FileshipHost";
    RegisterClassExW(&windowClass);

    window = CreateWindowExW(WS_EX_TOOLWINDOW, L"FileshipHost", L"Fileship", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
                             windowClass.hInstance, nullptr);
}

constexpr DWORD MIN_WINDOWS_BUILD = 26100;

bool isSupportedWindows() {
    using RtlGetVersion = LONG(WINAPI*)(OSVERSIONINFOW*);

    auto getVersion = reinterpret_cast<RtlGetVersion>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
    OSVERSIONINFOW version{sizeof(version)};
    if (!getVersion || getVersion(&version) != 0) return false;

    return version.dwMajorVersion >= 10 && version.dwBuildNumber >= MIN_WINDOWS_BUILD;
}

}

HWND mainWindow() {
    return window;
}

}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    using namespace fileship;
    using namespace fileship::host;

    parseOptions(commandLine());

    HANDLE mutex = CreateMutexW(nullptr, TRUE, (L"Local\\FileshipHost" + profileSuffix()).c_str());
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (shouldForwardLaunch()) {
            AllowSetForegroundWindow(ASFW_ANY);
            forwardToRunningHost(launchOptions().arguments);
        }

        return 0;
    }

    winrt::init_apartment(winrt::apartment_type::single_threaded);
    SetCurrentProcessExplicitAppUserModelID(appUserModelId().c_str());

    initHost();

    if (!isSupportedWindows()) {
        logError(L"This Windows version is not supported");
        MessageBoxW(nullptr, t(L"app.unsupportedWindows").c_str(), L"Fileship", MB_OK | MB_ICONERROR);
        return 1;
    }

    createMainWindow();
    runMainQueue();
    startHost();

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (message.message == WM_APP && !message.hwnd) runMainQueue();
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    stopRpcServer();
    logInfo(L"Stopped");
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    return 0;
}

#include "app.h"
#include "rpc.h"

#include <host/harbor.h>
#include <host/recording.h>
#include <shared/i18n.h>
#include <shared/log.h>

#include <shellapi.h>
#include <objbase.h>
#include <shobjidl.h>
#include <winrt/base.h>

namespace fileship::host {

namespace {

HWND window = nullptr;
bool isQuitting = false;

std::vector<std::wstring> commandLine() {
    int count = 0;
    auto values = CommandLineToArgvW(GetCommandLineW(), &count);
    std::vector<std::wstring> arguments(values + 1, values + count);
    LocalFree(values);
    return arguments;
}

void parseOptions(std::vector<std::wstring> const& arguments) {
    auto& options = launchOptions();
    std::vector<std::wstring> rest;

    for (size_t index = 0; index < arguments.size(); index++) {
        auto const& argument = arguments[index];
        bool hasValue = index + 1 < arguments.size();

        if (argument == L"--hidden") options.hidden = true;
        else if (argument == L"--dev") options.dev = true;
        else if (argument == L"--user-data-dir" && hasValue) options.userData = arguments[++index];
        else if (argument.starts_with(L"--user-data-dir=")) options.userData = argument.substr(16);
        else if (argument == L"--renderer-url" && hasValue) options.rendererUrl = arguments[++index];
        else if (argument == L"--electron" && hasValue) options.electronCommand = arguments[++index];
        else if (argument == L"--update-feed" && hasValue) options.updateFeed = arguments[++index];
        else rest.push_back(argument);
    }

    if (options.hidden) rest.push_back(L"--hidden");
    options.arguments = rest;
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

void setOverlayStore() {
    ui::setOverlayPreferencesStore({
        [] {
            auto value = storeValue(L"overlay");
            return value.ValueType() == JsonValueType::Null ? std::string() : stringify(value);
        },
        [](std::string const& text) {
            JsonValue value{nullptr};
            if (JsonValue::TryParse(toWide(text), value)) setStoreValue(L"overlay", value);
        },
    });
}

}

HWND mainWindow() {
    return window;
}

void quitApp() {
    if (isQuitting) return;
    isQuitting = true;

    logInfo(L"Quitting");
    broadcast(L"app:quit", JsonValue::CreateNullValue());
    unregisterHotkeys();
    removeTray();

    runAsync([] {
        recording::quitRecorder();
        harbor::stopHarbor();
        installOnQuit();
        postToMainThread([] { DestroyWindow(window); });
    });
}

}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    using namespace fileship;
    using namespace fileship::host;

    parseOptions(commandLine());

    HANDLE mutex = CreateMutexW(nullptr, TRUE, mutexName().c_str());
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (std::find(launchOptions().arguments.begin(), launchOptions().arguments.end(), L"--hidden") ==
                launchOptions().arguments.end() ||
            launchOptions().arguments.size() > 1) {
            AllowSetForegroundWindow(ASFW_ANY);
            forwardToRunningHost(launchOptions().arguments);
        }

        return 0;
    }

    winrt::init_apartment(winrt::apartment_type::single_threaded);
    SetCurrentProcessExplicitAppUserModelID(appUserModelId().c_str());

    initLog(userDataPath() / L"logs" / L"host.log");
    initI18n(resourcesPath() / L"i18n");
    logInfo(L"Fileship host " + appVersion() + (isPackaged() ? L"" : L" (development)"));

    loadStore();
    setLanguage(resolveLanguage(jsonString(settings(), L"language", L"system")));
    createMainWindow();
    runMainQueue();
    initNotifications();
    setOverlayStore();

    registerHandlers();
    harbor::registerHarborHandlers();
    recording::registerRecordingHandlers();
    startRpcServer();

    applySettings();
    registerShellMenu();
    createTray();
    registerHotkeys();
    startUpdater();
    runAsync([] { harbor::startHarbor(); });

    auto const& arguments = launchOptions().arguments;
    if (!handleLaunch(arguments, std::filesystem::current_path()) && !launchOptions().hidden) showMainWindow();

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

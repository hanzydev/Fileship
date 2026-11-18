#include "host.h"

#include <host/platform.h>
#include <shared/log.h>
#include <shared/platform.h>

#include <sddl.h>
#include <shellapi.h>
#include <shlobj.h>

#include <algorithm>

#pragma comment(lib, "version.lib")

namespace fileship::host {

namespace {

DWORD mainThreadId = GetCurrentThreadId();

std::wstring currentUserSid() {
    HANDLE processToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &processToken)) return L"unknown";

    DWORD size = 0;
    GetTokenInformation(processToken, TokenUser, nullptr, 0, &size);
    std::vector<uint8_t> buffer(size);
    std::wstring result = L"unknown";

    if (GetTokenInformation(processToken, TokenUser, buffer.data(), size, &size)) {
        LPWSTR text = nullptr;
        if (ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid, &text)) {
            result = text;
            LocalFree(text);
        }
    }

    CloseHandle(processToken);
    return result;
}

std::filesystem::path knownFolder(KNOWNFOLDERID const& id) {
    PWSTR folder = nullptr;
    SHGetKnownFolderPath(id, 0, nullptr, &folder);
    std::filesystem::path result(folder);
    CoTaskMemFree(folder);
    return result;
}

void CALLBACK runWork(PTP_CALLBACK_INSTANCE, void* context) {
    std::unique_ptr<std::function<void()>> work(static_cast<std::function<void()>*>(context));

    try {
        (*work)();
    } catch (...) {
        logError(L"Background task failed: " + exceptionMessage());
    }
}

void CALLBACK runTimer(PTP_CALLBACK_INSTANCE instance, void* context, PTP_TIMER timer) {
    runWork(instance, context);
    CloseThreadpoolTimer(timer);
}

}

std::filesystem::path executablePath() {
    std::wstring buffer(MAX_PATH, L'\0');

    while (true) {
        DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length < buffer.size()) return std::filesystem::path(buffer.substr(0, length));
        buffer.resize(buffer.size() * 2);
    }
}

std::wstring pipeName() {
    if (!launchOptions().pipe.empty()) return launchOptions().pipe;
    return L"\\\\.\\pipe\\fileship-host-" + currentUserSid() + profileSuffix();
}

std::wstring mutexName() {
    return L"Local\\FileshipHost" + profileSuffix();
}

std::wstring appUserModelId() {
    return isPackaged() ? L"dev.hanzy.fileship" : L"dev.hanzy.fileship.dev";
}

std::filesystem::path defaultUserDataPath() {
    return knownFolder(FOLDERID_RoamingAppData) / L"Fileship";
}

std::filesystem::path picturesPath() {
    return knownFolder(FOLDERID_Pictures);
}

void describePlatform(JsonObject& info) {
    info.SetNamedValue(L"platform", JsonValue::CreateStringValue(L"windows"));
    info.SetNamedValue(L"hotkeys", JsonValue::CreateStringValue(L"native"));
}

std::wstring platformName() {
    return L"Windows";
}

std::wstring electronExecutable() {
    return L"Fileship.exe";
}

LocalTime localTime() {
    SYSTEMTIME now{};
    GetLocalTime(&now);
    return {now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds};
}

std::wstring appVersion() {
    static std::wstring const version = [] {
        auto path = hostExecutable().wstring();
        DWORD handle = 0;
        DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
        std::vector<uint8_t> data(size);
        wchar_t* value = nullptr;
        UINT length = 0;

        if (size && GetFileVersionInfoW(path.c_str(), 0, size, data.data()) &&
            VerQueryValueW(data.data(), L"\\StringFileInfo\\040904b0\\ProductVersion",
                           reinterpret_cast<void**>(&value), &length) &&
            length) {
            return std::wstring(value);
        }

        return std::wstring(L"0.0.0");
    }();

    return version;
}

void wakeMainThread() {
    if (auto window = mainWindow()) PostMessageW(window, WM_APP, 0, 0);
    else PostThreadMessageW(mainThreadId, WM_APP, 0, 0);
}

void runAsync(std::function<void()> work) {
    auto context = new std::function<void()>(std::move(work));
    if (!TrySubmitThreadpoolCallback(runWork, context, nullptr)) {
        delete context;
        logError(L"Could not queue a background task");
    }
}

void runAfter(int delayMs, std::function<void()> work) {
    auto context = new std::function<void()>(std::move(work));
    auto timer = CreateThreadpoolTimer(runTimer, context, nullptr);

    if (!timer) {
        delete context;
        logError(L"Could not create a timer");
        return;
    }

    ULARGE_INTEGER due{};
    due.QuadPart = static_cast<ULONGLONG>(-static_cast<LONGLONG>(std::max(delayMs, 0)) * 10000);

    FILETIME time{};
    time.dwLowDateTime = due.LowPart;
    time.dwHighDateTime = due.HighPart;
    SetThreadpoolTimer(timer, &time, 0, 0);
}

bool replaceFile(std::filesystem::path const& from, std::filesystem::path const& to) {
    return MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
}

void openExternal(std::wstring const& url) {
    ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void showItemInFolder(std::filesystem::path const& path) {
    postToMainThread([path] {
        PIDLIST_ABSOLUTE item = ILCreateFromPathW(path.c_str());
        if (!item) return;

        SHOpenFolderAndSelectItems(item, 0, nullptr, 0);
        ILFree(item);
    });
}

bool isSystemDark() {
    DWORD value = 0;
    DWORD size = sizeof(value);

    return RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                        L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &size) == ERROR_SUCCESS &&
           !value;
}

bool isNameBefore(std::wstring const& a, std::wstring const& b) {
    return CompareStringEx(language().c_str(), LINGUISTIC_IGNORECASE, a.c_str(), -1, b.c_str(), -1, nullptr, nullptr,
                           0) == CSTR_LESS_THAN;
}

}

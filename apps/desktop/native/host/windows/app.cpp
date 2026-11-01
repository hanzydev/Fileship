#include "win32.h"

#include <host/app.h>
#include <shared/log.h>

#include <sddl.h>
#include <shellapi.h>
#include <shlobj.h>

namespace fileship::host {

namespace {

DWORD mainThreadId = GetCurrentThreadId();

std::filesystem::path knownFolder(KNOWNFOLDERID const& id) {
    PWSTR folder = nullptr;
    SHGetKnownFolderPath(id, 0, nullptr, &folder);
    std::filesystem::path result(folder);
    CoTaskMemFree(folder);
    return result;
}

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

void CALLBACK runWork(PTP_CALLBACK_INSTANCE, void* context) {
    std::unique_ptr<std::function<void()>> work(static_cast<std::function<void()>*>(context));

    try {
        (*work)();
    } catch (std::exception const& error) {
        logError(L"Background task failed: " + errorMessage(error));
    } catch (...) {
        logError(L"Background task failed: " + describeException(std::current_exception()));
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

std::filesystem::path defaultUserDataPath() {
    return knownFolder(FOLDERID_RoamingAppData) / L"Fileship";
}

std::filesystem::path picturesFolder() {
    return knownFolder(FOLDERID_Pictures);
}

std::filesystem::path updatesFolder() {
    return knownFolder(FOLDERID_LocalAppData) / L"fileship-updater" / L"pending";
}

std::filesystem::path localDataFolder() {
    return knownFolder(FOLDERID_LocalAppData);
}

std::filesystem::path homeFolder() {
    return knownFolder(FOLDERID_Profile);
}

std::wstring executableName(std::wstring const& base) {
    return base + L".exe";
}

std::wstring platformName() {
    return L"Windows";
}

std::wstring launchCommand() {
    auto launcher = isPackaged() ? resourcesPath().parent_path().parent_path() / L"Fileship.exe" : hostExecutable();
    return L"\"" + launcher.wstring() + L"\"";
}

JsonObject platformCapabilities() {
    JsonObject voiceProcessing;
    for (auto key : {L"noiseSuppression", L"echoCancellation", L"autoGainControl"})
        voiceProcessing.SetNamedValue(key, JsonValue::CreateBooleanValue(true));

    JsonArray videoSources;
    for (auto source : {L"auto", L"ddagrab", L"gdigrab"}) videoSources.Append(JsonValue::CreateStringValue(source));

    JsonObject capabilities;
    for (auto key : {L"windowDetection", L"controlDetection", L"snippingToolKey", L"harbor", L"recordingBorder"})
        capabilities.SetNamedValue(key, JsonValue::CreateBooleanValue(true));
    capabilities.SetNamedValue(L"hotkeys", JsonValue::CreateStringValue(L"native"));
    capabilities.SetNamedValue(L"voiceProcessing", voiceProcessing);
    capabilities.SetNamedValue(L"videoSources", videoSources);
    capabilities.SetNamedValue(L"updates", JsonValue::CreateStringValue(isPackaged() ? L"auto" : L"none"));
    return capabilities;
}

bool hasPackagedLayout() {
    auto resources = resourcesPath().parent_path();
    return resources.filename() == L"resources" && std::filesystem::exists(resources.parent_path() / L"Fileship.exe");
}

std::wstring defaultPipeName(std::wstring const& suffix) {
    return L"\\\\.\\pipe\\fileship-host-" + currentUserSid() + suffix;
}

std::wstring appUserModelId() {
    return isPackaged() ? L"dev.hanzy.fileship" : L"dev.hanzy.fileship.dev";
}

void wakeMainThread() {
    if (auto window = mainWindow()) PostMessageW(window, WM_APP, 0, 0);
    else PostThreadMessageW(mainThreadId, WM_APP, 0, 0);
}

void stopMainLoop() {
    DestroyWindow(mainWindow());
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

bool systemUsesLightTheme() {
    DWORD value = 0;
    DWORD size = sizeof(value);
    return RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                        L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS ||
           value;
}

bool textLess(std::wstring const& a, std::wstring const& b) {
    return CompareStringEx(language().c_str(), LINGUISTIC_IGNORECASE, a.c_str(), -1, b.c_str(), -1, nullptr, nullptr,
                           0) == CSTR_LESS_THAN;
}

void openWebUrl(std::wstring const& url) {
    auto lower = url.substr(0, 8);
    for (auto& character : lower) character = static_cast<wchar_t>(towlower(character));
    if (lower.rfind(L"http://", 0) != 0 && lower.rfind(L"https://", 0) != 0) return;

    ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void openFolder(std::filesystem::path const& path) {
    ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void showItemInFolder(std::filesystem::path const& path) {
    postToMainThread([path] {
        PIDLIST_ABSOLUTE item = ILCreateFromPathW(path.c_str());
        if (!item) return;

        SHOpenFolderAndSelectItems(item, 0, nullptr, 0);
        ILFree(item);
    });
}

}

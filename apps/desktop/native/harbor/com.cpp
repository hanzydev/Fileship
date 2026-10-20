#include "com.h"

#include "share.h"
#include "status.h"
#include "thumbnails.h"

#include <shlobj.h>
#include <wincrypt.h>

#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>

using namespace winrt;
using Clock = std::chrono::steady_clock;

namespace harbor {

namespace {

constexpr auto IDLE_TIME = std::chrono::seconds(30);

struct ShellClass {
    CLSID clsid;
    wchar_t const* syncRootValue;
    com_ptr<IClassFactory> (*factory)();
};

std::vector<ShellClass> const& shellClasses() {
    static std::vector<ShellClass> const classes{
        {CLSID_FileshipThumbnails, L"ThumbnailProvider", thumbnailFactory},
        {CLSID_FileshipShare, L"ShareHandler", shareFactory},
        {CLSID_FileshipStatus, L"StorageProviderStatusUISourceFactory", statusFactory},
    };
    return classes;
}

std::atomic<long> locks = 0;
std::atomic<bool> hosted = false;
std::atomic<Clock::rep> lastActivity = Clock::now().time_since_epoch().count();
std::mutex cookiesMutex;
std::vector<DWORD> cookies;

void touch() {
    lastActivity = Clock::now().time_since_epoch().count();
}

struct Factory : implements<Factory, IClassFactory> {
    com_ptr<IClassFactory> inner;

    explicit Factory(com_ptr<IClassFactory> value) : inner(std::move(value)) {}

    HRESULT __stdcall CreateInstance(IUnknown* outer, REFIID iid, void** result) noexcept final {
        touch();
        return inner->CreateInstance(outer, iid, result);
    }

    HRESULT __stdcall LockServer(BOOL lock) noexcept final {
        lock ? serverAddRef() : serverRelease();
        return S_OK;
    }
};

std::wstring guidText(GUID const& guid) {
    wchar_t text[39]{};
    StringFromGUID2(guid, text, 39);
    return text;
}

std::wstring classKey(GUID const& clsid) {
    return L"Software\\Classes\\CLSID\\" + guidText(clsid);
}

void setString(HKEY root, std::wstring const& key, wchar_t const* name, std::wstring const& value) {
    check_win32(RegSetKeyValueW(root, key.c_str(), name, REG_SZ, value.c_str(),
                                static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t))));
}

std::filesystem::path sessionPath() {
    PWSTR folder = nullptr;
    check_hresult(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &folder));
    std::filesystem::path path = std::filesystem::path(folder) / L"Fileship" / L"harbor.json";
    CoTaskMemFree(folder);
    return path;
}

std::wstring protect(std::wstring const& text) {
    auto bytes = toUtf8(text);
    DATA_BLOB input{static_cast<DWORD>(bytes.size()), reinterpret_cast<BYTE*>(bytes.data())};
    DATA_BLOB output{};

    if (!CryptProtectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        throw_last_error();
    }

    DWORD length = 0;
    DWORD flags = CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF;
    CryptBinaryToStringW(output.pbData, output.cbData, flags, nullptr, &length);
    std::wstring encoded(length, L'\0');
    CryptBinaryToStringW(output.pbData, output.cbData, flags, encoded.data(), &length);
    LocalFree(output.pbData);
    encoded.resize(length);
    return encoded;
}

std::wstring unprotect(std::wstring const& encoded) {
    DWORD length = 0;
    if (!CryptStringToBinaryW(encoded.c_str(), 0, CRYPT_STRING_BASE64, nullptr, &length, nullptr, nullptr)) {
        throw_last_error();
    }

    std::vector<BYTE> bytes(length);
    CryptStringToBinaryW(encoded.c_str(), 0, CRYPT_STRING_BASE64, bytes.data(), &length, nullptr, nullptr);

    DATA_BLOB input{length, bytes.data()};
    DATA_BLOB output{};

    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        throw_last_error();
    }

    std::string text(reinterpret_cast<char const*>(output.pbData), output.cbData);
    LocalFree(output.pbData);
    return toWide(text);
}

}

std::wstring executablePath() {
    std::wstring path(MAX_PATH, L'\0');

    for (;;) {
        DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (length < path.size()) return path.substr(0, length);
        path.resize(path.size() * 2);
    }
}

void serverAddRef() {
    ++locks;
    touch();
}

void serverRelease() {
    --locks;
    touch();
}

void registerClassObjects() {
    std::lock_guard lock(cookiesMutex);
    if (!cookies.empty()) return;

    for (auto const& shellClass : shellClasses()) {
        auto factory = make<Factory>(shellClass.factory());
        DWORD cookie = 0;
        HRESULT result = CoRegisterClassObject(shellClass.clsid, factory.get(), CLSCTX_LOCAL_SERVER,
                                               REGCLS_MULTIPLEUSE, &cookie);

        if (FAILED(result)) {
            log(L"Could not register " + guidText(shellClass.clsid) + L": " + std::to_wstring(result));
            continue;
        }

        cookies.push_back(cookie);
    }
}

void revokeClassObjects() {
    std::lock_guard lock(cookiesMutex);
    for (auto cookie : cookies) CoRevokeClassObject(cookie);
    cookies.clear();
}

void writeShellRegistration() {
    auto server = L"\"" + executablePath() + L"\"";
    auto syncRootKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\SyncRootManager\\" + syncRootId();

    for (auto const& shellClass : shellClasses()) {
        auto key = classKey(shellClass.clsid);
        setString(HKEY_CURRENT_USER, key, nullptr, L"Fileship Harbor");
        setString(HKEY_CURRENT_USER, key + L"\\LocalServer32", nullptr, server);

        if (shellClass.syncRootValue) {
            setString(HKEY_LOCAL_MACHINE, syncRootKey, shellClass.syncRootValue, guidText(shellClass.clsid));
        }
    }
}

void removeShellRegistration() {
    for (auto const& shellClass : shellClasses()) {
        RegDeleteTreeW(HKEY_CURRENT_USER, classKey(shellClass.clsid).c_str());
    }
}

void waitUntilIdle() {
    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto idle = Clock::now() - Clock::time_point(Clock::duration(lastActivity.load()));
        if (locks <= 0 && idle >= IDLE_TIME) return;
    }
}

void setHosted() {
    hosted = true;
}

void requestAction(std::wstring const& action, std::vector<std::wstring> const& paths) {
    if (hosted) {
        JsonArray items;
        for (auto const& path : paths) items.Append(JsonValue::CreateStringValue(path));

        JsonObject message;
        message.SetNamedValue(L"action", JsonValue::CreateStringValue(action));
        message.SetNamedValue(L"paths", items);
        sendEvent(L"action", message);
        return;
    }

    auto const& app = config().appPath;
    if (app.empty()) return;

    auto command = L"\"" + app + L"\" --harbor-action " + action;
    for (auto const& path : paths) command += L" \"" + path + L"\"";

    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION process{};

    if (!CreateProcessW(app.c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup,
                        &process)) {
        log(L"Could not start Fileship: " + std::to_wstring(GetLastError()));
        return;
    }

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
}

void saveSession() {
    auto const& settings = config();

    JsonObject session;
    session.SetNamedValue(L"instanceUrl", JsonValue::CreateStringValue(settings.instanceUrl));
    session.SetNamedValue(L"root", JsonValue::CreateStringValue(settings.root));
    session.SetNamedValue(L"syncRootId", JsonValue::CreateStringValue(settings.syncRootId));
    session.SetNamedValue(L"userAgent", JsonValue::CreateStringValue(settings.userAgent));
    session.SetNamedValue(L"appPath", JsonValue::CreateStringValue(settings.appPath));
    session.SetNamedValue(L"labels", settings.labels);
    session.SetNamedValue(L"token", JsonValue::CreateStringValue(protect(token())));

    auto path = sessionPath();
    auto temporary = std::filesystem::path(path).replace_extension(L".tmp");
    std::filesystem::create_directories(path.parent_path());

    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        file << toUtf8(std::wstring(session.Stringify()));
        if (!file) throw std::runtime_error("Could not write the Harbor session");
    }

    if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        throw_last_error();
    }
}

bool loadSession() {
    try {
        std::ifstream file(sessionPath(), std::ios::binary);
        if (!file) return false;

        std::stringstream content;
        content << file.rdbuf();

        JsonObject session;
        if (!JsonObject::TryParse(toWide(content.str()), session)) return false;

        auto& settings = config();
        settings.instanceUrl = session.GetNamedString(L"instanceUrl");
        settings.root = session.GetNamedString(L"root");
        settings.syncRootId = session.GetNamedString(L"syncRootId");
        settings.userAgent = session.GetNamedString(L"userAgent");
        settings.appPath = session.GetNamedString(L"appPath", L"");
        settings.labels = session.GetNamedObject(L"labels", JsonObject());
        setToken(unprotect(std::wstring(session.GetNamedString(L"token"))));
        return true;
    } catch (hresult_error const& error) {
        log(L"Could not load the Harbor session: " + std::wstring(error.message()));
        return false;
    }
}

void removeSession() {
    std::error_code error;
    std::filesystem::remove(sessionPath(), error);
}

}

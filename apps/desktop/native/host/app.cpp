#include "app.h"

#include <shared/log.h>

#include <bcrypt.h>
#include <shellapi.h>
#include <sddl.h>
#include <shlobj.h>
#include <wincrypt.h>

#include <array>
#include <chrono>
#include <deque>
#include <fstream>
#include <mutex>
#include <sstream>

#pragma comment(lib, "version.lib")

namespace fileship::host {

namespace {

std::mutex queueMutex;
std::deque<std::function<void()>> mainQueue;
DWORD mainThreadId = GetCurrentThreadId();

std::filesystem::path executablePath() {
    std::wstring buffer(MAX_PATH, L'\0');

    while (true) {
        DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length < buffer.size()) return std::filesystem::path(buffer.substr(0, length));
        buffer.resize(buffer.size() * 2);
    }
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

std::wstring profileSuffix() {
    if (launchOptions().userData.empty()) return {};

    auto path = std::filesystem::absolute(launchOptions().userData).lexically_normal().wstring();
    while (!path.empty() && (path.back() == L'\\' || path.back() == L'/')) path.pop_back();

    uint32_t hash = 2166136261u;
    for (auto character : path) {
        hash ^= static_cast<uint32_t>(towlower(character));
        hash *= 16777619u;
    }

    wchar_t text[16]{};
    swprintf_s(text, L"-%08x", hash);
    return text;
}

void CALLBACK runWork(PTP_CALLBACK_INSTANCE, void* context) {
    std::unique_ptr<std::function<void()>> work(static_cast<std::function<void()>*>(context));

    try {
        (*work)();
    } catch (std::exception const& error) {
        logError(L"Background task failed: " + errorMessage(error));
    } catch (winrt::hresult_error const& error) {
        logError(L"Background task failed: " + std::wstring(error.message()));
    }
}

void CALLBACK runTimer(PTP_CALLBACK_INSTANCE instance, void* context, PTP_TIMER timer) {
    runWork(instance, context);
    CloseThreadpoolTimer(timer);
}

}

LaunchOptions& launchOptions() {
    static LaunchOptions options;
    return options;
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

std::filesystem::path userDataPath() {
    if (!launchOptions().userData.empty()) return std::filesystem::absolute(launchOptions().userData);

    PWSTR folder = nullptr;
    SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &folder);
    std::filesystem::path result = std::filesystem::path(folder) / L"Fileship";
    CoTaskMemFree(folder);
    return result;
}

std::filesystem::path hostExecutable() {
    static auto const path = executablePath();
    return path;
}

std::filesystem::path resourcesPath() {
    return hostExecutable().parent_path();
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

bool isPackaged() {
    static bool const packaged = [] {
        auto resources = resourcesPath().parent_path();
        return resources.filename() == L"resources" &&
               std::filesystem::exists(resources.parent_path() / L"Fileship.exe");
    }();

    return packaged && !launchOptions().dev;
}

std::wstring desktopUserAgent() {
    return L"FileshipDesktop/" + appVersion() + L" (Windows)";
}

void postToMainThread(std::function<void()> work) {
    {
        std::lock_guard lock(queueMutex);
        mainQueue.push_back(std::move(work));
    }

    if (auto window = mainWindow()) PostMessageW(window, WM_APP, 0, 0);
    else PostThreadMessageW(mainThreadId, WM_APP, 0, 0);
}

bool isMainThread() {
    return GetCurrentThreadId() == mainThreadId;
}

void runMainQueue() {
    while (true) {
        std::function<void()> work;

        {
            std::lock_guard lock(queueMutex);
            if (mainQueue.empty()) return;
            work = std::move(mainQueue.front());
            mainQueue.pop_front();
        }

        try {
            work();
        } catch (std::exception const& error) {
            logError(L"Main thread task failed: " + errorMessage(error));
        } catch (winrt::hresult_error const& error) {
            logError(L"Main thread task failed: " + std::wstring(error.message()));
        }
    }
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

int64_t nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::runtime_error translatedError(std::wstring const& message) {
    return std::runtime_error(toUtf8(message));
}

bool isQuotaExceeded(std::exception const& error) {
    auto api = dynamic_cast<ApiError const*>(&error);
    return api && api->data && api->data.HasKey(L"quotaExceeded") && jsonBool(api->data, L"quotaExceeded");
}

bool isUnauthorized(std::exception const& error) {
    auto api = dynamic_cast<ApiError const*>(&error);
    return api && api->status == 401;
}

std::wstring errorMessage(std::exception const& error) {
    return toWide(error.what());
}

std::wstring jsonString(JsonObject const& object, wchar_t const* key, std::wstring const& fallback) {
    if (!object || !object.HasKey(key)) return fallback;

    auto value = object.GetNamedValue(key);
    return value.ValueType() == JsonValueType::String ? std::wstring(value.GetString()) : fallback;
}

bool jsonBool(JsonObject const& object, wchar_t const* key, bool fallback) {
    if (!object || !object.HasKey(key)) return fallback;

    auto value = object.GetNamedValue(key);
    return value.ValueType() == JsonValueType::Boolean ? value.GetBoolean() : fallback;
}

double jsonNumber(JsonObject const& object, wchar_t const* key, double fallback) {
    if (!object || !object.HasKey(key)) return fallback;

    auto value = object.GetNamedValue(key);
    return value.ValueType() == JsonValueType::Number ? value.GetNumber() : fallback;
}

JsonObject jsonObject(JsonObject const& object, wchar_t const* key) {
    if (!object || !object.HasKey(key)) return JsonObject();

    auto value = object.GetNamedValue(key);
    return value.ValueType() == JsonValueType::Object ? value.GetObject() : JsonObject();
}

JsonValue stringOrNull(std::wstring const& value) {
    return value.empty() ? JsonValue::CreateNullValue() : JsonValue::CreateStringValue(value);
}

JsonValue stringOrNull(std::optional<std::wstring> const& value) {
    return value ? JsonValue::CreateStringValue(*value) : JsonValue::CreateNullValue();
}

JsonObject cloneObject(JsonObject const& object) {
    return JsonObject::Parse(object.Stringify());
}

namespace {

void writePretty(std::wstring& out, IJsonValue const& value, int depth) {
    auto indent = [&](int level) { out.append(static_cast<size_t>(level) * 2, L' '); };

    switch (value.ValueType()) {
    case JsonValueType::Object: {
        auto object = value.GetObject();
        if (!object.Size()) {
            out += L"{}";
            return;
        }

        out += L"{\n";
        uint32_t index = 0;
        for (auto const& pair : object) {
            indent(depth + 1);
            out += JsonValue::CreateStringValue(pair.Key()).Stringify();
            out += L": ";
            writePretty(out, pair.Value(), depth + 1);
            out += ++index < object.Size() ? L",\n" : L"\n";
        }
        indent(depth);
        out += L"}";
        return;
    }
    case JsonValueType::Array: {
        auto array = value.GetArray();
        if (!array.Size()) {
            out += L"[]";
            return;
        }

        out += L"[\n";
        for (uint32_t index = 0; index < array.Size(); index++) {
            indent(depth + 1);
            writePretty(out, array.GetAt(index), depth + 1);
            out += index + 1 < array.Size() ? L",\n" : L"\n";
        }
        indent(depth);
        out += L"]";
        return;
    }
    default:
        out += value.Stringify();
    }
}

}

std::string prettyJson(IJsonValue const& value) {
    std::wstring out;
    writePretty(out, value, 0);
    return toUtf8(out);
}

std::wstring randomHex(size_t bytes) {
    std::vector<uint8_t> random(bytes);
    BCryptGenRandom(nullptr, random.data(), static_cast<ULONG>(random.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    std::wstring result;
    for (auto byte : random) {
        wchar_t hex[3]{};
        swprintf_s(hex, L"%02x", byte);
        result += hex;
    }

    return result;
}

std::wstring randomUuid() {
    std::array<uint8_t, 16> bytes{};
    BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    bytes[6] = static_cast<uint8_t>((bytes[6] & 0x0f) | 0x40);
    bytes[8] = static_cast<uint8_t>((bytes[8] & 0x3f) | 0x80);

    std::wstring result;
    for (size_t index = 0; index < bytes.size(); index++) {
        if (index == 4 || index == 6 || index == 8 || index == 10) result += L'-';

        wchar_t hex[3]{};
        swprintf_s(hex, L"%02x", bytes[index]);
        result += hex;
    }

    return result;
}

std::wstring base64(void const* data, size_t size) {
    if (!size) return {};

    DWORD length = 0;
    auto bytes = static_cast<BYTE const*>(data);
    CryptBinaryToStringW(bytes, static_cast<DWORD>(size), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr,
                         &length);

    std::wstring result(length, L'\0');
    CryptBinaryToStringW(bytes, static_cast<DWORD>(size), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                         result.data(), &length);
    result.resize(length);
    return result;
}

std::vector<uint8_t> fromBase64(std::wstring const& text) {
    DWORD length = 0;
    if (!CryptStringToBinaryW(text.c_str(), static_cast<DWORD>(text.size()), CRYPT_STRING_BASE64, nullptr,
                              &length, nullptr, nullptr))
        return {};

    std::vector<uint8_t> result(length);
    if (!CryptStringToBinaryW(text.c_str(), static_cast<DWORD>(text.size()), CRYPT_STRING_BASE64, result.data(),
                              &length, nullptr, nullptr))
        return {};

    result.resize(length);
    return result;
}

std::vector<char> readFileBytes(std::filesystem::path const& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};

    return std::vector<char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

bool writeFileAtomic(std::filesystem::path const& path, std::string const& content) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);

    auto temporary = std::filesystem::path(path).concat(L".tmp");

    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        if (!file) return false;
        file.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!file) return false;
    }

    return MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
}

std::wstring argumentOf(std::vector<std::wstring> const& arguments, std::wstring const& name) {
    for (size_t index = 0; index + 1 < arguments.size(); index++) {
        if (arguments[index] == name) return arguments[index + 1];
    }

    return {};
}

std::wstring mimeTypeOf(std::filesystem::path const& path) {
    static std::map<std::wstring, std::wstring> const types{
        {L".png", L"image/png"},        {L".jpg", L"image/jpeg"},       {L".jpeg", L"image/jpeg"},
        {L".gif", L"image/gif"},        {L".webp", L"image/webp"},      {L".bmp", L"image/bmp"},
        {L".svg", L"image/svg+xml"},    {L".mp4", L"video/mp4"},        {L".webm", L"video/webm"},
        {L".mkv", L"video/x-matroska"}, {L".mov", L"video/quicktime"},  {L".mp3", L"audio/mpeg"},
        {L".wav", L"audio/wav"},        {L".ogg", L"audio/ogg"},        {L".pdf", L"application/pdf"},
        {L".zip", L"application/zip"},  {L".txt", L"text/plain"},       {L".md", L"text/markdown"},
        {L".json", L"application/json"}, {L".html", L"text/html"},      {L".css", L"text/css"},
        {L".js", L"text/javascript"},   {L".ts", L"text/plain"},
    };

    auto extension = path.extension().wstring();
    for (auto& character : extension) character = static_cast<wchar_t>(towlower(character));

    auto found = types.find(extension);
    return found == types.end() ? L"application/octet-stream" : found->second;
}

std::wstring formatStorageSize(int64_t bytes) {
    static wchar_t const* const UNITS[] = {L"B", L"KB", L"MB", L"GB", L"TB", L"PB"};

    double value = static_cast<double>(std::max<int64_t>(bytes, 0));
    size_t unit = 0;
    while (value >= 1024 && unit + 1 < std::size(UNITS)) {
        value /= 1024;
        unit++;
    }

    wchar_t text[64]{};
    swprintf_s(text, L"%.2f", value);
    std::wstring number(text);
    while (number.back() == L'0') number.pop_back();
    if (number.back() == L'.') number.pop_back();

    wchar_t separator[8]{};
    if (GetLocaleInfoEx(language().c_str(), LOCALE_SDECIMAL, separator, 8) > 0) {
        auto dot = number.find(L'.');
        if (dot != std::wstring::npos) number.replace(dot, 1, separator);
    }

    return number + L" " + UNITS[unit];
}

void openWebUrl(std::wstring const& url) {
    auto lower = url.substr(0, 8);
    for (auto& character : lower) character = static_cast<wchar_t>(towlower(character));
    if (lower.rfind(L"http://", 0) != 0 && lower.rfind(L"https://", 0) != 0) return;

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

}

#include "app.h"

#include <shared/hash.h>
#include <shared/log.h>

#include <version.h>

#include <array>
#include <chrono>
#include <cstdio>
#include <deque>
#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>

namespace fileship::host {

namespace {

std::mutex queueMutex;
std::deque<std::function<void()>> mainQueue;
std::thread::id mainThread = std::this_thread::get_id();

}

LaunchOptions& launchOptions() {
    static LaunchOptions options;
    return options;
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
    std::swprintf(text, std::size(text), L"-%08x", hash);
    return text;
}

std::wstring pipeName() {
    if (!launchOptions().pipe.empty()) return launchOptions().pipe;
    return defaultPipeName(profileSuffix());
}

std::filesystem::path userDataPath() {
    if (!launchOptions().userData.empty()) return std::filesystem::absolute(launchOptions().userData);
    return defaultUserDataPath();
}

std::filesystem::path hostExecutable() {
    static auto const path = executablePath();
    return path;
}

std::filesystem::path resourcesPath() {
    return hostExecutable().parent_path();
}

std::wstring appVersion() {
    return FILESHIP_VERSION;
}

bool isPackaged() {
    static bool const packaged = hasPackagedLayout();
    return packaged && !launchOptions().dev;
}

std::wstring desktopUserAgent() {
    return L"FileshipDesktop/" + appVersion() + L" (" + platformName() + L")";
}

void postToMainThread(std::function<void()> work) {
    {
        std::lock_guard lock(queueMutex);
        mainQueue.push_back(std::move(work));
    }

    wakeMainThread();
}

bool isMainThread() {
    return std::this_thread::get_id() == mainThread;
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
        } catch (...) {
            logError(L"Main thread task failed: " + describeException(std::current_exception()));
        }
    }
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
    randomBytes(random.data(), random.size());
    return toWide(toHex(random));
}

std::wstring randomUuid() {
    std::vector<uint8_t> bytes(16);
    randomBytes(bytes.data(), bytes.size());
    bytes[6] = static_cast<uint8_t>((bytes[6] & 0x0f) | 0x40);
    bytes[8] = static_cast<uint8_t>((bytes[8] & 0x3f) | 0x80);

    auto hex = toWide(toHex(bytes));
    return hex.substr(0, 8) + L"-" + hex.substr(8, 4) + L"-" + hex.substr(12, 4) + L"-" + hex.substr(16, 4) + L"-" +
           hex.substr(20);
}

std::wstring base64(void const* data, size_t size) {
    static constexpr wchar_t ALPHABET[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    auto bytes = static_cast<uint8_t const*>(data);
    std::wstring result;
    result.reserve((size + 2) / 3 * 4);

    for (size_t index = 0; index < size; index += 3) {
        uint32_t chunk = static_cast<uint32_t>(bytes[index]) << 16;
        if (index + 1 < size) chunk |= static_cast<uint32_t>(bytes[index + 1]) << 8;
        if (index + 2 < size) chunk |= bytes[index + 2];

        result += ALPHABET[(chunk >> 18) & 0x3F];
        result += ALPHABET[(chunk >> 12) & 0x3F];
        result += index + 1 < size ? ALPHABET[(chunk >> 6) & 0x3F] : L'=';
        result += index + 2 < size ? ALPHABET[chunk & 0x3F] : L'=';
    }

    return result;
}

std::vector<uint8_t> fromBase64(std::wstring const& text) {
    std::vector<uint8_t> result;
    uint32_t buffer = 0;
    int bits = 0;

    for (auto character : text) {
        int value = character >= L'A' && character <= L'Z'   ? character - L'A'
                    : character >= L'a' && character <= L'z' ? character - L'a' + 26
                    : character >= L'0' && character <= L'9' ? character - L'0' + 52
                    : character == L'+' || character == L'-' ? 62
                    : character == L'/' || character == L'_' ? 63
                                                             : -1;
        if (character == L'=' || character == L'\r' || character == L'\n' || character == L' ') continue;
        if (value < 0) return {};

        buffer = (buffer << 6) | static_cast<uint32_t>(value);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            result.push_back(static_cast<uint8_t>((buffer >> bits) & 0xFF));
        }
    }

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

    return replaceFile(temporary, path);
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
    std::swprintf(text, std::size(text), L"%.2f", value);
    std::wstring number(text);
    while (number.back() == L'0') number.pop_back();
    if (number.back() == L'.') number.pop_back();

    static std::wstring const COMMA_LANGUAGES[] = {L"tr", L"de", L"es", L"fr", L"it", L"pt", L"ru", L"az"};
    if (std::ranges::find(COMMA_LANGUAGES, language()) != std::end(COMMA_LANGUAGES)) {
        auto dot = number.find(L'.');
        if (dot != std::wstring::npos) number.replace(dot, 1, L",");
    }

    return number + L" " + UNITS[unit];
}

}

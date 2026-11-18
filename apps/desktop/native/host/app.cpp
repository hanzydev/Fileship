#include "app.h"

#include "platform.h"

#include <shared/log.h>
#include <shared/platform.h>
#include <shared/random.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cwctype>
#include <deque>
#include <fstream>
#include <mutex>
#include <thread>

namespace fileship::host {

namespace {

std::mutex queueMutex;
std::deque<std::function<void()>> mainQueue;
std::thread::id mainThread = std::this_thread::get_id();

std::wstring hexOf(uint8_t const* bytes, size_t size, std::initializer_list<size_t> dashes = {}) {
    static wchar_t const DIGITS[] = L"0123456789abcdef";
    std::wstring result;

    for (size_t index = 0; index < size; index++) {
        if (std::find(dashes.begin(), dashes.end(), index) != dashes.end()) result += L'-';

        result += DIGITS[bytes[index] >> 4];
        result += DIGITS[bytes[index] & 15];
    }

    return result;
}

}

LaunchOptions& launchOptions() {
    static LaunchOptions options;
    return options;
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
        else if (argument == L"--pipe" && hasValue) options.pipe = arguments[++index];
        else rest.push_back(argument);
    }

    if (options.hidden) rest.push_back(L"--hidden");
    options.arguments = rest;
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

    uint8_t bytes[] = {static_cast<uint8_t>(hash >> 24), static_cast<uint8_t>(hash >> 16),
                       static_cast<uint8_t>(hash >> 8), static_cast<uint8_t>(hash)};
    return L"-" + hexOf(bytes, sizeof(bytes));
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

bool isPackaged() {
    static bool const packaged = [] {
        auto resources = resourcesPath().parent_path();
        return resources.filename() == L"resources" &&
               std::filesystem::exists(resources.parent_path() / electronExecutable());
    }();

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
        } catch (...) {
            logError(L"Main thread task failed: " + exceptionMessage());
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
    return hexOf(random.data(), random.size());
}

std::wstring randomUuid() {
    std::array<uint8_t, 16> bytes{};
    randomBytes(bytes.data(), bytes.size());
    bytes[6] = static_cast<uint8_t>((bytes[6] & 0x0f) | 0x40);
    bytes[8] = static_cast<uint8_t>((bytes[8] & 0x3f) | 0x80);

    return hexOf(bytes.data(), bytes.size(), {4, 6, 8, 10});
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
    static std::wstring const DOT_LANGUAGES[] = {L"en", L"ja", L"ko", L"zh"};

    double value = static_cast<double>(std::max<int64_t>(bytes, 0));
    size_t unit = 0;
    while (value >= 1024 && unit + 1 < std::size(UNITS)) {
        value /= 1024;
        unit++;
    }

    char text[64]{};
    auto end = std::to_chars(text, text + sizeof(text), value, std::chars_format::fixed, 2).ptr;
    std::wstring number(text, end);
    while (number.back() == L'0') number.pop_back();
    if (number.back() == L'.') number.pop_back();

    bool usesDot = std::find(std::begin(DOT_LANGUAGES), std::end(DOT_LANGUAGES), language()) != std::end(DOT_LANGUAGES);
    auto dot = number.find(L'.');
    if (!usesDot && dot != std::wstring::npos) number[dot] = L',';

    return number + L" " + UNITS[unit];
}

void openWebUrl(std::wstring const& url) {
    auto lower = url.substr(0, 8);
    for (auto& character : lower) character = static_cast<wchar_t>(towlower(character));
    if (lower.rfind(L"http://", 0) != 0 && lower.rfind(L"https://", 0) != 0) return;

    openExternal(url);
}

}

#include "support.h"

#include <shared/i18n.h>
#include <shared/log.h>

#include <bcrypt.h>
#include <icu.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>

#pragma comment(lib, "icu.lib")

namespace fileship::host::harbor {

SerialQueue::SerialQueue(std::wstring name) : name(std::move(name)) {
    std::promise<void> ready;
    ready.set_value();
    last = ready.get_future().share();
    worker = std::thread([this] { run(); });
}

SerialQueue::~SerialQueue() {
    post({});
    worker.join();
}

std::shared_future<void> SerialQueue::post(std::function<void()> task) {
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future().share();

    {
        std::lock_guard lock(mutex);
        tasks.emplace_back(std::move(task), promise);
        last = future;
    }

    wake.notify_one();
    return future;
}

void SerialQueue::drain() {
    std::shared_future<void> current;

    {
        std::lock_guard lock(mutex);
        current = last;
    }

    current.wait();
}

void SerialQueue::run() {
    while (true) {
        std::function<void()> task;
        std::shared_ptr<std::promise<void>> promise;

        {
            std::unique_lock lock(mutex);
            wake.wait(lock, [this] { return !tasks.empty(); });
            task = std::move(tasks.front().first);
            promise = std::move(tasks.front().second);
            tasks.pop_front();
        }

        if (!task) {
            promise->set_value();
            return;
        }

        try {
            task();
            promise->set_value();
        } catch (std::exception const& error) {
            logError(name + L" task failed: " + errorMessage(error));
            promise->set_exception(std::current_exception());
        } catch (winrt::hresult_error const& error) {
            logError(name + L" task failed: " + std::wstring(error.message()));
            promise->set_exception(std::current_exception());
        }
    }
}

Timers::Timers() : worker([this] { run(); }) {}

Timers::~Timers() {
    {
        std::lock_guard lock(mutex);
        stopping = true;
    }

    wake.notify_all();
    worker.join();
}

uint64_t Timers::set(int delayMs, std::function<void()> callback) {
    uint64_t id = 0;

    {
        std::lock_guard lock(mutex);
        id = nextId++;
        pending.emplace(Clock::now() + std::chrono::milliseconds(delayMs),
                        std::make_pair(id, std::move(callback)));
    }

    wake.notify_all();
    return id;
}

void Timers::clear(uint64_t id) {
    if (!id) return;

    std::lock_guard lock(mutex);
    for (auto item = pending.begin(); item != pending.end(); ++item) {
        if (item->second.first == id) {
            pending.erase(item);
            return;
        }
    }
}

void Timers::run() {
    std::unique_lock lock(mutex);

    while (!stopping) {
        if (pending.empty()) {
            wake.wait(lock);
            continue;
        }

        auto first = pending.begin();
        if (first->first > Clock::now()) {
            wake.wait_until(lock, first->first);
            continue;
        }

        auto callback = std::move(first->second.second);
        pending.erase(first);
        lock.unlock();

        try {
            callback();
        } catch (std::exception const& error) {
            logError(L"Harbor timer failed: " + errorMessage(error));
        } catch (winrt::hresult_error const& error) {
            logError(L"Harbor timer failed: " + std::wstring(error.message()));
        }

        lock.lock();
    }
}

Timers& timers() {
    static auto* instance = new Timers();
    return *instance;
}

int64_t nowMs() {
    FILETIME time{};
    GetSystemTimePreciseAsFileTime(&time);
    ULARGE_INTEGER value{};
    value.LowPart = time.dwLowDateTime;
    value.HighPart = time.dwHighDateTime;
    return static_cast<int64_t>(value.QuadPart / 10000 - 11644473600000LL);
}

int64_t parseIsoDate(std::wstring const& text) {
    int year = 0, month = 0, day = 0, hour = 0, minute = 0;
    double second = 0;
    wchar_t rest[16]{};

    int read = swscanf_s(text.c_str(), L"%d-%d-%dT%d:%d:%lf%15s", &year, &month, &day, &hour, &minute,
                         &second, rest, static_cast<unsigned>(std::size(rest)));
    if (read < 6) return 0;

    std::tm parts{};
    parts.tm_year = year - 1900;
    parts.tm_mon = month - 1;
    parts.tm_mday = day;
    parts.tm_hour = hour;
    parts.tm_min = minute;
    parts.tm_sec = 0;

    int64_t result = _mkgmtime64(&parts) * 1000 + static_cast<int64_t>(std::llround(second * 1000));

    std::wstring zone(rest);
    if (zone.size() >= 6 && (zone[0] == L'+' || zone[0] == L'-')) {
        int offset = std::stoi(zone.substr(1, 2)) * 60 + std::stoi(zone.substr(4, 2));
        result -= (zone[0] == L'+' ? 1 : -1) * static_cast<int64_t>(offset) * 60000;
    }

    return result;
}

std::wstring randomHex(size_t bytes) {
    std::vector<unsigned char> random(bytes);
    BCryptGenRandom(nullptr, random.data(), static_cast<ULONG>(random.size()),
                    BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    std::wstring result;
    for (auto byte : random) {
        wchar_t hex[3]{};
        swprintf_s(hex, L"%02x", byte);
        result += hex;
    }

    return result;
}

namespace {

std::string hashHex(LPCWSTR algorithm, std::string const& data) {
    BCRYPT_ALG_HANDLE provider = nullptr;
    BCryptOpenAlgorithmProvider(&provider, algorithm, nullptr, 0);

    DWORD length = 0;
    DWORD size = 0;
    BCryptGetProperty(provider, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&length), sizeof(length),
                      &size, 0);

    std::vector<unsigned char> digest(length);
    BCryptHash(provider, nullptr, 0, reinterpret_cast<PUCHAR>(const_cast<char*>(data.data())),
               static_cast<ULONG>(data.size()), digest.data(), length);
    BCryptCloseAlgorithmProvider(provider, 0);

    std::string result;
    for (auto byte : digest) {
        char hex[3]{};
        snprintf(hex, sizeof(hex), "%02x", byte);
        result += hex;
    }

    return result;
}

std::string localeName() {
    return toUtf8(language());
}

}

std::string sha1Hex(std::string const& data) {
    return hashHex(BCRYPT_SHA1_ALGORITHM, data);
}

std::string sha256Hex(std::string const& data) {
    return hashHex(BCRYPT_SHA256_ALGORITHM, data);
}

std::wstring lower(std::wstring const& text) {
    if (text.empty()) return text;

    int size = LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, text.c_str(),
                             static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr, 0);
    std::wstring result(size, L'\0');
    LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, text.c_str(), static_cast<int>(text.size()),
                  result.data(), size, nullptr, nullptr, 0);
    return result;
}

bool equalsIgnoreCase(std::wstring const& a, std::wstring const& b) {
    return a.size() == b.size() && lower(a) == lower(b);
}

bool startsWith(std::wstring const& text, std::wstring const& prefix) {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

std::wstring dirnameOf(std::wstring const& path) {
    auto slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return L".";
    if (slash == 0) return path.substr(0, 1);
    return path.substr(0, slash);
}

std::wstring basenameOf(std::wstring const& path) {
    auto slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? path : path.substr(slash + 1);
}

std::wstring extnameOf(std::wstring const& path) {
    auto name = basenameOf(path);
    auto dot = name.find_last_of(L'.');
    if (dot == std::wstring::npos || dot == 0) return {};
    return name.substr(dot);
}

std::wstring formatNumber(double value, int maxFractionDigits) {
    UErrorCode status = U_ZERO_ERROR;
    auto locale = localeName();
    UNumberFormat* format = unum_open(UNUM_DECIMAL, nullptr, 0, locale.c_str(), nullptr, &status);

    if (U_FAILURE(status)) {
        wchar_t buffer[64]{};
        swprintf_s(buffer, L"%.*f", maxFractionDigits, value);
        return buffer;
    }

    unum_setAttribute(format, UNUM_MIN_FRACTION_DIGITS, 0);
    unum_setAttribute(format, UNUM_MAX_FRACTION_DIGITS, maxFractionDigits);

    std::array<UChar, 128> buffer{};
    int32_t length = unum_formatDouble(format, value, buffer.data(), static_cast<int32_t>(buffer.size()),
                                       nullptr, &status);
    unum_close(format);

    return std::wstring(reinterpret_cast<wchar_t const*>(buffer.data()), std::max(length, 0));
}

std::wstring formatSize(int64_t bytes) {
    static constexpr std::array<wchar_t const*, 9> SYMBOLS = {L"B",  L"KB", L"MB", L"GB", L"TB",
                                                              L"PB", L"EB", L"ZB", L"YB"};

    int exponent = bytes > 0 ? static_cast<int>(std::floor(std::log(static_cast<double>(bytes)) /
                                                            std::log(1024.0)))
                             : 0;
    exponent = std::clamp(exponent, 0, 8);

    double value = static_cast<double>(bytes) / std::pow(1024.0, exponent);
    value = std::round(value * 100) / 100;

    if (value == 1024 && exponent < 8) {
        value = 1;
        exponent++;
    }

    return formatNumber(value, 3) + L" " + SYMBOLS[exponent];
}

std::wstring formatDateTime(int64_t time) {
    UErrorCode status = U_ZERO_ERROR;
    auto locale = localeName();
    UDateFormat* format = udat_open(UDAT_SHORT, UDAT_MEDIUM, locale.c_str(), nullptr, -1, nullptr, 0,
                                    &status);
    if (U_FAILURE(status)) return std::to_wstring(time);

    std::array<UChar, 256> buffer{};
    int32_t length = udat_format(format, static_cast<UDate>(time), buffer.data(),
                                 static_cast<int32_t>(buffer.size()), nullptr, &status);
    udat_close(format);

    return std::wstring(reinterpret_cast<wchar_t const*>(buffer.data()), std::max(length, 0));
}

std::wstring relativeTime(int64_t value, int64_t now, std::wstring const& justNow) {
    static constexpr std::array<std::pair<URelativeDateTimeUnit, int>, 6> UNITS = {{
        {UDAT_REL_UNIT_YEAR, 31536000},
        {UDAT_REL_UNIT_MONTH, 2592000},
        {UDAT_REL_UNIT_WEEK, 604800},
        {UDAT_REL_UNIT_DAY, 86400},
        {UDAT_REL_UNIT_HOUR, 3600},
        {UDAT_REL_UNIT_MINUTE, 60},
    }};

    double seconds = static_cast<double>(value - now) / 1000;
    auto unit = std::ranges::find_if(UNITS, [&](auto const& item) { return std::abs(seconds) >= item.second; });
    if (unit == UNITS.end()) return justNow;

    UErrorCode status = U_ZERO_ERROR;
    auto locale = localeName();
    URelativeDateTimeFormatter* format =
        ureldatefmt_open(locale.c_str(), nullptr, UDAT_STYLE_LONG, UDISPCTX_CAPITALIZATION_NONE, &status);
    if (U_FAILURE(status)) return justNow;

    std::array<UChar, 128> buffer{};
    double offset = std::round(seconds / unit->second);
    int32_t length = ureldatefmt_format(format, offset == 0 ? 0 : offset, unit->first, buffer.data(),
                                        static_cast<int32_t>(buffer.size()), &status);
    ureldatefmt_close(format);

    if (U_FAILURE(status)) return justNow;
    return std::wstring(reinterpret_cast<wchar_t const*>(buffer.data()), std::max(length, 0));
}

std::wstring errorMessage(std::exception const& error) {
    return toWide(error.what());
}

std::wstring mimeTypeOf(std::wstring const& path) {
    static std::map<std::wstring, std::wstring> const TYPES = {
        {L".png", L"image/png"},        {L".jpg", L"image/jpeg"},
        {L".jpeg", L"image/jpeg"},      {L".gif", L"image/gif"},
        {L".webp", L"image/webp"},      {L".bmp", L"image/bmp"},
        {L".svg", L"image/svg+xml"},    {L".mp4", L"video/mp4"},
        {L".webm", L"video/webm"},      {L".mkv", L"video/x-matroska"},
        {L".mov", L"video/quicktime"},  {L".mp3", L"audio/mpeg"},
        {L".wav", L"audio/wav"},        {L".ogg", L"audio/ogg"},
        {L".pdf", L"application/pdf"},  {L".zip", L"application/zip"},
        {L".txt", L"text/plain"},       {L".md", L"text/markdown"},
        {L".json", L"application/json"}, {L".html", L"text/html"},
        {L".css", L"text/css"},         {L".js", L"text/javascript"},
        {L".ts", L"text/plain"},
    };

    auto found = TYPES.find(lower(extnameOf(path)));
    return found == TYPES.end() ? L"application/octet-stream" : found->second;
}

std::optional<std::wstring> stringField(JsonObject const& object, std::wstring const& key) {
    if (!object.HasKey(key)) return std::nullopt;

    auto value = object.GetNamedValue(key);
    if (value.ValueType() != JsonValueType::String) return std::nullopt;
    return std::wstring(value.GetString());
}

std::optional<double> numberField(JsonObject const& object, std::wstring const& key) {
    if (!object.HasKey(key)) return std::nullopt;

    auto value = object.GetNamedValue(key);
    if (value.ValueType() == JsonValueType::Number) return value.GetNumber();
    if (value.ValueType() == JsonValueType::String) {
        try {
            return std::stod(std::wstring(value.GetString()));
        } catch (std::exception const&) {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

std::optional<bool> boolField(JsonObject const& object, std::wstring const& key) {
    if (!object.HasKey(key)) return std::nullopt;

    auto value = object.GetNamedValue(key);
    if (value.ValueType() != JsonValueType::Boolean) return std::nullopt;
    return value.GetBoolean();
}

bool isNullField(JsonObject const& object, std::wstring const& key) {
    return object.HasKey(key) && object.GetNamedValue(key).ValueType() == JsonValueType::Null;
}

}

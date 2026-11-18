#include <host/harbor/support.h>

#include <shared/i18n.h>
#include <shared/utf8.h>

#include <windows.h>

#include <bcrypt.h>
#include <icu.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>

#pragma comment(lib, "icu.lib")

namespace fileship::host::harbor {

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

}

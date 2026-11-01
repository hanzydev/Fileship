#include <host/harbor/support.h>

#include <shared/i18n.h>
#include <shared/utf8.h>

#include <dlfcn.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <cwctype>
#include <string>

namespace fileship::host::harbor {

namespace {

constexpr int NUMBER_DECIMAL = 1;
constexpr int MAX_FRACTION_DIGITS = 6;
constexpr int MIN_FRACTION_DIGITS = 7;
constexpr int DATE_MEDIUM = 2;
constexpr int DATE_SHORT = 3;
constexpr int RELATIVE_STYLE_LONG = 0;
constexpr int CAPITALIZATION_NONE = 0x100;

enum RelativeUnit { YEAR = 0, MONTH = 2, WEEK = 3, DAY = 4, HOUR = 5, MINUTE = 6 };

using NumberOpen = void* (*)(int, char16_t const*, int32_t, char const*, void*, int*);
using NumberSetAttribute = void (*)(void*, int, int32_t);
using NumberFormat = int32_t (*)(void const*, double, char16_t*, int32_t, void*, int*);
using DateOpen = void* (*)(int, int, char const*, char16_t const*, int32_t, char16_t const*, int32_t, int*);
using DateFormat = int32_t (*)(void const*, double, char16_t*, int32_t, void*, int*);
using RelativeOpen = void* (*)(char const*, void*, int, int, int*);
using RelativeFormat = int32_t (*)(void const*, double, int, char16_t*, int32_t, int*);
using Close = void (*)(void*);

struct Icu {
    NumberOpen numberOpen = nullptr;
    NumberSetAttribute numberSetAttribute = nullptr;
    NumberFormat numberFormat = nullptr;
    Close numberClose = nullptr;
    DateOpen dateOpen = nullptr;
    DateFormat dateFormat = nullptr;
    Close dateClose = nullptr;
    RelativeOpen relativeOpen = nullptr;
    RelativeFormat relativeFormat = nullptr;
    Close relativeClose = nullptr;

    bool isLoaded() const {
        return numberOpen && numberSetAttribute && numberFormat && numberClose && dateOpen && dateFormat &&
               dateClose && relativeOpen && relativeFormat && relativeClose;
    }
};

Icu const& icu() {
    static Icu const loaded = [] {
        Icu result;

        for (int version = 99; version >= 60; version--) {
            auto library = dlopen(("libicui18n.so." + std::to_string(version)).c_str(), RTLD_LAZY | RTLD_LOCAL);
            if (!library) continue;

            auto symbol = [&](char const* name) {
                auto suffixed = std::string(name) + "_" + std::to_string(version);
                auto found = dlsym(library, suffixed.c_str());
                return found ? found : dlsym(library, name);
            };

            result.numberOpen = reinterpret_cast<NumberOpen>(symbol("unum_open"));
            result.numberSetAttribute = reinterpret_cast<NumberSetAttribute>(symbol("unum_setAttribute"));
            result.numberFormat = reinterpret_cast<NumberFormat>(symbol("unum_formatDouble"));
            result.numberClose = reinterpret_cast<Close>(symbol("unum_close"));
            result.dateOpen = reinterpret_cast<DateOpen>(symbol("udat_open"));
            result.dateFormat = reinterpret_cast<DateFormat>(symbol("udat_format"));
            result.dateClose = reinterpret_cast<Close>(symbol("udat_close"));
            result.relativeOpen = reinterpret_cast<RelativeOpen>(symbol("ureldatefmt_open"));
            result.relativeFormat = reinterpret_cast<RelativeFormat>(symbol("ureldatefmt_format"));
            result.relativeClose = reinterpret_cast<Close>(symbol("ureldatefmt_close"));
            if (result.isLoaded()) return result;

            dlclose(library);
            result = Icu();
        }

        return result;
    }();
    return loaded;
}

std::string localeName() {
    return toUtf8(language());
}

std::wstring fromUtf16(char16_t const* text, int32_t length) {
    std::wstring result;
    for (int32_t index = 0; index < length; index++) {
        char32_t unit = text[index];
        if (unit >= 0xd800 && unit < 0xdc00 && index + 1 < length) {
            char32_t low = text[++index];
            unit = 0x10000 + ((unit - 0xd800) << 10) + (low - 0xdc00);
        }
        result.push_back(static_cast<wchar_t>(unit));
    }
    return result;
}

std::wstring plainNumber(double value, int maxFractionDigits) {
    char buffer[64]{};
    std::snprintf(buffer, sizeof(buffer), "%.*f", maxFractionDigits, value);
    std::string text = buffer;
    if (text.find('.') != std::string::npos) {
        while (text.back() == '0') text.pop_back();
        if (text.back() == '.') text.pop_back();
    }
    return toWide(text);
}

}

std::wstring lower(std::wstring const& text) {
    std::wstring result(text);
    for (auto& character : result) character = static_cast<wchar_t>(std::towlower(static_cast<wint_t>(character)));
    return result;
}

std::wstring formatNumber(double value, int maxFractionDigits) {
    auto const& library = icu();
    if (!library.isLoaded()) return plainNumber(value, maxFractionDigits);

    int status = 0;
    auto format = library.numberOpen(NUMBER_DECIMAL, nullptr, 0, localeName().c_str(), nullptr, &status);
    if (status > 0) return plainNumber(value, maxFractionDigits);

    library.numberSetAttribute(format, MIN_FRACTION_DIGITS, 0);
    library.numberSetAttribute(format, MAX_FRACTION_DIGITS, maxFractionDigits);

    std::array<char16_t, 128> buffer{};
    int32_t length = library.numberFormat(format, value, buffer.data(), static_cast<int32_t>(buffer.size()), nullptr,
                                          &status);
    library.numberClose(format);

    if (status > 0) return plainNumber(value, maxFractionDigits);
    return fromUtf16(buffer.data(), std::max(length, 0));
}

std::wstring formatDateTime(int64_t time) {
    auto const& library = icu();

    if (library.isLoaded()) {
        int status = 0;
        auto format =
            library.dateOpen(DATE_SHORT, DATE_MEDIUM, localeName().c_str(), nullptr, -1, nullptr, 0, &status);

        if (status <= 0) {
            std::array<char16_t, 256> buffer{};
            int32_t length = library.dateFormat(format, static_cast<double>(time), buffer.data(),
                                                static_cast<int32_t>(buffer.size()), nullptr, &status);
            library.dateClose(format);
            if (status <= 0) return fromUtf16(buffer.data(), std::max(length, 0));
        }
    }

    auto seconds = static_cast<std::time_t>(time / 1000);
    std::tm local{};
    localtime_r(&seconds, &local);
    char buffer[64]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &local);
    return toWide(buffer);
}

std::wstring relativeTime(int64_t value, int64_t now, std::wstring const& justNow) {
    static constexpr std::array<std::pair<RelativeUnit, int>, 6> UNITS = {{
        {YEAR, 31536000},
        {MONTH, 2592000},
        {WEEK, 604800},
        {DAY, 86400},
        {HOUR, 3600},
        {MINUTE, 60},
    }};

    double seconds = static_cast<double>(value - now) / 1000;
    auto unit = std::ranges::find_if(UNITS, [&](auto const& item) { return std::abs(seconds) >= item.second; });
    if (unit == UNITS.end()) return justNow;

    auto const& library = icu();
    if (!library.isLoaded()) return formatDateTime(value);

    int status = 0;
    auto format = library.relativeOpen(localeName().c_str(), nullptr, RELATIVE_STYLE_LONG, CAPITALIZATION_NONE, &status);
    if (status > 0) return formatDateTime(value);

    std::array<char16_t, 128> buffer{};
    double offset = std::round(seconds / unit->second);
    int32_t length = library.relativeFormat(format, offset == 0 ? 0 : offset, unit->first, buffer.data(),
                                            static_cast<int32_t>(buffer.size()), &status);
    library.relativeClose(format);

    if (status > 0) return formatDateTime(value);
    return fromUtf16(buffer.data(), std::max(length, 0));
}

}

#include <host/harbor/support.h>

#include <shared/i18n.h>
#include <shared/random.h>
#include <shared/utf8.h>

#include <glib.h>

#include <array>
#include <cstdio>
#include <vector>

namespace fileship::host::harbor {

namespace {

struct NumberStyle {
    wchar_t const* language;
    wchar_t const* decimal;
    wchar_t const* group;
};

constexpr std::array<NumberStyle, 8> COMMA_STYLES = {{
    {L"tr", L",", L"."},
    {L"de", L",", L"."},
    {L"es", L",", L"."},
    {L"it", L",", L"."},
    {L"pt", L",", L"."},
    {L"az", L",", L"."},
    {L"fr", L",", L"\x202f"},
    {L"ru", L",", L"\xa0"},
}};

NumberStyle numberStyle() {
    auto current = language();

    for (auto const& style : COMMA_STYLES) {
        if (current == style.language) return style;
    }

    return {L"en", L".", L","};
}

std::string checksum(GChecksumType type, std::string const& data) {
    gchar* hex = g_compute_checksum_for_data(type, reinterpret_cast<guchar const*>(data.data()), data.size());
    std::string result(hex);
    g_free(hex);
    return result;
}

}

int64_t nowMs() {
    return g_get_real_time() / 1000;
}

int64_t parseIsoDate(std::wstring const& text) {
    GTimeZone* utc = g_time_zone_new_utc();
    GDateTime* time = g_date_time_new_from_iso8601(toUtf8(text).c_str(), utc);
    g_time_zone_unref(utc);
    if (!time) return 0;

    int64_t result = g_date_time_to_unix(time) * 1000 + g_date_time_get_microsecond(time) / 1000;
    g_date_time_unref(time);
    return result;
}

std::wstring randomHex(size_t bytes) {
    std::vector<unsigned char> random(bytes);
    randomBytes(random.data(), random.size());

    std::wstring result;
    for (auto byte : random) {
        wchar_t hex[3]{};
        swprintf(hex, 3, L"%02x", byte);
        result += hex;
    }

    return result;
}

std::string sha1Hex(std::string const& data) {
    return checksum(G_CHECKSUM_SHA1, data);
}

std::string sha256Hex(std::string const& data) {
    return checksum(G_CHECKSUM_SHA256, data);
}

std::wstring lower(std::wstring const& text) {
    auto utf8 = toUtf8(text);
    gchar* lowered = g_utf8_strdown(utf8.data(), static_cast<gssize>(utf8.size()));
    auto result = toWide(lowered);
    g_free(lowered);
    return result;
}

std::wstring formatNumber(double value, int maxFractionDigits) {
    char buffer[64]{};
    snprintf(buffer, sizeof(buffer), "%.*f", maxFractionDigits, value);

    std::string digits(buffer);
    if (digits.find('.') != std::string::npos) {
        while (digits.back() == '0') digits.pop_back();
        if (digits.back() == '.') digits.pop_back();
    }

    auto style = numberStyle();
    auto dot = digits.find('.');
    auto whole = digits.substr(0, dot);
    std::wstring result;

    for (size_t index = 0; index < whole.size(); index++) {
        size_t left = whole.size() - index;
        if (index && left % 3 == 0 && whole[index - 1] != '-') result += style.group;
        result += static_cast<wchar_t>(whole[index]);
    }

    if (dot != std::string::npos) result += style.decimal + toWide(digits.substr(dot + 1));
    return result;
}

std::wstring formatDateTime(int64_t time) {
    GDateTime* local = g_date_time_new_from_unix_local(time / 1000);
    gchar* text = g_date_time_format(local, "%x %X");
    auto result = toWide(text);
    g_free(text);
    g_date_time_unref(local);
    return result;
}

std::wstring relativeTime(int64_t value, int64_t now, std::wstring const& justNow) {
    return now - value < 60000 ? justNow : formatDateTime(value);
}

}

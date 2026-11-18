#include <shared/utf8.h>

#include <cstdint>

namespace fileship {

namespace {

struct Decoded {
    size_t length;
    char32_t point;
    bool isValid;
};

Decoded decode(std::string_view text, size_t start) {
    auto at = [&](size_t offset) {
        return start + offset < text.size() ? static_cast<uint8_t>(text[start + offset]) : uint8_t{0};
    };

    uint8_t lead = at(0);
    if (lead < 0x80) return {1, lead, true};

    size_t count = lead >= 0xf0 && lead <= 0xf4 ? 4 : lead >= 0xe0 && lead <= 0xef ? 3 : lead >= 0xc2 && lead <= 0xdf ? 2 : 0;
    if (!count) return {1, 0, false};

    uint8_t low = lead == 0xe0 ? 0xa0 : lead == 0xf0 ? 0x90 : 0x80;
    uint8_t high = lead == 0xed ? 0x9f : lead == 0xf4 ? 0x8f : 0xbf;
    char32_t point = lead & (0xff >> (count + 1));

    for (size_t offset = 1; offset < count; offset++) {
        uint8_t tail = at(offset);
        if (tail < low || tail > high) return {offset, 0, false};

        point = (point << 6) | (tail & 0x3f);
        low = 0x80;
        high = 0xbf;
    }

    return {count, point, true};
}

}

std::wstring toWide(std::string_view text) {
    std::wstring result;
    result.reserve(text.size());

    for (size_t index = 0; index < text.size();) {
        auto decoded = decode(text, index);
        result += decoded.isValid ? static_cast<wchar_t>(decoded.point) : L'\xfffd';
        index += decoded.length;
    }

    return result;
}

std::string toUtf8(std::wstring_view text) {
    std::string result;
    result.reserve(text.size());

    for (auto character : text) {
        auto point = static_cast<uint32_t>(character);
        if (point > 0x10ffff || (point >= 0xd800 && point < 0xe000)) point = 0xfffd;

        if (point < 0x80) {
            result += static_cast<char>(point);
        } else if (point < 0x800) {
            result += static_cast<char>(0xc0 | (point >> 6));
            result += static_cast<char>(0x80 | (point & 0x3f));
        } else if (point < 0x10000) {
            result += static_cast<char>(0xe0 | (point >> 12));
            result += static_cast<char>(0x80 | ((point >> 6) & 0x3f));
            result += static_cast<char>(0x80 | (point & 0x3f));
        } else {
            result += static_cast<char>(0xf0 | (point >> 18));
            result += static_cast<char>(0x80 | ((point >> 12) & 0x3f));
            result += static_cast<char>(0x80 | ((point >> 6) & 0x3f));
            result += static_cast<char>(0x80 | (point & 0x3f));
        }
    }

    return result;
}

}

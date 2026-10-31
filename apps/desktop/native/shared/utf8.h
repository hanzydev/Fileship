#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace fileship {

inline std::wstring toWide(std::string_view text) {
    std::wstring result;
    result.reserve(text.size());

    auto append = [&](uint32_t code) {
        if constexpr (sizeof(wchar_t) == 2) {
            if (code >= 0x10000) {
                code -= 0x10000;
                result += static_cast<wchar_t>(0xD800 + (code >> 10));
                result += static_cast<wchar_t>(0xDC00 + (code & 0x3FF));
                return;
            }
        }
        result += static_cast<wchar_t>(code);
    };

    size_t index = 0;
    while (index < text.size()) {
        auto lead = static_cast<unsigned char>(text[index]);
        if (lead < 0x80) {
            result += static_cast<wchar_t>(lead);
            index++;
            continue;
        }

        int length = lead >= 0xC2 && lead <= 0xDF ? 2 : lead >= 0xE0 && lead <= 0xEF ? 3 : lead >= 0xF0 && lead <= 0xF4 ? 4 : 0;
        uint32_t code = length == 4 ? lead & 0x07 : length == 3 ? lead & 0x0F : lead & 0x1F;
        int read = 1;
        while (length && read < length && index + read < text.size()) {
            auto next = static_cast<unsigned char>(text[index + read]);
            if ((next & 0xC0) != 0x80) break;
            code = (code << 6) | (next & 0x3F);
            read++;
        }

        bool valid = length && read == length && !(length == 3 && code < 0x800) &&
                     !(length == 4 && (code < 0x10000 || code > 0x10FFFF)) && !(code >= 0xD800 && code < 0xE000);
        append(valid ? code : 0xFFFD);
        index += valid ? length : read;
    }

    return result;
}

inline std::string toUtf8(std::wstring_view text) {
    std::string result;
    result.reserve(text.size());

    for (size_t index = 0; index < text.size(); index++) {
        auto code = static_cast<uint32_t>(text[index]);
        if constexpr (sizeof(wchar_t) == 2) {
            if (code >= 0xD800 && code < 0xDC00 && index + 1 < text.size()) {
                auto low = static_cast<uint32_t>(text[index + 1]);
                if (low >= 0xDC00 && low < 0xE000) {
                    code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                    index++;
                }
            }
        }
        if ((code >= 0xD800 && code < 0xE000) || code > 0x10FFFF) code = 0xFFFD;

        if (code < 0x80) {
            result += static_cast<char>(code);
        } else if (code < 0x800) {
            result += static_cast<char>(0xC0 | (code >> 6));
            result += static_cast<char>(0x80 | (code & 0x3F));
        } else if (code < 0x10000) {
            result += static_cast<char>(0xE0 | (code >> 12));
            result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (code & 0x3F));
        } else {
            result += static_cast<char>(0xF0 | (code >> 18));
            result += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
            result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (code & 0x3F));
        }
    }

    return result;
}

}

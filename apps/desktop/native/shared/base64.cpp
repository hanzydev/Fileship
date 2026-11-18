#include "base64.h"

namespace fileship {

namespace {

constexpr wchar_t ALPHABET[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int valueOf(wchar_t character) {
    if (character >= L'A' && character <= L'Z') return character - L'A';
    if (character >= L'a' && character <= L'z') return character - L'a' + 26;
    if (character >= L'0' && character <= L'9') return character - L'0' + 52;
    if (character == L'+') return 62;
    if (character == L'/') return 63;
    return -1;
}

}

std::wstring base64(void const* data, size_t size) {
    auto bytes = static_cast<uint8_t const*>(data);
    std::wstring result;
    result.reserve((size + 2) / 3 * 4);

    for (size_t index = 0; index < size; index += 3) {
        size_t count = size - index < 3 ? size - index : 3;
        uint32_t group = bytes[index] << 16;
        if (count > 1) group |= bytes[index + 1] << 8;
        if (count > 2) group |= bytes[index + 2];

        result += ALPHABET[group >> 18];
        result += ALPHABET[(group >> 12) & 63];
        result += count > 1 ? ALPHABET[(group >> 6) & 63] : L'=';
        result += count > 2 ? ALPHABET[group & 63] : L'=';
    }

    return result;
}

std::vector<uint8_t> fromBase64(std::wstring const& text) {
    std::vector<uint8_t> result;
    uint32_t group = 0;
    int bits = 0;

    for (auto character : text) {
        if (character == L'=') break;
        if (character == L'\r' || character == L'\n' || character == L' ' || character == L'\t') continue;

        int value = valueOf(character);
        if (value < 0) return {};

        group = (group << 6) | static_cast<uint32_t>(value);
        bits += 6;

        if (bits >= 8) {
            bits -= 8;
            result.push_back(static_cast<uint8_t>(group >> bits));
        }
    }

    return result;
}

}

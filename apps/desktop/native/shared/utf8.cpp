#include "utf8.h"

#include <cstdint>

namespace fileship {

namespace {

size_t unitsOf(wchar_t character) {
    return static_cast<uint32_t>(character) > 0xffff ? 2 : 1;
}

bool isHighSurrogate(wchar_t character) {
    return (static_cast<uint32_t>(character) & 0xfc00) == 0xd800;
}

}

size_t utf16Length(std::wstring_view text) {
    size_t units = 0;
    for (auto character : text) units += unitsOf(character);
    return units;
}

std::wstring utf16Prefix(std::wstring_view text, size_t units) {
    size_t size = 0;
    size_t taken = 0;

    while (size < text.size() && taken + unitsOf(text[size]) <= units) taken += unitsOf(text[size++]);
    if (size && size < text.size() && isHighSurrogate(text[size - 1])) size--;

    return std::wstring(text.substr(0, size));
}

}

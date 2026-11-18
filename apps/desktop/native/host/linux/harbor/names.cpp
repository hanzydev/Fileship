#include <host/harbor/paths.h>
#include <host/harbor/platform.h>
#include <shared/utf8.h>

#include <cwctype>

namespace fileship::host::harbor {

namespace {

constexpr size_t NAME_BYTES = 200;

bool isInvalid(wchar_t character) {
    return character == L'/' || character <= 0x1f || character == 0x7f;
}

}

wchar_t const SEPARATOR = L'/';

std::wstring pathKey(std::wstring const& path) {
    return path;
}

std::wstring safeName(std::wstring const& name, std::wstring const& fallback) {
    std::wstring value;
    for (auto character : name) {
        if (!isInvalid(character)) value += character;
    }

    while (!value.empty() && std::iswspace(value.front())) value.erase(value.begin());
    while (!value.empty() && std::iswspace(value.back())) value.pop_back();

    if (value.empty() || value == L"." || value == L"..") value = fallback;
    while (toUtf8(value).size() > NAME_BYTES) value.pop_back();

    return value;
}

}

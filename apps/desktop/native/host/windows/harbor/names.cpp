#include <host/harbor/paths.h>

#include <cwctype>

namespace fileship::host::harbor {

namespace {

bool isInvalid(wchar_t character) {
    return std::wstring_view(L"\\/:*?\"<>|").find(character) != std::wstring_view::npos ||
           character <= 0x1f || (character >= 0x7f && character <= 0x9f);
}

bool isSpace(wchar_t character) {
    return std::iswspace(character) || character == 0xfeff || character == 0xa0;
}

bool isReserved(std::wstring const& value) {
    auto stem = lower(value.substr(0, value.find(L'.')));

    if (stem == L"con" || stem == L"prn" || stem == L"aux" || stem == L"nul") return true;
    return stem.size() == 4 && (startsWith(stem, L"com") || startsWith(stem, L"lpt")) &&
           stem[3] >= L'0' && stem[3] <= L'9';
}

}

std::wstring safeName(std::wstring const& name, std::wstring const& fallback) {
    std::wstring value;
    for (auto character : name) {
        if (!isInvalid(character)) value += character;
    }

    while (!value.empty() && isSpace(value.front())) value.erase(value.begin());
    while (!value.empty() && isSpace(value.back())) value.pop_back();
    while (!value.empty() && (value.back() == L'.' || value.back() == L' ')) value.pop_back();

    if (value.empty()) value = fallback;
    if (isReserved(value)) value = L"_" + value;

    return value.substr(0, 200);
}

}

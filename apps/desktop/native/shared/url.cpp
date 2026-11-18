#include "url.h"

#include <cwctype>

namespace fileship {

namespace {

constexpr int MAX_PORT = 65535;

bool startsWith(std::wstring_view text, std::wstring_view prefix) {
    if (text.size() < prefix.size()) return false;

    for (size_t index = 0; index < prefix.size(); index++) {
        if (static_cast<wchar_t>(towlower(text[index])) != prefix[index]) return false;
    }

    return true;
}

}

std::optional<HttpUrl> parseHttpUrl(std::wstring_view text) {
    HttpUrl url;

    if (startsWith(text, L"https://")) {
        url.isSecure = true;
        text.remove_prefix(8);
    } else if (startsWith(text, L"http://")) {
        text.remove_prefix(7);
    } else {
        return std::nullopt;
    }

    auto authority = text.substr(0, text.find_first_of(L"/?#"));
    if (auto user = authority.rfind(L'@'); user != std::wstring_view::npos) authority.remove_prefix(user + 1);

    size_t hostEnd = authority.starts_with(L'[') ? authority.find(L']') : std::wstring_view::npos;
    size_t colon = authority.find(L':', hostEnd == std::wstring_view::npos ? 0 : hostEnd);

    url.host = authority.substr(0, colon);
    if (url.host.empty()) return std::nullopt;

    url.port = url.isSecure ? 443 : 80;
    if (colon == std::wstring_view::npos || colon + 1 == authority.size()) return url;

    url.port = 0;
    for (auto digit : authority.substr(colon + 1)) {
        if (digit < L'0' || digit > L'9') return std::nullopt;

        url.port = url.port * 10 + (digit - L'0');
        if (url.port > MAX_PORT) return std::nullopt;
    }

    return url;
}

}

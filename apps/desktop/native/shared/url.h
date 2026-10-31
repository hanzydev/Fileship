#pragma once

#include <cwctype>
#include <optional>
#include <string>
#include <string_view>

namespace fileship {

struct Url {
    bool secure = false;
    std::wstring host;
    int port = 0;
    std::wstring path;

    bool isDefaultPort() const {
        return port == (secure ? 443 : 80);
    }
};

inline std::optional<Url> parseUrl(std::wstring_view text) {
    auto startsWith = [&](std::wstring_view prefix) {
        if (text.size() < prefix.size()) return false;
        for (size_t index = 0; index < prefix.size(); index++)
            if (static_cast<wchar_t>(std::towlower(text[index])) != prefix[index]) return false;
        return true;
    };

    Url url;
    if (startsWith(L"https://")) {
        url.secure = true;
        text.remove_prefix(8);
    } else if (startsWith(L"http://")) {
        text.remove_prefix(7);
    } else {
        return std::nullopt;
    }

    auto authorityEnd = text.find_first_of(L"/?#");
    auto authority = text.substr(0, authorityEnd);
    auto rest = authorityEnd == std::wstring_view::npos ? std::wstring_view() : text.substr(authorityEnd);

    if (auto at = authority.rfind(L'@'); at != std::wstring_view::npos) authority.remove_prefix(at + 1);

    std::wstring_view port;
    if (!authority.empty() && authority.front() == L'[') {
        auto close = authority.find(L']');
        if (close == std::wstring_view::npos) return std::nullopt;
        url.host = authority.substr(0, close + 1);
        auto after = authority.substr(close + 1);
        if (!after.empty()) {
            if (after.front() != L':') return std::nullopt;
            port = after.substr(1);
        }
    } else {
        auto colon = authority.find(L':');
        url.host = authority.substr(0, colon);
        if (colon != std::wstring_view::npos) port = authority.substr(colon + 1);
    }

    if (url.host.empty()) return std::nullopt;
    for (auto character : url.host)
        if (std::iswspace(character) || std::iswcntrl(character) || character == L'\\') return std::nullopt;

    if (port.empty()) {
        url.port = url.secure ? 443 : 80;
    } else {
        if (port.size() > 5) return std::nullopt;
        for (auto character : port) {
            if (character < L'0' || character > L'9') return std::nullopt;
            url.port = url.port * 10 + (character - L'0');
        }
        if (url.port < 1 || url.port > 65535) return std::nullopt;
    }

    if (auto hash = rest.find(L'#'); hash != std::wstring_view::npos) rest = rest.substr(0, hash);
    url.path = rest.empty() || rest.front() == L'/' ? std::wstring(rest) : L"/" + std::wstring(rest);
    return url;
}

}

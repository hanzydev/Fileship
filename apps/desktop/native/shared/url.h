#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace fileship {

struct HttpUrl {
    bool isSecure = false;
    std::wstring host;
    int port = 0;
};

std::optional<HttpUrl> parseHttpUrl(std::wstring_view text);

}

#pragma once

#include <windows.h>
#include <winhttp.h>

#include <string>

namespace fileship {

struct Internet {
    HINTERNET value = nullptr;

    explicit Internet(HINTERNET handle) : value(handle) {}

    ~Internet() {
        if (value) WinHttpCloseHandle(value);
    }

    Internet(Internet const&) = delete;
    Internet& operator=(Internet const&) = delete;
};

struct CrackedUrl {
    std::wstring host;
    std::wstring path;
    INTERNET_PORT port = 0;
    bool isSecure = false;
};

bool crackUrl(std::wstring const& url, CrackedUrl& result);

}

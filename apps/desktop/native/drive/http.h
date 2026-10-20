#pragma once

#include "common.h"

#include <winhttp.h>

namespace drive {

struct Internet {
    HINTERNET value = nullptr;

    explicit Internet(HINTERNET handle) : value(handle) {}

    ~Internet() {
        if (value) WinHttpCloseHandle(value);
    }

    Internet(Internet const&) = delete;
    Internet& operator=(Internet const&) = delete;
};

struct HttpStream {
    Internet connection;
    Internet request;
    DWORD status = 0;

    HttpStream(std::wstring const& path, std::wstring const& headers = {});
    DWORD read(char* buffer, DWORD size);
};

struct HttpResponse {
    DWORD status = 0;
    std::vector<char> body;
};

HttpResponse httpGet(std::wstring const& path);

}

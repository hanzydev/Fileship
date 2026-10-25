#pragma once

#include <windows.h>
#include <winhttp.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace fileship {

struct HttpEndpoint {
    std::wstring baseUrl;
    std::wstring token;
    std::wstring userAgent;
    std::wstring language;
};

struct Internet {
    HINTERNET value = nullptr;

    explicit Internet(HINTERNET handle) : value(handle) {}

    ~Internet() {
        if (value) WinHttpCloseHandle(value);
    }

    Internet(Internet const&) = delete;
    Internet& operator=(Internet const&) = delete;
};

struct HttpRequest {
    std::wstring method = L"GET";
    std::wstring path;
    std::wstring headers;
    std::vector<char> body;
    int timeoutMs = 30000;
};

struct HttpResponse {
    DWORD status = 0;
    std::vector<char> body;

    std::string text() const { return std::string(body.begin(), body.end()); }
};

class HttpStream {
public:
    HttpStream(HttpEndpoint const& endpoint, HttpRequest const& request);
    DWORD read(char* buffer, DWORD size);

    DWORD status = 0;

private:
    Internet connection;
    Internet request;
};

HttpResponse httpSend(HttpEndpoint const& endpoint, HttpRequest const& request);

class Multipart {
public:
    Multipart();

    void field(std::string const& name, std::string const& value);
    void file(std::string const& name, std::string const& fileName, std::string const& mimeType,
              char const* data, size_t size);
    std::wstring contentType() const;
    std::vector<char> finish();

private:
    std::string boundary;
    std::vector<char> body;
};

}

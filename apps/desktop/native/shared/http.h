#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace fileship {

struct HttpEndpoint {
    std::wstring baseUrl;
    std::wstring token;
    std::wstring userAgent;
    std::wstring language;
};

struct HttpRequest {
    std::wstring method = L"GET";
    std::wstring path;
    std::wstring headers;
    std::vector<char> body;
    int timeoutMs = 30000;
    std::function<void(size_t sent, size_t total)> onSent;
};

struct HttpInterrupted : std::runtime_error {
    HttpInterrupted() : std::runtime_error("interrupted") {}
};

struct HttpTimeout : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct HttpResponse {
    uint32_t status = 0;
    std::vector<char> body;

    std::string text() const { return std::string(body.begin(), body.end()); }
};

class HttpStream {
public:
    HttpStream(HttpEndpoint const& endpoint, HttpRequest const& request);
    ~HttpStream();

    HttpStream(HttpStream const&) = delete;
    HttpStream& operator=(HttpStream const&) = delete;

    size_t read(char* buffer, size_t size);

    uint32_t status = 0;

private:
    struct State;
    std::unique_ptr<State> state;
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

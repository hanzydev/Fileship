#pragma once

#include <cstdint>
#include <memory>
#include <optional>
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
};

struct HttpResponse {
    int status = 0;
    std::vector<char> body;

    std::string text() const { return std::string(body.begin(), body.end()); }
};

class HttpError : public std::runtime_error {
public:
    HttpError(std::string const& message, bool timeout = false) : std::runtime_error(message), timeout(timeout) {}

    bool timeout;
};

class HttpStream {
public:
    HttpStream(HttpEndpoint const& endpoint, HttpRequest const& request);
    ~HttpStream();
    HttpStream(HttpStream const&) = delete;
    HttpStream& operator=(HttpStream const&) = delete;

    size_t read(char* buffer, size_t size);

    int status = 0;
    int64_t contentLength = -1;

private:
    struct Transport;
    std::unique_ptr<Transport> transport;
};

HttpResponse httpSend(HttpEndpoint const& endpoint, HttpRequest const& request);
std::wstring requestHeaders(HttpEndpoint const& endpoint, std::wstring const& extra);

class WebSocket {
public:
    WebSocket();
    ~WebSocket();
    WebSocket(WebSocket const&) = delete;
    WebSocket& operator=(WebSocket const&) = delete;

    int open(HttpEndpoint const& endpoint, std::wstring const& path, int connectTimeoutMs, int receiveTimeoutMs);
    std::optional<std::string> receive();
    bool send(std::string const& text);
    void close();

    std::string error;

private:
    struct Transport;
    std::unique_ptr<Transport> transport;
};

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

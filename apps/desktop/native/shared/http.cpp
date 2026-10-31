#include "http.h"

#include "system.h"

#include <cstdio>

namespace fileship {

HttpResponse httpSend(HttpEndpoint const& endpoint, HttpRequest const& request) {
    HttpStream stream(endpoint, request);
    HttpResponse response{stream.status, {}};
    std::vector<char> buffer(64 * 1024);

    while (size_t read = stream.read(buffer.data(), buffer.size())) {
        response.body.insert(response.body.end(), buffer.data(), buffer.data() + read);
    }

    return response;
}

std::wstring requestHeaders(HttpEndpoint const& endpoint, std::wstring const& extra) {
    std::wstring headers = L"User-Agent: " + endpoint.userAgent + L"\r\n";
    if (!endpoint.token.empty()) headers += L"Authorization: Bearer " + endpoint.token + L"\r\n";
    if (!endpoint.language.empty()) headers += L"Accept-Language: " + endpoint.language + L"\r\n";
    return headers + extra;
}

Multipart::Multipart() {
    unsigned char random[12]{};
    randomBytes(random, sizeof(random));

    boundary = "----FileshipBoundary";
    for (auto byte : random) {
        char hex[3]{};
        snprintf(hex, sizeof(hex), "%02x", byte);
        boundary += hex;
    }
}

void Multipart::field(std::string const& name, std::string const& value) {
    std::string part = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"" + name +
                       "\"\r\n\r\n" + value + "\r\n";
    body.insert(body.end(), part.begin(), part.end());
}

void Multipart::file(std::string const& name, std::string const& fileName, std::string const& mimeType,
                     char const* data, size_t size) {
    std::string header = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"" + name +
                         "\"; filename=\"" + fileName + "\"\r\nContent-Type: " + mimeType +
                         "\r\n\r\n";
    body.insert(body.end(), header.begin(), header.end());
    body.insert(body.end(), data, data + size);
    body.push_back('\r');
    body.push_back('\n');
}

std::wstring Multipart::contentType() const {
    return L"multipart/form-data; boundary=" + std::wstring(boundary.begin(), boundary.end());
}

std::vector<char> Multipart::finish() {
    std::string end = "--" + boundary + "--\r\n";
    body.insert(body.end(), end.begin(), end.end());
    return std::move(body);
}

}

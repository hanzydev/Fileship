#include "http.h"

#include <bcrypt.h>

#include <iterator>
#include <mutex>
#include <stdexcept>

namespace fileship {

namespace {

[[noreturn]] void throwLastError(char const* what) {
    throw std::runtime_error(std::string(what) + " failed with " + std::to_string(GetLastError()));
}

HINTERNET session(std::wstring const& userAgent) {
    static std::mutex mutex;
    static HINTERNET handle = nullptr;

    std::lock_guard lock(mutex);
    if (!handle) {
        handle = WinHttpOpen(userAgent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                             WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!handle) throwLastError("WinHttpOpen");
    }

    return handle;
}

}

HttpStream::HttpStream(HttpEndpoint const& endpoint, HttpRequest const& options)
    : connection(nullptr), request(nullptr) {
    URL_COMPONENTS url{};
    url.dwStructSize = sizeof(url);
    wchar_t host[256]{};
    wchar_t base[2048]{};
    url.lpszHostName = host;
    url.dwHostNameLength = static_cast<DWORD>(std::size(host));
    url.lpszUrlPath = base;
    url.dwUrlPathLength = static_cast<DWORD>(std::size(base));

    if (!WinHttpCrackUrl(endpoint.baseUrl.c_str(), 0, 0, &url)) throwLastError("WinHttpCrackUrl");

    std::wstring path(base);
    while (!path.empty() && path.back() == L'/') path.pop_back();
    path += options.path;

    connection.value = WinHttpConnect(session(endpoint.userAgent), host, url.nPort, 0);
    if (!connection.value) throwLastError("WinHttpConnect");

    DWORD flags = url.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    request.value = WinHttpOpenRequest(connection.value, options.method.c_str(), path.c_str(), nullptr,
                                       WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request.value) throwLastError("WinHttpOpenRequest");

    int timeout = options.timeoutMs;
    WinHttpSetTimeouts(request.value, timeout, timeout, timeout, timeout);

    std::wstring headers = L"User-Agent: " + endpoint.userAgent + L"\r\n";
    if (!endpoint.token.empty()) headers += L"Authorization: Bearer " + endpoint.token + L"\r\n";
    if (!endpoint.language.empty()) headers += L"Accept-Language: " + endpoint.language + L"\r\n";
    headers += options.headers;

    auto body = options.body.empty() ? WINHTTP_NO_REQUEST_DATA
                                     : const_cast<char*>(options.body.data());
    auto length = static_cast<DWORD>(options.body.size());

    if (!WinHttpSendRequest(request.value, headers.c_str(), static_cast<DWORD>(headers.size()), body,
                            length, length, 0) ||
        !WinHttpReceiveResponse(request.value, nullptr)) {
        throwLastError("WinHttpSendRequest");
    }

    DWORD size = sizeof(status);
    WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
}

DWORD HttpStream::read(char* buffer, DWORD size) {
    DWORD read = 0;
    if (!WinHttpReadData(request.value, buffer, size, &read)) throwLastError("WinHttpReadData");
    return read;
}

HttpResponse httpSend(HttpEndpoint const& endpoint, HttpRequest const& request) {
    HttpStream stream(endpoint, request);
    HttpResponse response{stream.status, {}};
    std::vector<char> buffer(64 * 1024);

    while (DWORD read = stream.read(buffer.data(), static_cast<DWORD>(buffer.size()))) {
        response.body.insert(response.body.end(), buffer.data(), buffer.data() + read);
    }

    return response;
}

Multipart::Multipart() {
    unsigned char random[12]{};
    BCryptGenRandom(nullptr, random, sizeof(random), BCRYPT_USE_SYSTEM_PREFERRED_RNG);

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

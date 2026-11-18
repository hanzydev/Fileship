#include <shared/http.h>

#include "winhttp.h"

#include <algorithm>
#include <iterator>
#include <mutex>

namespace fileship {

namespace {

constexpr DWORD WRITE_BLOCK = 256 * 1024;

[[noreturn]] void throwLastError(char const* what) {
    DWORD error = GetLastError();
    std::string message = std::string(what) + " failed with " + std::to_string(error);

    if (error == ERROR_WINHTTP_TIMEOUT) throw HttpTimeout(message);
    throw std::runtime_error(message);
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

bool crackUrl(std::wstring const& url, CrackedUrl& result) {
    URL_COMPONENTS components{};
    components.dwStructSize = sizeof(components);
    wchar_t host[256]{};
    wchar_t path[2048]{};
    components.lpszHostName = host;
    components.dwHostNameLength = static_cast<DWORD>(std::size(host));
    components.lpszUrlPath = path;
    components.dwUrlPathLength = static_cast<DWORD>(std::size(path));

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &components)) return false;

    result.host = host;
    result.path = path;
    result.port = components.nPort;
    result.isSecure = components.nScheme == INTERNET_SCHEME_HTTPS;

    while (!result.path.empty() && result.path.back() == L'/') result.path.pop_back();
    return true;
}

struct HttpStream::State {
    Internet connection{nullptr};
    Internet request{nullptr};
};

HttpStream::HttpStream(HttpEndpoint const& endpoint, HttpRequest const& options)
    : state(std::make_unique<State>()) {
    auto& connection = state->connection;
    auto& request = state->request;

    CrackedUrl url;
    if (!crackUrl(endpoint.baseUrl, url)) throwLastError("WinHttpCrackUrl");

    std::wstring path = url.path + options.path;

    connection.value = WinHttpConnect(session(endpoint.userAgent), url.host.c_str(), url.port, 0);
    if (!connection.value) throwLastError("WinHttpConnect");

    DWORD flags = url.isSecure ? WINHTTP_FLAG_SECURE : 0;
    request.value = WinHttpOpenRequest(connection.value, options.method.c_str(), path.c_str(), nullptr,
                                       WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request.value) throwLastError("WinHttpOpenRequest");

    int timeout = options.timeoutMs;
    WinHttpSetTimeouts(request.value, timeout, timeout, timeout, timeout);

    std::wstring headers = L"User-Agent: " + endpoint.userAgent + L"\r\n";
    if (!endpoint.token.empty()) headers += L"Authorization: Bearer " + endpoint.token + L"\r\n";
    if (!endpoint.language.empty()) headers += L"Accept-Language: " + endpoint.language + L"\r\n";
    headers += options.headers;

    auto length = static_cast<DWORD>(options.body.size());

    if (options.onSent && length) {
        if (!WinHttpSendRequest(request.value, headers.c_str(), static_cast<DWORD>(headers.size()),
                                WINHTTP_NO_REQUEST_DATA, 0, length, 0)) {
            throwLastError("WinHttpSendRequest");
        }

        DWORD sent = 0;
        while (sent < length) {
            DWORD written = 0;
            DWORD block = std::min<DWORD>(WRITE_BLOCK, length - sent);
            if (!WinHttpWriteData(request.value, options.body.data() + sent, block, &written) || !written) {
                throwLastError("WinHttpSendRequest");
            }

            sent += written;
            options.onSent(sent, length);
        }

        if (!WinHttpReceiveResponse(request.value, nullptr)) throwLastError("WinHttpSendRequest");
    } else {
        auto body = options.body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(options.body.data());

        if (!WinHttpSendRequest(request.value, headers.c_str(), static_cast<DWORD>(headers.size()), body,
                                length, length, 0) ||
            !WinHttpReceiveResponse(request.value, nullptr)) {
            throwLastError("WinHttpSendRequest");
        }
    }

    DWORD code = 0;
    DWORD size = sizeof(code);
    WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &code, &size, WINHTTP_NO_HEADER_INDEX);
    status = code;
}

HttpStream::~HttpStream() = default;

size_t HttpStream::read(char* buffer, size_t size) {
    DWORD read = 0;
    if (!WinHttpReadData(state->request.value, buffer, static_cast<DWORD>(size), &read)) {
        throwLastError("WinHttpReadData");
    }

    return read;
}

}

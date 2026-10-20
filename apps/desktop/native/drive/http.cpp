#include "http.h"

using namespace winrt;

namespace drive {

namespace {

HINTERNET session() {
    static HINTERNET handle = [] {
        HINTERNET opened = WinHttpOpen(config().userAgent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                       WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        WinHttpSetTimeouts(opened, 15000, 15000, 30000, 60000);
        return opened;
    }();

    return handle;
}

}

HttpStream::HttpStream(std::wstring const& path, std::wstring const& headers)
    : connection(nullptr), request(nullptr) {
    URL_COMPONENTS url{};
    url.dwStructSize = sizeof(url);
    wchar_t host[256]{};
    wchar_t base[2048]{};
    url.lpszHostName = host;
    url.dwHostNameLength = static_cast<DWORD>(std::size(host));
    url.lpszUrlPath = base;
    url.dwUrlPathLength = static_cast<DWORD>(std::size(base));

    std::wstring instanceUrl = config().instanceUrl;
    if (!WinHttpCrackUrl(instanceUrl.c_str(), 0, 0, &url)) throw_last_error();

    std::wstring requestPath(base);
    while (!requestPath.empty() && requestPath.back() == L'/') requestPath.pop_back();
    requestPath += path;

    connection.value = WinHttpConnect(session(), host, url.nPort, 0);
    if (!connection.value) throw_last_error();

    DWORD flags = url.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    request.value = WinHttpOpenRequest(connection.value, L"GET", requestPath.c_str(), nullptr, WINHTTP_NO_REFERER,
                                       WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request.value) throw_last_error();

    std::wstring allHeaders = L"Authorization: Bearer " + token() + L"\r\n" + headers;

    if (!WinHttpSendRequest(request.value, allHeaders.c_str(), static_cast<DWORD>(allHeaders.size()),
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request.value, nullptr)) {
        throw_last_error();
    }

    DWORD size = sizeof(status);
    WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
}

DWORD HttpStream::read(char* buffer, DWORD size) {
    DWORD read = 0;
    if (!WinHttpReadData(request.value, buffer, size, &read)) throw_last_error();
    return read;
}

HttpResponse httpGet(std::wstring const& path) {
    HttpStream stream(path);
    HttpResponse response{stream.status, {}};
    std::vector<char> buffer(64 * 1024);

    while (DWORD read = stream.read(buffer.data(), static_cast<DWORD>(buffer.size()))) {
        response.body.insert(response.body.end(), buffer.data(), buffer.data() + read);
    }

    return response;
}

}

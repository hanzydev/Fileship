#include <shared/http.h>

#include <windows.h>

#include <winhttp.h>

#include <iterator>
#include <mutex>

namespace fileship {

namespace {

struct Handle {
    HINTERNET value = nullptr;

    Handle() = default;
    explicit Handle(HINTERNET handle) : value(handle) {}

    ~Handle() {
        reset();
    }

    Handle(Handle const&) = delete;
    Handle& operator=(Handle const&) = delete;

    void reset(HINTERNET handle = nullptr) {
        if (value) WinHttpCloseHandle(value);
        value = handle;
    }

    HINTERNET release() {
        auto handle = value;
        value = nullptr;
        return handle;
    }
};

[[noreturn]] void throwLastError(char const* what) {
    auto code = GetLastError();
    throw HttpError(std::string(what) + " failed with " + std::to_string(code), code == ERROR_WINHTTP_TIMEOUT);
}

HINTERNET session(std::wstring const& userAgent) {
    static std::mutex mutex;
    static HINTERNET handle = nullptr;

    std::lock_guard lock(mutex);
    if (!handle) {
        handle = WinHttpOpen(userAgent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                             WINHTTP_NO_PROXY_BYPASS, 0);
        if (!handle) throwLastError("WinHttpOpen");
    }

    return handle;
}

struct Target {
    std::wstring host;
    std::wstring path;
    INTERNET_PORT port = 0;
    bool secure = false;
};

std::optional<Target> crack(std::wstring const& baseUrl, std::wstring const& suffix) {
    URL_COMPONENTS url{};
    url.dwStructSize = sizeof(url);
    wchar_t host[256]{};
    wchar_t base[2048]{};
    url.lpszHostName = host;
    url.dwHostNameLength = static_cast<DWORD>(std::size(host));
    url.lpszUrlPath = base;
    url.dwUrlPathLength = static_cast<DWORD>(std::size(base));

    if (!WinHttpCrackUrl(baseUrl.c_str(), 0, 0, &url)) return std::nullopt;

    std::wstring path(base);
    while (!path.empty() && path.back() == L'/') path.pop_back();
    return Target{host, path + suffix, url.nPort, url.nScheme == INTERNET_SCHEME_HTTPS};
}

}

struct HttpStream::Transport {
    Handle connection;
    Handle request;
};

HttpStream::HttpStream(HttpEndpoint const& endpoint, HttpRequest const& options)
    : transport(std::make_unique<Transport>()) {
    auto target = crack(endpoint.baseUrl, options.path);
    if (!target) throwLastError("WinHttpCrackUrl");

    transport->connection.reset(WinHttpConnect(session(endpoint.userAgent), target->host.c_str(), target->port, 0));
    if (!transport->connection.value) throwLastError("WinHttpConnect");

    transport->request.reset(WinHttpOpenRequest(transport->connection.value, options.method.c_str(),
                                                target->path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                                WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                target->secure ? WINHTTP_FLAG_SECURE : 0));
    if (!transport->request.value) throwLastError("WinHttpOpenRequest");

    int timeout = options.timeoutMs;
    WinHttpSetTimeouts(transport->request.value, timeout, timeout, timeout, timeout);

    auto headers = requestHeaders(endpoint, options.headers);
    auto body = options.body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(options.body.data());
    auto length = static_cast<DWORD>(options.body.size());

    if (!WinHttpSendRequest(transport->request.value, headers.c_str(), static_cast<DWORD>(headers.size()), body,
                            length, length, 0) ||
        !WinHttpReceiveResponse(transport->request.value, nullptr)) {
        throwLastError("WinHttpSendRequest");
    }

    DWORD code = 0;
    DWORD size = sizeof(code);
    WinHttpQueryHeaders(transport->request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &code, &size, WINHTTP_NO_HEADER_INDEX);
    status = static_cast<int>(code);

    wchar_t lengthText[32]{};
    DWORD lengthSize = sizeof(lengthText);
    if (WinHttpQueryHeaders(transport->request.value, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX,
                            lengthText, &lengthSize, WINHTTP_NO_HEADER_INDEX)) {
        contentLength = std::wcstoll(lengthText, nullptr, 10);
    }
}

HttpStream::~HttpStream() = default;

size_t HttpStream::read(char* buffer, size_t size) {
    DWORD read = 0;
    if (!WinHttpReadData(transport->request.value, buffer, static_cast<DWORD>(size), &read))
        throwLastError("WinHttpReadData");
    return read;
}

struct WebSocket::Transport {
    std::mutex mutex;
    Handle internet;
    Handle connection;
    Handle socket;
};

WebSocket::WebSocket() : transport(std::make_unique<Transport>()) {}

WebSocket::~WebSocket() {
    close();
}

int WebSocket::open(HttpEndpoint const& endpoint, std::wstring const& path, int connectTimeoutMs,
                    int receiveTimeoutMs) {
    auto target = crack(endpoint.baseUrl, path);
    if (!target) throw std::invalid_argument("Invalid address");

    Handle internet(WinHttpOpen(endpoint.userAgent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                                WINHTTP_NO_PROXY_BYPASS, 0));
    if (!internet.value) {
        error = "WinHttpOpen failed with " + std::to_string(GetLastError());
        return 0;
    }

    Handle connection(WinHttpConnect(internet.value, target->host.c_str(), target->port, 0));
    if (!connection.value) {
        error = "WinHttpConnect failed with " + std::to_string(GetLastError());
        return 0;
    }

    Handle request(WinHttpOpenRequest(connection.value, L"GET", target->path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                      WINHTTP_DEFAULT_ACCEPT_TYPES, target->secure ? WINHTTP_FLAG_SECURE : 0));
    if (!request.value) {
        error = "WinHttpOpenRequest failed with " + std::to_string(GetLastError());
        return 0;
    }

    WinHttpSetTimeouts(request.value, connectTimeoutMs, connectTimeoutMs, connectTimeoutMs, receiveTimeoutMs);
    WinHttpSetOption(request.value, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0);

    auto headers = requestHeaders({endpoint.baseUrl, endpoint.token, endpoint.userAgent, {}}, {});

    DWORD status = 0;
    DWORD size = sizeof(status);
    bool sent = WinHttpSendRequest(request.value, headers.c_str(), static_cast<DWORD>(headers.size()),
                                   WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                WinHttpReceiveResponse(request.value, nullptr) &&
                WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                    WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);

    if (!sent) {
        error = "connection failed with " + std::to_string(GetLastError());
        return 0;
    }
    if (status != 101) return static_cast<int>(status);

    Handle socket(WinHttpWebSocketCompleteUpgrade(request.value, 0));
    if (!socket.value) {
        error = "WinHttpWebSocketCompleteUpgrade failed with " + std::to_string(GetLastError());
        return 0;
    }

    DWORD timeout = receiveTimeoutMs;
    WinHttpSetOption(socket.value, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    std::lock_guard lock(transport->mutex);
    transport->internet.reset(internet.release());
    transport->connection.reset(connection.release());
    transport->socket.reset(socket.release());
    return 101;
}

std::optional<std::string> WebSocket::receive() {
    HINTERNET socket = nullptr;
    {
        std::lock_guard lock(transport->mutex);
        socket = transport->socket.value;
    }
    if (!socket) return std::nullopt;

    std::vector<char> buffer(64 * 1024);
    std::string message;

    while (true) {
        DWORD read = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE type{};
        DWORD result = WinHttpWebSocketReceive(socket, buffer.data(), static_cast<DWORD>(buffer.size()), &read, &type);
        if (result != NO_ERROR || type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) return std::nullopt;

        if (type == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE || type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE)
            message.append(buffer.data(), read);

        if (type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE) return message;
        if (type == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE) message.clear();
    }
}

bool WebSocket::send(std::string const& text) {
    std::lock_guard lock(transport->mutex);
    if (!transport->socket.value) return false;

    return WinHttpWebSocketSend(transport->socket.value, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
                                const_cast<char*>(text.data()), static_cast<DWORD>(text.size())) == NO_ERROR;
}

void WebSocket::close() {
    std::lock_guard lock(transport->mutex);

    if (transport->socket.value) {
        WinHttpWebSocketShutdown(transport->socket.value, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, nullptr, 0);
        transport->socket.reset();
    }

    transport->connection.reset();
    transport->internet.reset();
}

}

#include <shared/websocket.h>

#include "winhttp.h"

#include <mutex>
#include <vector>

namespace fileship {

namespace {

std::runtime_error lastError(char const* what) {
    return std::runtime_error(std::string(what) + " with " + std::to_string(GetLastError()));
}

}

struct WebSocket::State {
    std::mutex mutex;
    HINTERNET internet = nullptr;
    HINTERNET connection = nullptr;
    HINTERNET socket = nullptr;

    void close() {
        std::lock_guard lock(mutex);

        if (socket) {
            WinHttpWebSocketShutdown(socket, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, nullptr, 0);
            WinHttpCloseHandle(socket);
            socket = nullptr;
        }

        if (connection) {
            WinHttpCloseHandle(connection);
            connection = nullptr;
        }
    }

    HINTERNET current() {
        std::lock_guard lock(mutex);
        return socket;
    }

    ~State() {
        close();
        if (internet) WinHttpCloseHandle(internet);
    }
};

WebSocket::WebSocket(HttpEndpoint const& endpoint, std::wstring const& path, int connectTimeoutMs,
                     int receiveTimeoutMs)
    : state(std::make_unique<State>()) {
    state->internet = WinHttpOpen(endpoint.userAgent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!state->internet) throw lastError("WinHttpOpen failed");

    CrackedUrl url;
    if (!crackUrl(endpoint.baseUrl, url)) throw InvalidAddress("invalid instance address");

    state->connection = WinHttpConnect(state->internet, url.host.c_str(), url.port, 0);
    if (!state->connection) throw lastError("connection failed");

    Internet request(WinHttpOpenRequest(state->connection, L"GET", (url.path + path).c_str(), nullptr,
                                        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                        url.isSecure ? WINHTTP_FLAG_SECURE : 0));
    if (!request.value) throw lastError("connection failed");

    WinHttpSetTimeouts(request.value, connectTimeoutMs, connectTimeoutMs, connectTimeoutMs, receiveTimeoutMs);
    WinHttpSetOption(request.value, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0);

    std::wstring headers = L"User-Agent: " + endpoint.userAgent + L"\r\n";
    if (!endpoint.token.empty()) headers += L"Authorization: Bearer " + endpoint.token + L"\r\n";

    DWORD status = 0;
    DWORD size = sizeof(status);
    bool sent = WinHttpSendRequest(request.value, headers.c_str(), static_cast<DWORD>(headers.size()),
                                   WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                WinHttpReceiveResponse(request.value, nullptr) &&
                WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                    WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);

    if (!sent) throw lastError("connection failed");
    if (status != 101) throw std::runtime_error("upgrade refused with " + std::to_string(status));

    state->socket = WinHttpWebSocketCompleteUpgrade(request.value, 0);
    if (!state->socket) throw lastError("upgrade failed");

    DWORD timeout = static_cast<DWORD>(receiveTimeoutMs);
    WinHttpSetOption(state->socket, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
}

WebSocket::~WebSocket() = default;

std::optional<std::string> WebSocket::receive() {
    std::vector<char> buffer(64 * 1024);
    std::string message;

    while (true) {
        HINTERNET socket = state->current();
        if (!socket) return std::nullopt;

        DWORD read = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE type{};
        DWORD error = WinHttpWebSocketReceive(socket, buffer.data(), static_cast<DWORD>(buffer.size()), &read, &type);
        if (error != NO_ERROR || type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) return std::nullopt;

        if (type == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE ||
            type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE) {
            message.append(buffer.data(), read);
        }

        if (type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE) return message;
        if (type == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE) message.clear();
    }
}

bool WebSocket::send(std::string const& text) {
    HINTERNET socket = state->current();
    if (!socket) return false;

    return WinHttpWebSocketSend(socket, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
                                const_cast<char*>(text.data()), static_cast<DWORD>(text.size())) == NO_ERROR;
}

void WebSocket::close() {
    state->close();
}

}

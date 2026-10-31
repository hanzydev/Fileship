#include <shared/http.h>
#include <shared/system.h>
#include <shared/url.h>
#include <shared/utf8.h>

#include <curl/curl.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <mutex>

namespace fileship {

namespace {

void initCurl() {
    static std::once_flag once;
    std::call_once(once, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

std::string joinUrl(std::wstring const& base, std::wstring const& path) {
    auto url = toUtf8(base);
    while (!url.empty() && url.back() == '/') url.pop_back();
    return url + toUtf8(path);
}

curl_slist* headerList(std::wstring const& headers) {
    curl_slist* list = nullptr;
    auto text = toUtf8(headers);

    for (size_t start = 0; start < text.size();) {
        auto end = text.find("\r\n", start);
        if (end == std::string::npos) end = text.size();
        if (end > start) list = curl_slist_append(list, text.substr(start, end - start).c_str());
        start = end + 2;
    }

    return curl_slist_append(list, "Expect:");
}

[[noreturn]] void fail(CURLcode code, char const* detail) {
    throw HttpError(detail && *detail ? detail : curl_easy_strerror(code), code == CURLE_OPERATION_TIMEDOUT);
}

size_t collect(char* data, size_t size, size_t count, void* target) {
    static_cast<std::string*>(target)->append(data, size * count);
    return size * count;
}

std::string base64(uint8_t const* bytes, size_t size) {
    static constexpr char ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;

    for (size_t index = 0; index < size; index += 3) {
        uint32_t chunk = static_cast<uint32_t>(bytes[index]) << 16;
        if (index + 1 < size) chunk |= static_cast<uint32_t>(bytes[index + 1]) << 8;
        if (index + 2 < size) chunk |= bytes[index + 2];

        result += ALPHABET[(chunk >> 18) & 0x3F];
        result += ALPHABET[(chunk >> 12) & 0x3F];
        result += index + 1 < size ? ALPHABET[(chunk >> 6) & 0x3F] : '=';
        result += index + 2 < size ? ALPHABET[chunk & 0x3F] : '=';
    }

    return result;
}

}

struct HttpStream::Transport {
    CURLM* multi = curl_multi_init();
    CURL* easy = curl_easy_init();
    curl_slist* headers = nullptr;
    std::vector<char> body;
    std::string pending;
    size_t offset = 0;
    bool done = false;
    CURLcode result = CURLE_OK;
    char error[CURL_ERROR_SIZE]{};

    ~Transport() {
        curl_multi_remove_handle(multi, easy);
        curl_easy_cleanup(easy);
        curl_multi_cleanup(multi);
        curl_slist_free_all(headers);
    }

    void pump() {
        int running = 0;
        curl_multi_perform(multi, &running);

        int queued = 0;
        while (auto message = curl_multi_info_read(multi, &queued)) {
            if (message->msg != CURLMSG_DONE) continue;
            result = message->data.result;
            done = true;
        }

        if (!done && pending.size() == offset) curl_multi_poll(multi, nullptr, 0, 1000, nullptr);
    }
};

HttpStream::HttpStream(HttpEndpoint const& endpoint, HttpRequest const& options)
    : transport(std::make_unique<Transport>()) {
    initCurl();

    auto easy = transport->easy;
    auto url = joinUrl(endpoint.baseUrl, options.path);
    auto method = toUtf8(options.method);
    long seconds = std::max(1, options.timeoutMs / 1000);

    transport->headers = headerList(requestHeaders(endpoint, options.headers));
    transport->body = options.body;

    curl_easy_setopt(easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(easy, CURLOPT_HTTPHEADER, transport->headers);
    curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easy, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(options.timeoutMs));
    curl_easy_setopt(easy, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(easy, CURLOPT_LOW_SPEED_TIME, seconds);
    curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, transport->error);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, collect);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, &transport->pending);

    if (method == "HEAD") {
        curl_easy_setopt(easy, CURLOPT_NOBODY, 1L);
    } else if (method != "GET") {
        curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, method.c_str());
        curl_easy_setopt(easy, CURLOPT_POSTFIELDS, transport->body.data());
        curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(transport->body.size()));
    }

    curl_multi_add_handle(transport->multi, easy);

    while (transport->pending.empty() && !transport->done) transport->pump();
    if (transport->done && transport->result != CURLE_OK) fail(transport->result, transport->error);

    long code = 0;
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &code);
    status = static_cast<int>(code);

    curl_off_t length = -1;
    curl_easy_getinfo(easy, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &length);
    contentLength = length;
}

HttpStream::~HttpStream() = default;

size_t HttpStream::read(char* buffer, size_t size) {
    auto& state = *transport;

    while (state.offset == state.pending.size() && !state.done) {
        state.pending.clear();
        state.offset = 0;
        state.pump();
    }

    if (state.offset == state.pending.size()) {
        if (state.result != CURLE_OK) fail(state.result, state.error);
        return 0;
    }

    auto count = std::min(size, state.pending.size() - state.offset);
    std::copy_n(state.pending.data() + state.offset, count, buffer);
    state.offset += count;
    return count;
}

struct WebSocket::Transport {
    std::mutex mutex;
    CURL* easy = nullptr;
    curl_socket_t socket = CURL_SOCKET_BAD;
    int wake = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    std::string buffer;
    std::atomic<bool> closed = false;
    int receiveTimeoutMs = 60000;

    ~Transport() {
        if (easy) curl_easy_cleanup(easy);
        ::close(wake);
    }

    bool waitFor(short events, int timeoutMs) {
        pollfd targets[] = {{socket, events, 0}, {wake, POLLIN, 0}};
        int ready = poll(targets, 2, timeoutMs);
        return ready > 0 && !(targets[1].revents & POLLIN) && targets[0].revents;
    }

    bool sendAll(char const* data, size_t size, int timeoutMs) {
        std::lock_guard lock(mutex);

        while (size) {
            size_t sent = 0;
            auto code = curl_easy_send(easy, data, size, &sent);
            if (code == CURLE_AGAIN) {
                if (!waitFor(POLLOUT, timeoutMs)) return false;
                continue;
            }
            if (code != CURLE_OK) return false;

            data += sent;
            size -= sent;
        }

        return true;
    }

    bool receiveMore(int timeoutMs) {
        while (!closed) {
            char chunk[16 * 1024];
            size_t received = 0;
            CURLcode code;

            {
                std::lock_guard lock(mutex);
                code = curl_easy_recv(easy, chunk, sizeof(chunk), &received);
            }

            if (code == CURLE_OK) {
                if (!received) return false;
                buffer.append(chunk, received);
                return true;
            }

            if (code != CURLE_AGAIN || !waitFor(POLLIN, timeoutMs)) return false;
        }

        return false;
    }

    bool sendFrame(uint8_t opcode, std::string_view payload) {
        std::string frame;
        frame += static_cast<char>(0x80 | opcode);

        if (payload.size() < 126) {
            frame += static_cast<char>(0x80 | payload.size());
        } else if (payload.size() < 65536) {
            frame += static_cast<char>(0x80 | 126);
            frame += static_cast<char>((payload.size() >> 8) & 0xFF);
            frame += static_cast<char>(payload.size() & 0xFF);
        } else {
            frame += static_cast<char>(0x80 | 127);
            for (int shift = 56; shift >= 0; shift -= 8) frame += static_cast<char>((payload.size() >> shift) & 0xFF);
        }

        uint8_t mask[4];
        randomBytes(mask, sizeof(mask));
        frame.append(reinterpret_cast<char*>(mask), sizeof(mask));
        for (size_t index = 0; index < payload.size(); index++)
            frame += static_cast<char>(payload[index] ^ mask[index % 4]);

        return sendAll(frame.data(), frame.size(), receiveTimeoutMs);
    }
};

WebSocket::WebSocket() : transport(std::make_unique<Transport>()) {}

WebSocket::~WebSocket() {
    close();
}

int WebSocket::open(HttpEndpoint const& endpoint, std::wstring const& path, int connectTimeoutMs,
                    int receiveTimeoutMs) {
    initCurl();

    auto target = parseUrl(endpoint.baseUrl);
    if (!target) throw std::invalid_argument("Invalid address");

    auto& state = *transport;
    state.receiveTimeoutMs = receiveTimeoutMs;
    state.easy = curl_easy_init();

    auto url = joinUrl(endpoint.baseUrl, path);
    char failure[CURL_ERROR_SIZE]{};
    curl_easy_setopt(state.easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(state.easy, CURLOPT_CONNECT_ONLY, 1L);
    curl_easy_setopt(state.easy, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(state.easy, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(connectTimeoutMs));
    curl_easy_setopt(state.easy, CURLOPT_ERRORBUFFER, failure);

    if (auto code = curl_easy_perform(state.easy); code != CURLE_OK) {
        error = *failure ? failure : curl_easy_strerror(code);
        return 0;
    }

    curl_easy_getinfo(state.easy, CURLINFO_ACTIVESOCKET, &state.socket);

    uint8_t key[16];
    randomBytes(key, sizeof(key));

    auto host = toUtf8(target->host) + (target->isDefaultPort() ? "" : ":" + std::to_string(target->port));
    auto base = toUtf8(target->path);
    while (!base.empty() && base.back() == '/') base.pop_back();

    std::string request = "GET " + base + toUtf8(path) + " HTTP/1.1\r\nHost: " + host +
                          "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\n"
                          "Sec-WebSocket-Key: " +
                          base64(key, sizeof(key)) + "\r\n" +
                          toUtf8(requestHeaders({endpoint.baseUrl, endpoint.token, endpoint.userAgent, {}}, {})) +
                          "\r\n";

    if (!state.sendAll(request.data(), request.size(), connectTimeoutMs)) {
        error = "could not send the upgrade request";
        return 0;
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(connectTimeoutMs);
    size_t end = std::string::npos;

    while ((end = state.buffer.find("\r\n\r\n")) == std::string::npos) {
        auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
        if (left.count() <= 0 || !state.receiveMore(static_cast<int>(left.count()))) {
            error = "no upgrade response";
            return 0;
        }
    }

    auto statusLine = state.buffer.substr(0, state.buffer.find("\r\n"));
    auto space = statusLine.find(' ');
    int status = space == std::string::npos ? 0 : std::atoi(statusLine.c_str() + space + 1);
    state.buffer.erase(0, end + 4);
    return status;
}

std::optional<std::string> WebSocket::receive() {
    auto& state = *transport;
    std::string message;
    uint8_t kind = 0;

    while (!state.closed) {
        auto& buffer = state.buffer;

        if (buffer.size() >= 2) {
            auto first = static_cast<uint8_t>(buffer[0]);
            auto second = static_cast<uint8_t>(buffer[1]);
            uint64_t length = second & 0x7F;
            size_t header = 2;

            if (length == 126 && buffer.size() >= 4) {
                length = (static_cast<uint8_t>(buffer[2]) << 8) | static_cast<uint8_t>(buffer[3]);
                header = 4;
            } else if (length == 127 && buffer.size() >= 10) {
                length = 0;
                for (int index = 2; index < 10; index++) length = (length << 8) | static_cast<uint8_t>(buffer[index]);
                header = 10;
            }

            bool hasLength = (second & 0x7F) < 126 || header > 2;
            bool masked = second & 0x80;
            size_t total = header + (masked ? 4 : 0) + length;

            if (hasLength && buffer.size() >= total) {
                std::string payload = buffer.substr(header + (masked ? 4 : 0), length);
                if (masked) {
                    for (size_t index = 0; index < payload.size(); index++)
                        payload[index] = static_cast<char>(payload[index] ^ buffer[header + index % 4]);
                }
                buffer.erase(0, total);

                auto opcode = first & 0x0F;
                bool final = first & 0x80;

                if (opcode == 0x8) {
                    state.sendFrame(0x8, payload.substr(0, 2));
                    return std::nullopt;
                }
                if (opcode == 0x9) {
                    state.sendFrame(0xA, payload);
                    continue;
                }
                if (opcode == 0xA) continue;

                if (opcode) kind = opcode;
                message += payload;
                if (!final) continue;
                if (kind == 0x2) {
                    message.clear();
                    continue;
                }
                return message;
            }
        }

        if (!state.receiveMore(state.receiveTimeoutMs)) return std::nullopt;
    }

    return std::nullopt;
}

bool WebSocket::send(std::string const& text) {
    return transport->easy && !transport->closed && transport->sendFrame(0x1, text);
}

void WebSocket::close() {
    auto& state = *transport;
    if (!state.easy || state.closed.exchange(true)) return;

    state.sendFrame(0x8, std::string("\x03\xE8", 2));
    uint64_t signal = 1;
    [[maybe_unused]] auto written = ::write(state.wake, &signal, sizeof(signal));
}

}

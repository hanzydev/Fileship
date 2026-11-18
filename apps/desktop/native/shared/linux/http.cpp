#include <shared/http.h>
#include <shared/utf8.h>

#include <libsoup/soup.h>

#include <algorithm>
#include <map>
#include <mutex>

namespace fileship {

namespace {

constexpr size_t WRITE_BLOCK = 256 * 1024;

struct Progress {
    HttpRequest const& request;
    GCancellable* cancellable;
    size_t sent = 0;
    size_t reported = 0;
    std::exception_ptr failure;
};

SoupSession* session(int timeoutMs) {
    static std::mutex mutex;
    static std::map<int, SoupSession*> sessions;

    std::lock_guard lock(mutex);
    auto& handle = sessions[timeoutMs];
    if (!handle) {
        handle = soup_session_new_with_options("timeout", static_cast<guint>(std::max(timeoutMs / 1000, 1)),
                                               "max-conns-per-host", 16, nullptr);
    }

    return handle;
}

[[noreturn]] void throwError(GError* error) {
    bool isTimeout = g_error_matches(error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT);
    std::string message = error->message;
    g_error_free(error);

    if (isTimeout) throw HttpTimeout(message);
    throw std::runtime_error(message);
}

void appendHeaders(SoupMessageHeaders* headers, std::string const& lines) {
    size_t start = 0;

    while (start < lines.size()) {
        size_t end = std::min(lines.find("\r\n", start), lines.size());
        size_t colon = lines.find(':', start);

        if (colon < end) {
            size_t value = lines.find_first_not_of(' ', colon + 1);
            soup_message_headers_replace(headers, lines.substr(start, colon - start).c_str(),
                                         lines.substr(value, end - value).c_str());
        }

        start = end + 2;
    }
}

void handleBodyData(SoupMessage*, guint size, gpointer data) {
    auto progress = static_cast<Progress*>(data);
    size_t total = progress->request.body.size();

    progress->sent += size;
    if (progress->failure || (progress->sent - progress->reported < WRITE_BLOCK && progress->sent < total)) return;

    progress->reported = progress->sent;

    try {
        progress->request.onSent(progress->sent, total);
    } catch (...) {
        progress->failure = std::current_exception();
        g_cancellable_cancel(progress->cancellable);
    }
}

}

struct HttpStream::State {
    SoupMessage* message = nullptr;
    GInputStream* body = nullptr;
    GCancellable* cancellable = g_cancellable_new();

    ~State() {
        g_clear_object(&body);
        g_clear_object(&message);
        g_clear_object(&cancellable);
    }
};

HttpStream::HttpStream(HttpEndpoint const& endpoint, HttpRequest const& options)
    : state(std::make_unique<State>()) {
    auto base = toUtf8(endpoint.baseUrl);
    while (!base.empty() && base.back() == '/') base.pop_back();

    auto url = base + toUtf8(options.path);
    state->message = soup_message_new(toUtf8(options.method).c_str(), url.c_str());
    if (!state->message) throw std::runtime_error("Invalid address " + url);

    soup_message_set_force_http1(state->message, TRUE);

    auto headers = soup_message_get_request_headers(state->message);
    soup_message_headers_replace(headers, "User-Agent", toUtf8(endpoint.userAgent).c_str());
    if (!endpoint.token.empty()) {
        soup_message_headers_replace(headers, "Authorization", ("Bearer " + toUtf8(endpoint.token)).c_str());
    }
    if (!endpoint.language.empty()) {
        soup_message_headers_replace(headers, "Accept-Language", toUtf8(endpoint.language).c_str());
    }
    appendHeaders(headers, toUtf8(options.headers));

    if (!options.body.empty()) {
        GBytes* bytes = g_bytes_new_static(options.body.data(), options.body.size());
        soup_message_set_request_body_from_bytes(state->message, nullptr, bytes);
        g_bytes_unref(bytes);
    }

    Progress progress{options, state->cancellable, 0, 0, nullptr};
    gulong handler = options.onSent ? g_signal_connect(state->message, "wrote-body-data",
                                                        G_CALLBACK(handleBodyData), &progress)
                                    : 0;

    GError* error = nullptr;
    state->body = soup_session_send(session(options.timeoutMs), state->message, state->cancellable, &error);

    if (handler) g_signal_handler_disconnect(state->message, handler);
    if (progress.failure) {
        g_clear_error(&error);
        std::rethrow_exception(progress.failure);
    }
    if (!state->body) throwError(error);

    status = soup_message_get_status(state->message);
}

HttpStream::~HttpStream() = default;

size_t HttpStream::read(char* buffer, size_t size) {
    GError* error = nullptr;
    gssize read = g_input_stream_read(state->body, buffer, size, state->cancellable, &error);
    if (read < 0) throwError(error);

    return static_cast<size_t>(read);
}

}

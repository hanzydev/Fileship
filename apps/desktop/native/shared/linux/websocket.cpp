#include <shared/utf8.h>
#include <shared/websocket.h>

#include <libsoup/soup.h>

#include <atomic>
#include <deque>

namespace fileship {

struct WebSocket::State {
    GMainContext* context = g_main_context_new();
    SoupSession* session = nullptr;
    SoupWebsocketConnection* connection = nullptr;
    std::deque<std::string> messages;
    std::string failure;
    std::atomic<bool> isClosed = false;
    bool isConnecting = true;
    bool isTimedOut = false;
    int receiveTimeoutMs = 0;

    ~State() {
        g_clear_object(&connection);
        g_clear_object(&session);
        g_main_context_unref(context);
    }

    static void handleConnected(GObject* source, GAsyncResult* result, gpointer data) {
        auto state = static_cast<State*>(data);
        GError* error = nullptr;

        state->connection = soup_session_websocket_connect_finish(SOUP_SESSION(source), result, &error);
        state->isConnecting = false;

        if (error) {
            state->failure = error->message;
            g_error_free(error);
        }
    }

    static void handleMessage(SoupWebsocketConnection*, gint type, GBytes* message, gpointer data) {
        if (type != SOUP_WEBSOCKET_DATA_TEXT) return;

        gsize size = 0;
        auto text = static_cast<char const*>(g_bytes_get_data(message, &size));
        static_cast<State*>(data)->messages.emplace_back(text, size);
    }

    static void handleClosed(SoupWebsocketConnection*, gpointer data) {
        static_cast<State*>(data)->isClosed = true;
    }

    static gboolean handleTimeout(gpointer data) {
        static_cast<State*>(data)->isTimedOut = true;
        return G_SOURCE_REMOVE;
    }
};

WebSocket::WebSocket(HttpEndpoint const& endpoint, std::wstring const& path, int connectTimeoutMs,
                     int receiveTimeoutMs)
    : state(std::make_unique<State>()) {
    state->receiveTimeoutMs = receiveTimeoutMs;

    auto base = toUtf8(endpoint.baseUrl);
    while (!base.empty() && base.back() == '/') base.pop_back();

    SoupMessage* message = soup_message_new("GET", (base + toUtf8(path)).c_str());
    if (!message) throw InvalidAddress("invalid instance address");

    auto headers = soup_message_get_request_headers(message);
    soup_message_headers_replace(headers, "User-Agent", toUtf8(endpoint.userAgent).c_str());
    if (!endpoint.token.empty()) {
        soup_message_headers_replace(headers, "Authorization", ("Bearer " + toUtf8(endpoint.token)).c_str());
    }

    g_main_context_push_thread_default(state->context);
    state->session = soup_session_new_with_options("timeout", static_cast<guint>(connectTimeoutMs / 1000), nullptr);
    soup_session_websocket_connect_async(state->session, message, nullptr, nullptr, G_PRIORITY_DEFAULT, nullptr,
                                         State::handleConnected, state.get());
    g_object_unref(message);

    while (state->isConnecting) g_main_context_iteration(state->context, TRUE);
    g_main_context_pop_thread_default(state->context);

    if (!state->connection) throw std::runtime_error("connection failed: " + state->failure);

    soup_websocket_connection_set_max_incoming_payload_size(state->connection, 0);
    g_signal_connect(state->connection, "message", G_CALLBACK(State::handleMessage), state.get());
    g_signal_connect(state->connection, "closed", G_CALLBACK(State::handleClosed), state.get());
}

WebSocket::~WebSocket() = default;

std::optional<std::string> WebSocket::receive() {
    state->isTimedOut = false;

    GSource* timeout = g_timeout_source_new(static_cast<guint>(state->receiveTimeoutMs));
    g_source_set_callback(timeout, State::handleTimeout, state.get(), nullptr);
    g_source_attach(timeout, state->context);

    g_main_context_push_thread_default(state->context);
    while (state->messages.empty() && !state->isClosed && !state->isTimedOut) {
        g_main_context_iteration(state->context, TRUE);
    }
    g_main_context_pop_thread_default(state->context);

    g_source_destroy(timeout);
    g_source_unref(timeout);

    if (state->messages.empty()) return std::nullopt;

    auto message = std::move(state->messages.front());
    state->messages.pop_front();
    return message;
}

bool WebSocket::send(std::string const& text) {
    if (state->isClosed) return false;

    g_main_context_push_thread_default(state->context);
    soup_websocket_connection_send_text(state->connection, text.c_str());
    g_main_context_pop_thread_default(state->context);
    return true;
}

void WebSocket::close() {
    state->isClosed = true;
    g_main_context_wakeup(state->context);
}

}

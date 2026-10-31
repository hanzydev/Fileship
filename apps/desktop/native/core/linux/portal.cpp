#include "linux.h"

#include <gio/gio.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <string>

namespace fileship::core::portal {

namespace {

constexpr char const* PORTAL = "org.freedesktop.portal.Desktop";
constexpr char const* PORTAL_PATH = "/org/freedesktop/portal/desktop";
constexpr char const* REQUEST = "org.freedesktop.portal.Request";
constexpr guint RESPONSE_TIMEOUT_MS = 60000;

std::atomic<uint32_t> requests = 0;

struct Response {
    GMainLoop* loop = nullptr;
    bool isDone = false;
    uint32_t code = 2;
    std::string uri;
};

void handleResponse(GDBusConnection*, gchar const*, gchar const*, gchar const*, gchar const*, GVariant* parameters,
                    gpointer data) {
    auto& response = *static_cast<Response*>(data);
    GVariant* results = nullptr;
    g_variant_get(parameters, "(u@a{sv})", &response.code, &results);

    gchar const* uri = nullptr;
    if (g_variant_lookup(results, "uri", "&s", &uri)) response.uri = uri;
    g_variant_unref(results);

    response.isDone = true;
    g_main_loop_quit(response.loop);
}

std::string requestPath(GDBusConnection* bus, std::string const& token) {
    std::string sender = g_dbus_connection_get_unique_name(bus) + 1;
    std::replace(sender.begin(), sender.end(), '.', '_');
    return std::string(PORTAL_PATH) + "/request/" + sender + "/" + token;
}

std::optional<std::string> takeScreenshot() {
    auto bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
    if (!bus) return std::nullopt;

    auto context = g_main_context_new();
    g_main_context_push_thread_default(context);

    Response response;
    response.loop = g_main_loop_new(context, FALSE);

    auto token = "fileship_screenshot_" + std::to_string(++requests);
    auto path = requestPath(bus, token);
    auto subscription = g_dbus_connection_signal_subscribe(bus, PORTAL, REQUEST, "Response", path.c_str(), nullptr,
                                                           G_DBUS_SIGNAL_FLAGS_NONE, handleResponse, &response, nullptr);

    GVariantBuilder options;
    g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string(token.c_str()));
    g_variant_builder_add(&options, "{sv}", "modal", g_variant_new_boolean(FALSE));
    g_variant_builder_add(&options, "{sv}", "interactive", g_variant_new_boolean(FALSE));

    auto reply = g_dbus_connection_call_sync(bus, PORTAL, PORTAL_PATH, "org.freedesktop.portal.Screenshot",
                                             "Screenshot", g_variant_new("(sa{sv})", "", &options),
                                             G_VARIANT_TYPE("(o)"), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);

    if (reply) {
        gchar const* handle = nullptr;
        g_variant_get(reply, "(&o)", &handle);

        if (path != handle) {
            g_dbus_connection_signal_unsubscribe(bus, subscription);
            path = handle;
            subscription = g_dbus_connection_signal_subscribe(bus, PORTAL, REQUEST, "Response", path.c_str(), nullptr,
                                                              G_DBUS_SIGNAL_FLAGS_NONE, handleResponse, &response,
                                                              nullptr);
        }

        g_variant_unref(reply);

        auto timeout = g_timeout_source_new(RESPONSE_TIMEOUT_MS);
        g_source_set_callback(
            timeout,
            [](gpointer loop) {
                g_main_loop_quit(static_cast<GMainLoop*>(loop));
                return G_SOURCE_REMOVE;
            },
            response.loop, nullptr);
        g_source_attach(timeout, context);

        if (!response.isDone) g_main_loop_run(response.loop);

        g_source_destroy(timeout);
        g_source_unref(timeout);

        if (!response.isDone) {
            g_dbus_connection_call(bus, PORTAL, path.c_str(), REQUEST, "Close", nullptr, nullptr,
                                   G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr, nullptr);
        }
    }

    g_dbus_connection_signal_unsubscribe(bus, subscription);
    while (g_main_context_iteration(context, FALSE)) {
    }

    g_main_context_pop_thread_default(context);
    g_main_loop_unref(response.loop);
    g_main_context_unref(context);
    g_object_unref(bus);

    if (!response.isDone || response.code != 0 || response.uri.empty()) return std::nullopt;
    return response.uri;
}

std::optional<Bitmap> takeImage() {
    auto uri = takeScreenshot();
    if (!uri) return std::nullopt;

    auto file = g_filename_from_uri(uri->c_str(), nullptr, nullptr);
    if (!file) return std::nullopt;

    std::filesystem::path path(file);
    g_free(file);

    auto image = decodeImageFile(path);
    std::error_code error;
    std::filesystem::remove(path, error);
    return image;
}

void copyScaled(Bitmap const& image, Rect const& screen, Rect const& rect, uint8_t* target) {
    double scaleX = static_cast<double>(image.width) / screen.width;
    double scaleY = static_cast<double>(image.height) / screen.height;
    bool isExact = image.width == static_cast<uint32_t>(screen.width) && image.height == static_cast<uint32_t>(screen.height);

    for (int32_t y = 0; y < rect.height; y++) {
        auto row = target + static_cast<size_t>(y) * rect.width * 4;
        auto sourceY = static_cast<int64_t>(std::floor((rect.y + y - screen.y) * scaleY));

        if (sourceY < 0 || sourceY >= image.height) {
            std::memset(row, 0, static_cast<size_t>(rect.width) * 4);
            continue;
        }

        auto source = image.pixels.data() + static_cast<size_t>(sourceY) * image.width * 4;

        if (isExact) {
            int32_t start = std::clamp(screen.x - rect.x, 0, rect.width);
            int32_t end = std::clamp(screen.x + screen.width - rect.x, start, rect.width);
            std::memset(row, 0, static_cast<size_t>(start) * 4);
            std::memcpy(row + static_cast<size_t>(start) * 4, source + static_cast<size_t>(rect.x + start - screen.x) * 4,
                        static_cast<size_t>(end - start) * 4);
            std::memset(row + static_cast<size_t>(end) * 4, 0, static_cast<size_t>(rect.width - end) * 4);
            continue;
        }

        for (int32_t x = 0; x < rect.width; x++) {
            auto sourceX = static_cast<int64_t>(std::floor((rect.x + x - screen.x) * scaleX));
            auto pixel = row + static_cast<size_t>(x) * 4;

            if (sourceX < 0 || sourceX >= image.width) std::memset(pixel, 0, 4);
            else std::memcpy(pixel, source + static_cast<size_t>(sourceX) * 4, 4);
        }
    }
}

}

bool captureRects(std::vector<Rect> const& rects, std::vector<uint8_t*> const& targets) {
    auto image = takeImage();
    if (!image || !image->width || !image->height) return false;

    auto screen = boundsOf(wayland::listDisplays());
    if (screen.width <= 0 || screen.height <= 0)
        screen = {0, 0, static_cast<int32_t>(image->width), static_cast<int32_t>(image->height)};

    for (size_t index = 0; index < rects.size(); index++) {
        copyScaled(*image, screen, rects[index], targets[index]);

        size_t length = static_cast<size_t>(rects[index].width) * rects[index].height * 4;
        for (size_t offset = 3; offset < length; offset += 4) targets[index][offset] = 255;
    }

    return true;
}

}

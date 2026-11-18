#include "portal.h"

#include "host.h"

#include <shared/log.h>

#include <atomic>

namespace fileship::host {

namespace {

struct Pending {
    guint subscription = 0;
    PortalResponse onResponse;
};

void finish(Pending* pending, uint32_t response, GVariant* results) {
    g_dbus_connection_signal_unsubscribe(sessionBus(), pending->subscription);
    pending->onResponse(response, results);
    delete pending;
}

void handleResponse(GDBusConnection*, gchar const*, gchar const*, gchar const*, gchar const*, GVariant* parameters,
                    gpointer data) {
    guint32 response = 0;
    GVariant* results = nullptr;
    g_variant_get(parameters, "(u@a{sv})", &response, &results);

    finish(static_cast<Pending*>(data), response, results);
    g_variant_unref(results);
}

void handleCalled(GObject*, GAsyncResult* result, gpointer data) {
    GError* error = nullptr;
    GVariant* reply = g_dbus_connection_call_finish(sessionBus(), result, &error);

    if (reply) {
        g_variant_unref(reply);
        return;
    }

    logWarn(L"A desktop portal request failed: " + toWide(error->message));
    g_error_free(error);
    finish(static_cast<Pending*>(data), 2, nullptr);
}

}

void registerPortalApp() {
    GVariantBuilder options;
    g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));

    GVariant* reply =
        g_dbus_connection_call_sync(sessionBus(), PORTAL_NAME, PORTAL_PATH, "org.freedesktop.host.portal.Registry",
                                    "Register", g_variant_new("(sa{sv})", APP_ID, &options), nullptr,
                                    G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, nullptr);
    if (reply) g_variant_unref(reply);
}

uint32_t portalVersion(char const* interface) {
    GVariant* reply = g_dbus_connection_call_sync(sessionBus(), PORTAL_NAME, PORTAL_PATH,
                                                  "org.freedesktop.DBus.Properties", "Get",
                                                  g_variant_new("(ss)", interface, "version"), G_VARIANT_TYPE("(v)"),
                                                  G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, nullptr);
    if (!reply) return 0;

    GVariant* value = nullptr;
    g_variant_get(reply, "(v)", &value);
    uint32_t version = g_variant_get_uint32(value);

    g_variant_unref(value);
    g_variant_unref(reply);
    return version;
}

std::string portalToken() {
    static std::atomic<uint32_t> next = 0;
    return "fileship" + std::to_string(++next);
}

void portalRequest(char const* interface, char const* method, PortalParameters const& parameters,
                   PortalResponse onResponse) {
    auto token = portalToken();
    std::string sender = g_dbus_connection_get_unique_name(sessionBus()) + 1;
    for (auto& character : sender) character = character == '.' ? '_' : character;

    auto path = std::string(PORTAL_PATH) + "/request/" + sender + "/" + token;
    auto pending = new Pending{0, std::move(onResponse)};

    pending->subscription = g_dbus_connection_signal_subscribe(
        sessionBus(), PORTAL_NAME, "org.freedesktop.portal.Request", "Response", path.c_str(), nullptr,
        G_DBUS_SIGNAL_FLAGS_NONE, handleResponse, pending, nullptr);

    g_dbus_connection_call(sessionBus(), PORTAL_NAME, PORTAL_PATH, interface, method, parameters(token.c_str()),
                           G_VARIANT_TYPE("(o)"), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, handleCalled, pending);
}

}

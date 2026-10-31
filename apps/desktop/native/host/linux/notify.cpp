#include "linux.h"

#include <host/app.h>
#include <shared/log.h>

#include <map>
#include <mutex>

namespace fileship::host {

namespace {

constexpr char const* SERVICE = "org.freedesktop.Notifications";
constexpr char const* PATH = "/org/freedesktop/Notifications";
constexpr size_t KEEP_NOTIFICATIONS = 20;

std::mutex mutex;
std::map<uint32_t, std::function<void()>> clicks;

void handleSignal(GDBusConnection*, gchar const*, gchar const*, gchar const*, gchar const* signal,
                  GVariant* parameters, gpointer) {
    guint32 id = 0;
    std::function<void()> click;

    if (std::string_view(signal) == "ActionInvoked") {
        gchar const* action = nullptr;
        g_variant_get(parameters, "(u&s)", &id, &action);
        if (std::string_view(action) != "default") return;
    } else {
        g_variant_get(parameters, "(uu)", &id, nullptr);
    }

    {
        std::lock_guard lock(mutex);
        auto found = clicks.find(id);
        if (found == clicks.end()) return;
        if (std::string_view(signal) == "ActionInvoked") click = found->second;
        clicks.erase(found);
    }

    if (click) runAsync(click);
}

}

void initNotifications() {
    if (!sessionBus()) return;

    for (auto signal : {"ActionInvoked", "NotificationClosed"}) {
        g_dbus_connection_signal_subscribe(sessionBus(), SERVICE, SERVICE, signal, PATH, nullptr,
                                           G_DBUS_SIGNAL_FLAGS_NONE, handleSignal, nullptr, nullptr);
    }
}

void notify(std::wstring const& title, std::wstring const& body, std::function<void()> onClick) {
    if (!sessionBus()) return;

    GVariantBuilder actions;
    g_variant_builder_init(&actions, G_VARIANT_TYPE("as"));
    if (onClick) {
        g_variant_builder_add(&actions, "s", "default");
        g_variant_builder_add(&actions, "s", "");
    }

    GVariantBuilder hints;
    g_variant_builder_init(&hints, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&hints, "{sv}", "desktop-entry", g_variant_new_string("dev.hanzy.fileship"));

    GError* error = nullptr;
    auto reply = g_dbus_connection_call_sync(
        sessionBus(), SERVICE, PATH, SERVICE, "Notify",
        g_variant_new("(susssasa{sv}i)", "Fileship", 0u, iconPath().c_str(), toUtf8(title).c_str(),
                      toUtf8(body).c_str(), &actions, &hints, -1),
        G_VARIANT_TYPE("(u)"), G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, &error);

    if (!reply) {
        logWarn(L"Could not show a notification: " + toWide(error ? error->message : "unknown error"));
        if (error) g_error_free(error);
        return;
    }

    guint32 id = 0;
    g_variant_get(reply, "(u)", &id);
    g_variant_unref(reply);

    if (!onClick) return;

    std::lock_guard lock(mutex);
    clicks[id] = std::move(onClick);
    if (clicks.size() > KEEP_NOTIFICATIONS) clicks.erase(clicks.begin());
}

}

#include "host.h"

#include <host/platform.h>
#include <shared/log.h>
#include <shared/platform.h>

#include <map>
#include <mutex>

namespace fileship::host {

namespace {

constexpr char const* SERVICE = "org.freedesktop.Notifications";
constexpr char const* PATH = "/org/freedesktop/Notifications";

using ActionHandler = std::function<void(std::string const& action)>;

std::mutex mutex;
std::map<uint32_t, ActionHandler> handlers;
std::map<std::wstring, uint32_t> progressIds;

GVariant* call(char const* method, GVariant* parameters, char const* reply) {
    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(sessionBus(), SERVICE, PATH, SERVICE, method, parameters,
                                                   G_VARIANT_TYPE(reply), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);
    if (result) return result;

    std::string message = error->message;
    g_error_free(error);
    throw std::runtime_error(message);
}

bool hasBodyMarkup() {
    static bool const value = [] {
        GVariant* reply = call("GetCapabilities", nullptr, "(as)");
        gchar const** capabilities = nullptr;
        g_variant_get(reply, "(^a&s)", &capabilities);

        bool found = g_strv_contains(capabilities, "body-markup");
        g_free(capabilities);
        g_variant_unref(reply);
        return found;
    }();

    return value;
}

std::string bodyText(std::wstring const& text) {
    auto plain = toUtf8(text);
    if (!hasBodyMarkup()) return plain;

    gchar* escaped = g_markup_escape_text(plain.c_str(), -1);
    std::string result = escaped;
    g_free(escaped);
    return result;
}

struct Notification {
    uint32_t replaces = 0;
    std::wstring title;
    std::wstring body;
    std::vector<std::pair<std::string, std::wstring>> actions;
    GVariantBuilder hints;
    int timeout = -1;

    Notification() {
        g_variant_builder_init(&hints, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&hints, "{sv}", "desktop-entry", g_variant_new_string(APP_ID));
    }

    ~Notification() {
        g_variant_builder_clear(&hints);
    }
};

uint32_t send(Notification& notification) {
    auto body = bodyText(notification.body);
    auto icon = (resourcesPath() / "icon.png").string();

    GVariantBuilder actions;
    g_variant_builder_init(&actions, G_VARIANT_TYPE("as"));

    for (auto const& [key, label] : notification.actions) {
        g_variant_builder_add(&actions, "s", key.c_str());
        g_variant_builder_add(&actions, "s", toUtf8(label).c_str());
    }

    GVariant* reply = call("Notify",
                           g_variant_new("(susssasa{sv}i)", "Fileship", notification.replaces, icon.c_str(),
                                         toUtf8(notification.title).c_str(), body.c_str(), &actions,
                                         &notification.hints, notification.timeout),
                           "(u)");

    guint32 id = 0;
    g_variant_get(reply, "(u)", &id);
    g_variant_unref(reply);
    return id;
}

void handleAction(GDBusConnection*, gchar const*, gchar const*, gchar const*, gchar const*, GVariant* parameters,
                  gpointer) {
    guint32 id = 0;
    gchar const* action = nullptr;
    g_variant_get(parameters, "(u&s)", &id, &action);

    std::lock_guard lock(mutex);
    auto found = handlers.find(id);
    if (found != handlers.end()) runAsync([handler = found->second, name = std::string(action)] { handler(name); });
}

void handleClosed(GDBusConnection*, gchar const*, gchar const*, gchar const*, gchar const*, GVariant* parameters,
                  gpointer) {
    guint32 id = 0;
    g_variant_get(parameters, "(uu)", &id, nullptr);

    std::lock_guard lock(mutex);
    handlers.erase(id);
    std::erase_if(progressIds, [id](auto const& entry) { return entry.second == id; });
}

}

void initNotifications() {
    g_dbus_connection_signal_subscribe(sessionBus(), SERVICE, SERVICE, "ActionInvoked", PATH, nullptr,
                                       G_DBUS_SIGNAL_FLAGS_NONE, handleAction, nullptr, nullptr);
    g_dbus_connection_signal_subscribe(sessionBus(), SERVICE, SERVICE, "NotificationClosed", PATH, nullptr,
                                       G_DBUS_SIGNAL_FLAGS_NONE, handleClosed, nullptr, nullptr);
}

void notify(std::wstring const& title, std::wstring const& body, std::function<void()> onClick,
            std::optional<std::filesystem::path> const& image) {
    try {
        Notification notification;
        notification.title = title;
        notification.body = body;

        if (onClick) notification.actions.push_back({"default", L""});

        if (image) {
            gchar* uri = g_filename_to_uri(image->c_str(), nullptr, nullptr);
            g_variant_builder_add(&notification.hints, "{sv}", "image-path", g_variant_new_string(uri));
            g_free(uri);
        }

        auto id = send(notification);
        if (!onClick) return;

        std::lock_guard lock(mutex);
        handlers[id] = [onClick](std::string const&) { onClick(); };
    } catch (std::exception const& error) {
        logWarn(L"Could not show a notification: " + errorMessage(error));
    }
}

bool showUploadProgress(ProgressNotice const& progress, bool isNew) {
    uint32_t shown = 0;

    {
        std::lock_guard lock(mutex);
        auto found = progressIds.find(progress.tag);
        if (found != progressIds.end()) shown = found->second;
    }

    if (!isNew && !shown) return false;

    Notification notification;
    notification.replaces = shown;
    notification.title = t(L"tasks.uploadingTitle");
    notification.body = progress.name + L" · " + progress.status + L" · " + progress.remaining;
    notification.timeout = 0;
    notification.actions.push_back({progress.isPaused ? "resume" : "pause",
                                    t(progress.isPaused ? L"tasks.uploadingResume" : L"tasks.uploadingPause")});
    notification.actions.push_back({"cancel", t(L"tasks.uploadingCancel")});

    g_variant_builder_add(&notification.hints, "{sv}", "resident", g_variant_new_boolean(TRUE));
    g_variant_builder_add(&notification.hints, "{sv}", "suppress-sound", g_variant_new_boolean(TRUE));
    g_variant_builder_add(&notification.hints, "{sv}", "category", g_variant_new_string("transfer"));
    g_variant_builder_add(&notification.hints, "{sv}", "value",
                          g_variant_new_int32(static_cast<gint32>(progress.sent * 100 / progress.total)));

    auto id = send(notification);

    std::lock_guard lock(mutex);
    progressIds[progress.tag] = id;
    handlers[id] = [tag = progress.tag](std::string const& action) {
        if (action == "pause") pauseUpload(tag);
        else if (action == "resume") resumeUpload(tag);
        else if (action == "cancel") cancelUpload(tag);
    };

    return true;
}

void hideUploadProgress(std::wstring const& tag) {
    uint32_t id = 0;

    {
        std::lock_guard lock(mutex);
        auto found = progressIds.find(tag);
        if (found == progressIds.end()) return;

        id = found->second;
        progressIds.erase(found);
        handlers.erase(id);
    }

    g_variant_unref(call("CloseNotification", g_variant_new("(u)", id), "()"));
}

}

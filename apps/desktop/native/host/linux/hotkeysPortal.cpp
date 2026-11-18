#include "host.h"
#include "portal.h"

#include <shared/log.h>

#include <xkbcommon/xkbcommon.h>

namespace fileship::host {

namespace {

constexpr char const* INTERFACE = "org.freedesktop.portal.GlobalShortcuts";

std::vector<Hotkey> declared;
std::string session;
JsonObject triggers;
uint32_t generation = 0;

bool isSame(std::vector<Hotkey> const& a, std::vector<Hotkey> const& b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](Hotkey const& first, Hotkey const& second) {
        return first.action == second.action && first.accelerator.key == second.accelerator.key &&
               first.accelerator.control == second.accelerator.control &&
               first.accelerator.alt == second.accelerator.alt &&
               first.accelerator.shift == second.accelerator.shift &&
               first.accelerator.super == second.accelerator.super;
    });
}

std::string preferredTrigger(Accelerator const& accelerator) {
    char name[64]{};
    xkb_keysym_get_name(*keysymOf(accelerator.key), name, sizeof(name));

    return std::string(accelerator.control ? "CTRL+" : "") + (accelerator.alt ? "ALT+" : "") +
           (accelerator.shift ? "SHIFT+" : "") + (accelerator.super ? "LOGO+" : "") + name;
}

void readTriggers(GVariant* shortcuts) {
    triggers = JsonObject();

    GVariantIter iterator;
    gchar const* id = nullptr;
    GVariant* properties = nullptr;
    g_variant_iter_init(&iterator, shortcuts);

    while (g_variant_iter_next(&iterator, "(&s@a{sv})", &id, &properties)) {
        gchar const* description = "";
        g_variant_lookup(properties, "trigger_description", "&s", &description);
        triggers.SetNamedValue(toWide(id), JsonValue::CreateStringValue(toWide(description)));
        g_variant_unref(properties);
    }

    broadcast(L"hotkeys:triggers", cloneObject(triggers));
}

void closeSession() {
    generation++;
    triggers = JsonObject();
    if (session.empty()) return;

    g_dbus_connection_call(sessionBus(), PORTAL_NAME, session.c_str(), "org.freedesktop.portal.Session", "Close",
                           nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr, nullptr);
    session.clear();
}

void bindShortcuts(uint32_t current) {
    portalRequest(
        INTERFACE, "BindShortcuts",
        [](char const* token) {
            GVariantBuilder shortcuts;
            g_variant_builder_init(&shortcuts, G_VARIANT_TYPE("a(sa{sv})"));

            for (auto const& hotkey : declared) {
                GVariantBuilder properties;
                g_variant_builder_init(&properties, G_VARIANT_TYPE("a{sv}"));
                g_variant_builder_add(&properties, "{sv}", "description",
                                      g_variant_new_string(toUtf8(t(L"hotkeys.actions." + hotkey.action)).c_str()));
                g_variant_builder_add(&properties, "{sv}", "preferred_trigger",
                                      g_variant_new_string(preferredTrigger(hotkey.accelerator).c_str()));
                g_variant_builder_add(&shortcuts, "(sa{sv})", toUtf8(hotkey.action).c_str(), &properties);
            }

            GVariantBuilder options;
            g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
            g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string(token));

            return g_variant_new("(oa(sa{sv})sa{sv})", session.c_str(), &shortcuts, "", &options);
        },
        [current](uint32_t response, GVariant* results) {
            if (current != generation) return;
            if (response) return logWarn(L"The system did not assign the hotkeys");

            GVariant* shortcuts = g_variant_lookup_value(results, "shortcuts", G_VARIANT_TYPE("a(sa{sv})"));
            readTriggers(shortcuts);
            g_variant_unref(shortcuts);
        });
}

void createSession() {
    portalRequest(
        INTERFACE, "CreateSession",
        [](char const* token) {
            GVariantBuilder options;
            g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
            g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string(token));
            g_variant_builder_add(&options, "{sv}", "session_handle_token",
                                  g_variant_new_string(portalToken().c_str()));

            return g_variant_new("(a{sv})", &options);
        },
        [current = generation](uint32_t response, GVariant* results) {
            if (response) return logWarn(L"The system refused a hotkey session");

            gchar const* handle = nullptr;
            g_variant_lookup(results, "session_handle", "&s", &handle);

            if (current != generation) {
                g_dbus_connection_call(sessionBus(), PORTAL_NAME, handle, "org.freedesktop.portal.Session", "Close",
                                       nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr, nullptr);
                return;
            }

            session = handle;
            bindShortcuts(current);
        });
}

void handleActivated(GDBusConnection*, gchar const*, gchar const*, gchar const*, gchar const*, GVariant* parameters,
                     gpointer) {
    gchar const* handle = nullptr;
    gchar const* id = nullptr;
    g_variant_get(parameters, "(&o&st@a{sv})", &handle, &id, nullptr, nullptr);

    if (session == handle) runAction(toWide(id));
}

void handleChanged(GDBusConnection*, gchar const*, gchar const*, gchar const*, gchar const*, GVariant* parameters,
                   gpointer) {
    gchar const* handle = nullptr;
    GVariant* shortcuts = nullptr;
    g_variant_get(parameters, "(&o@a(sa{sv}))", &handle, &shortcuts);

    if (session == handle) readTriggers(shortcuts);
    g_variant_unref(shortcuts);
}

void subscribe() {
    static bool const isSubscribed = [] {
        g_dbus_connection_signal_subscribe(sessionBus(), PORTAL_NAME, INTERFACE, "Activated", PORTAL_PATH, nullptr,
                                           G_DBUS_SIGNAL_FLAGS_NONE, handleActivated, nullptr, nullptr);
        g_dbus_connection_signal_subscribe(sessionBus(), PORTAL_NAME, INTERFACE, "ShortcutsChanged", PORTAL_PATH,
                                           nullptr, G_DBUS_SIGNAL_FLAGS_NONE, handleChanged, nullptr, nullptr);
        return true;
    }();

    (void)isSubscribed;
}

}

uint32_t portalHotkeysVersion() {
    static uint32_t const version = portalVersion(INTERFACE);
    return version;
}

std::vector<std::wstring> bindPortalHotkeys(std::vector<Hotkey> const& hotkeys) {
    if (!portalHotkeysVersion()) return std::vector<std::wstring>(hotkeys.size(), L"unsupported");

    std::vector<std::wstring> results;
    std::vector<Hotkey> known;

    for (auto const& hotkey : hotkeys) {
        bool isKnown = keysymOf(hotkey.accelerator.key).has_value();
        if (isKnown) known.push_back(hotkey);
        results.push_back(isKnown ? L"ok" : L"invalid");
    }

    if (!isSame(declared, known)) {
        subscribe();
        closeSession();
        declared = known;
        if (!declared.empty()) createSession();
    }

    return results;
}

JsonObject portalHotkeyTriggers() {
    return runOnMainThread<JsonObject>([] { return cloneObject(triggers); });
}

void configurePortalHotkeys() {
    postToMainThread([] {
        if (session.empty()) return;

        GVariantBuilder options;
        g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
        g_dbus_connection_call(sessionBus(), PORTAL_NAME, PORTAL_PATH, INTERFACE, "ConfigureShortcuts",
                               g_variant_new("(osa{sv})", session.c_str(), "", &options), nullptr,
                               G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr, nullptr);
    });
}

}

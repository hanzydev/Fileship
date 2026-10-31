#include "eventThread.h"
#include "linux.h"

#include <core/linux/linux.h>
#include <core/linux/x11.h>
#include <host/app.h>
#include <host/hotkeys.h>
#include <shared/i18n.h>
#include <shared/log.h>

#include <X11/XF86keysym.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <set>

namespace fileship::host {

namespace {

constexpr char const* PORTAL = "org.freedesktop.portal.Desktop";
constexpr char const* PORTAL_PATH = "/org/freedesktop/portal/desktop";
constexpr char const* SHORTCUTS = "org.freedesktop.portal.GlobalShortcuts";
constexpr char const* REQUEST = "org.freedesktop.portal.Request";
constexpr auto BIND_WAIT = std::chrono::seconds(5);
constexpr Time REPEAT_MS = 1500;

using Statuses = std::map<std::wstring, std::wstring>;
using Bound = std::vector<std::pair<std::wstring, std::wstring>>;

std::mutex mutex;
std::condition_variable bindings;
JsonObject status;

void setStatus(Statuses const& values) {
    JsonObject object;
    for (auto const& [action, value] : values) object.SetNamedValue(action, JsonValue::CreateStringValue(value));

    std::lock_guard lock(mutex);
    status = object;
}

std::optional<uint32_t> keySymbol(std::wstring const& name) {
    static std::map<std::wstring, uint32_t> const keys{
        {L"plus", XK_plus},
        {L"space", XK_space},
        {L"tab", XK_Tab},
        {L"capslock", XK_Caps_Lock},
        {L"numlock", XK_Num_Lock},
        {L"scrolllock", XK_Scroll_Lock},
        {L"backspace", XK_BackSpace},
        {L"delete", XK_Delete},
        {L"insert", XK_Insert},
        {L"return", XK_Return},
        {L"enter", XK_Return},
        {L"up", XK_Up},
        {L"down", XK_Down},
        {L"left", XK_Left},
        {L"right", XK_Right},
        {L"home", XK_Home},
        {L"end", XK_End},
        {L"pageup", XK_Page_Up},
        {L"pagedown", XK_Page_Down},
        {L"escape", XK_Escape},
        {L"esc", XK_Escape},
        {L"volumeup", XF86XK_AudioRaiseVolume},
        {L"volumedown", XF86XK_AudioLowerVolume},
        {L"volumemute", XF86XK_AudioMute},
        {L"medianexttrack", XF86XK_AudioNext},
        {L"mediaprevioustrack", XF86XK_AudioPrev},
        {L"mediastop", XF86XK_AudioStop},
        {L"mediaplaypause", XF86XK_AudioPlay},
        {L"printscreen", XK_Print},
        {L"numdec", XK_KP_Decimal},
        {L"numadd", XK_KP_Add},
        {L"numsub", XK_KP_Subtract},
        {L"nummult", XK_KP_Multiply},
        {L"numdiv", XK_KP_Divide},
    };

    auto key = lowerKeyName(name);
    if (auto found = keys.find(key); found != keys.end()) return found->second;

    if (key.size() == 1) {
        auto character = static_cast<uint32_t>(key[0]);
        if (character <= 0x20 || character == 0x7f || (character >= 0x80 && character < 0xa0)) return std::nullopt;
        return character < 0x100 ? character : 0x01000000 | character;
    }

    if (key.size() >= 2 && key[0] == L'f') {
        auto number = std::wcstol(key.c_str() + 1, nullptr, 10);
        if (number >= 1 && number <= 24 && std::to_wstring(number) == key.substr(1))
            return static_cast<uint32_t>(XK_F1 + number - 1);
    }

    if (key.size() == 4 && key.rfind(L"num", 0) == 0 && iswdigit(key[3])) return XK_KP_0 + (key[3] - L'0');

    return std::nullopt;
}

class X11Hotkeys {
public:
    Statuses grab(Bound const& bound) {
        if (!start()) return {};

        return thread.run<Statuses>([this, bound] {
            lastBound = bound;
            return grabAll();
        });
    }

    void release() {
        if (!start()) return;

        thread.run<bool>([this] {
            lastBound.clear();
            ungrabAll();
            return true;
        });
    }

private:
    std::once_flag started;
    Display* display = nullptr;
    EventThread thread;
    Bound lastBound;
    std::map<std::pair<unsigned int, unsigned int>, std::wstring> grabs;
    std::map<unsigned int, Time> presses;
    unsigned int numLock = 0;
    unsigned int alt = Mod1Mask;
    unsigned int super = Mod4Mask;

    bool start() {
        std::call_once(started, [this] {
            display = core::x11::openDisplay();
            if (!display) return;

            XkbSetDetectableAutoRepeat(display, True, nullptr);
            thread.start({
                .fd = ConnectionNumber(display),
                .prepare =
                    [this] {
                        while (XPending(display)) {
                            XEvent event{};
                            XNextEvent(display, &event);
                            handleEvent(event);
                        }
                    },
            });
        });

        return display != nullptr;
    }

    void handleEvent(XEvent& event) {
        if (event.type == MappingNotify) {
            XRefreshKeyboardMapping(&event.xmapping);
            if (!lastBound.empty()) setStatus(grabAll());
            return;
        }

        if (event.type == KeyRelease) {
            if (!isAutoRepeat(event)) presses.erase(event.xkey.keycode);
            return;
        }

        if (event.type != KeyPress) return;

        auto const& key = event.xkey;
        auto last = presses.find(key.keycode);
        bool isRepeat = last != presses.end() && key.time - last->second < REPEAT_MS;
        presses[key.keycode] = key.time;
        if (isRepeat) return;

        auto modifiers = key.state & (ShiftMask | ControlMask | alt | super);
        if (auto found = grabs.find({key.keycode, modifiers}); found != grabs.end()) runAction(found->second);
    }

    bool isAutoRepeat(XEvent const& release) {
        if (!XEventsQueued(display, QueuedAfterReading)) return false;

        XEvent next{};
        XPeekEvent(display, &next);
        return next.type == KeyPress && next.xkey.keycode == release.xkey.keycode && next.xkey.time == release.xkey.time;
    }

    void readModifiers() {
        numLock = 0;
        alt = Mod1Mask;
        super = Mod4Mask;

        auto map = XGetModifierMapping(display);
        if (!map) return;

        for (int modifier = 3; modifier < 8; modifier++) {
            for (int index = 0; index < map->max_keypermod; index++) {
                auto keycode = map->modifiermap[modifier * map->max_keypermod + index];
                if (!keycode) continue;

                auto symbol = XkbKeycodeToKeysym(display, keycode, 0, 0);
                if (symbol == XK_Num_Lock) numLock = 1u << modifier;
                else if (symbol == XK_Alt_L || symbol == XK_Alt_R || symbol == XK_Meta_L) alt = 1u << modifier;
                else if (symbol == XK_Super_L || symbol == XK_Super_R) super = 1u << modifier;
            }
        }

        XFreeModifiermap(map);
    }

    std::vector<unsigned int> lockMasks() const {
        std::vector<unsigned int> masks{0, LockMask};
        if (numLock) {
            masks.push_back(numLock);
            masks.push_back(numLock | LockMask);
        }
        return masks;
    }

    unsigned int maskOf(uint32_t modifiers) const {
        return (modifiers & HotkeyControl ? ControlMask : 0) | (modifiers & HotkeyAlt ? alt : 0) |
               (modifiers & HotkeyShift ? ShiftMask : 0) | (modifiers & HotkeySuper ? super : 0);
    }

    void ungrabAll() {
        auto root = DefaultRootWindow(display);
        for (auto const& [key, action] : grabs) {
            for (auto extra : lockMasks()) XUngrabKey(display, static_cast<int>(key.first), key.second | extra, root);
        }

        grabs.clear();
        core::x11::syncErrors(display);
    }

    Statuses grabAll() {
        ungrabAll();
        readModifiers();

        auto root = DefaultRootWindow(display);
        Statuses result;

        for (auto const& [action, accelerator] : lastBound) {
            auto parsed = parseAccelerator(accelerator, keySymbol);
            auto keycode = parsed ? XKeysymToKeycode(display, parsed->key) : 0;
            if (!keycode) {
                result[action] = L"invalid";
                continue;
            }

            auto modifiers = maskOf(parsed->modifiers);
            if (grabs.contains({keycode, modifiers})) {
                result[action] = L"conflict";
                continue;
            }

            for (auto extra : lockMasks())
                XGrabKey(display, keycode, modifiers | extra, root, False, GrabModeAsync, GrabModeAsync);

            if (core::x11::syncErrors(display)) {
                for (auto extra : lockMasks()) XUngrabKey(display, keycode, modifiers | extra, root);
                core::x11::syncErrors(display);
                result[action] = L"conflict";
                continue;
            }

            grabs[{keycode, modifiers}] = action;
            result[action] = L"ok";
        }

        return result;
    }
};

X11Hotkeys& x11Hotkeys() {
    static auto instance = new X11Hotkeys();
    return *instance;
}

std::string requestPath(std::string const& token) {
    std::string sender = g_dbus_connection_get_unique_name(sessionBus()) + 1;
    std::replace(sender.begin(), sender.end(), '.', '_');
    return std::string(PORTAL_PATH) + "/request/" + sender + "/" + token;
}

std::string triggerOf(std::wstring const& accelerator) {
    auto parsed = parseAccelerator(accelerator, keySymbol);
    if (!parsed) return {};

    auto name = XKeysymToString(parsed->key);
    if (!name) return {};

    std::string trigger;
    if (parsed->modifiers & HotkeyControl) trigger += "CTRL+";
    if (parsed->modifiers & HotkeyAlt) trigger += "ALT+";
    if (parsed->modifiers & HotkeyShift) trigger += "SHIFT+";
    if (parsed->modifiers & HotkeySuper) trigger += "LOGO+";
    return trigger + name;
}

class PortalHotkeys {
public:
    bool isAvailable() {
        std::call_once(checked, [this] {
            auto reply = g_dbus_connection_call_sync(
                sessionBus(), PORTAL, PORTAL_PATH, "org.freedesktop.DBus.Properties", "Get",
                g_variant_new("(ss)", SHORTCUTS, "version"), G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, 5000,
                nullptr, nullptr);
            hasPortal = reply != nullptr;
            if (reply) g_variant_unref(reply);
        });

        return hasPortal;
    }

    void bind(Bound const& bound) {
        std::vector<std::pair<std::wstring, std::string>> desired;
        for (auto const& [action, accelerator] : bound) desired.emplace_back(action, triggerOf(accelerator));

        uint64_t current = 0;

        {
            std::lock_guard lock(mutex);
            isPaused = false;
            if (desired == requested && (isBinding || isHandled)) return;

            requested = desired;
            isBinding = !desired.empty();
            isHandled = false;
            current = ++generation;
            pendingStatus(desired);
        }

        postToMainThread([this, current, desired] { start(current, desired); });
    }

    void waitForBinding() {
        std::unique_lock lock(mutex);
        bindings.wait_for(lock, BIND_WAIT, [this] { return !isBinding; });
    }

    void pause() {
        std::lock_guard lock(mutex);
        isPaused = true;
    }

    void release() {
        std::lock_guard lock(mutex);
        requested.clear();
        isBinding = false;
        isHandled = false;
        generation++;
        postToMainThread([this] { closeSession(); });
    }

private:
    std::once_flag checked;
    bool hasPortal = false;
    std::vector<std::pair<std::wstring, std::string>> requested;
    std::string session;
    uint64_t generation = 0;
    bool isBinding = false;
    bool isHandled = false;
    bool isPaused = false;
    guint activation = 0;
    uint32_t tokens = 0;

    struct Pending {
        PortalHotkeys* owner;
        uint64_t generation;
        std::vector<std::pair<std::wstring, std::string>> desired;
        void (*onResponse)(Pending*, uint32_t, GVariant*) = nullptr;
        guint subscription = 0;
    };

    void pendingStatus(std::vector<std::pair<std::wstring, std::string>> const& desired) {
        JsonObject object;
        for (auto const& [action, trigger] : desired) {
            if (trigger.empty()) object.SetNamedValue(action, JsonValue::CreateStringValue(L"invalid"));
        }
        status = object;
    }

    static void closeSession(std::string const& handle) {
        if (handle.empty()) return;

        g_dbus_connection_call(sessionBus(), PORTAL, handle.c_str(), "org.freedesktop.portal.Session", "Close",
                               nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr, nullptr);
    }

    void closeSession() {
        std::string handle;

        {
            std::lock_guard lock(mutex);
            handle.swap(session);
        }

        closeSession(handle);
    }

    bool isCurrent(uint64_t value) {
        std::lock_guard lock(mutex);
        return generation == value;
    }

    void start(uint64_t current, std::vector<std::pair<std::wstring, std::string>> const& desired) {
        if (!isCurrent(current)) return;

        closeSession();
        subscribeActivation();

        if (desired.empty()) {
            finish(current, {}, {});
            return;
        }

        auto token = "fileship_hotkeys_" + std::to_string(++tokens);
        GVariantBuilder options;
        g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);
        g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string(token.c_str()));
        g_variant_builder_add(&options, "{sv}", "session_handle_token", g_variant_new_string(token.c_str()));

        request(new Pending{this, current, desired, &PortalHotkeys::handleCreated}, token, "CreateSession",
                g_variant_new("(a{sv})", &options));
    }

    void request(Pending* pending, std::string const& token, char const* method, GVariant* parameters) {
        pending->subscription = g_dbus_connection_signal_subscribe(
            sessionBus(), PORTAL, REQUEST, "Response", requestPath(token).c_str(), nullptr, G_DBUS_SIGNAL_FLAGS_NONE,
            [](GDBusConnection* bus, gchar const*, gchar const*, gchar const*, gchar const*, GVariant* parameters,
               gpointer data) {
                auto pending = static_cast<Pending*>(data);
                g_dbus_connection_signal_unsubscribe(bus, pending->subscription);

                guint32 code = 2;
                GVariant* results = nullptr;
                g_variant_get(parameters, "(u@a{sv})", &code, &results);
                pending->onResponse(pending, code, results);
                g_variant_unref(results);
            },
            pending, nullptr);

        g_dbus_connection_call(
            sessionBus(), PORTAL, PORTAL_PATH, SHORTCUTS, method, parameters, G_VARIANT_TYPE("(o)"),
            G_DBUS_CALL_FLAGS_NONE, -1, nullptr,
            [](GObject* source, GAsyncResult* result, gpointer data) {
                auto pending = static_cast<Pending*>(data);
                GError* error = nullptr;
                auto reply = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source), result, &error);
                if (reply) {
                    g_variant_unref(reply);
                    return;
                }

                logWarn(L"Could not bind the global shortcuts: " + toWide(error ? error->message : "unknown error"));
                if (error) g_error_free(error);
                g_dbus_connection_signal_unsubscribe(G_DBUS_CONNECTION(source), pending->subscription);
                pending->owner->finish(pending->generation, pending->desired, {});
                delete pending;
            },
            pending);
    }

    static void handleCreated(Pending* pending, uint32_t code, GVariant* results) {
        std::unique_ptr<Pending> owned(pending);
        auto& self = *pending->owner;

        std::string handle;
        if (auto value = g_variant_lookup_value(results, "session_handle", nullptr)) {
            if (g_variant_is_of_type(value, G_VARIANT_TYPE_STRING) ||
                g_variant_is_of_type(value, G_VARIANT_TYPE_OBJECT_PATH))
                handle = g_variant_get_string(value, nullptr);
            g_variant_unref(value);
        }

        if (!self.isCurrent(pending->generation)) {
            closeSession(handle);
            return;
        }

        if (code != 0 || handle.empty()) {
            logWarn(L"The global shortcuts session was not created");
            self.finish(pending->generation, pending->desired, {});
            return;
        }

        {
            std::lock_guard lock(mutex);
            self.session = handle;
        }

        GVariantBuilder shortcuts;
        g_variant_builder_init(&shortcuts, G_VARIANT_TYPE("a(sa{sv})"));
        for (auto const& [action, trigger] : pending->desired) {
            if (trigger.empty()) continue;

            GVariantBuilder properties;
            g_variant_builder_init(&properties, G_VARIANT_TYPE_VARDICT);
            g_variant_builder_add(&properties, "{sv}", "description",
                                  g_variant_new_string(toUtf8(t(L"hotkeys.actions." + action)).c_str()));
            g_variant_builder_add(&properties, "{sv}", "preferred_trigger", g_variant_new_string(trigger.c_str()));
            g_variant_builder_add(&shortcuts, "(sa{sv})", toUtf8(action).c_str(), &properties);
        }

        auto token = "fileship_hotkeys_" + std::to_string(++self.tokens);
        GVariantBuilder options;
        g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);
        g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string(token.c_str()));

        self.request(new Pending{&self, pending->generation, pending->desired, &PortalHotkeys::handleBound}, token,
                     "BindShortcuts", g_variant_new("(oa(sa{sv})sa{sv})", handle.c_str(), &shortcuts, "", &options));
    }

    static void handleBound(Pending* pending, uint32_t code, GVariant* results) {
        std::unique_ptr<Pending> owned(pending);
        std::set<std::wstring> bound;

        GVariantIter* shortcuts = nullptr;
        if (code == 0 && g_variant_lookup(results, "shortcuts", "a(sa{sv})", &shortcuts)) {
            gchar const* id = nullptr;
            GVariant* properties = nullptr;
            while (g_variant_iter_loop(shortcuts, "(&s@a{sv})", &id, &properties)) bound.insert(toWide(id));
            g_variant_iter_free(shortcuts);
        }

        if (code != 0) logWarn(L"The global shortcuts were not bound");
        pending->owner->finish(pending->generation, pending->desired, bound);
    }

    void finish(uint64_t current, std::vector<std::pair<std::wstring, std::string>> const& desired,
                std::set<std::wstring> const& bound) {
        {
            std::lock_guard lock(mutex);
            if (generation != current) return;

            JsonObject object;
            for (auto const& [action, trigger] : desired) {
                auto value = trigger.empty() ? L"invalid" : bound.contains(action) ? L"ok" : L"command";
                object.SetNamedValue(action, JsonValue::CreateStringValue(value));
            }

            status = object;
            isBinding = false;
            isHandled = true;
        }

        bindings.notify_all();
    }

    void subscribeActivation() {
        if (activation) return;

        activation = g_dbus_connection_signal_subscribe(
            sessionBus(), PORTAL, SHORTCUTS, "Activated", PORTAL_PATH, nullptr, G_DBUS_SIGNAL_FLAGS_NONE,
            [](GDBusConnection*, gchar const*, gchar const*, gchar const*, gchar const*, GVariant* parameters,
               gpointer data) {
                auto& self = *static_cast<PortalHotkeys*>(data);
                gchar const* handle = nullptr;
                gchar const* id = nullptr;
                g_variant_get(parameters, "(&o&st@a{sv})", &handle, &id, nullptr, nullptr);

                {
                    std::lock_guard lock(mutex);
                    if (self.isPaused || self.session != handle) return;
                }

                runAction(toWide(id));
            },
            this, nullptr);
    }
};

PortalHotkeys& portalHotkeys() {
    static auto instance = new PortalHotkeys();
    return *instance;
}

}

JsonObject registerHotkeys() {
    auto bound = boundHotkeys();

    if (!core::isWaylandSession()) {
        setStatus(x11Hotkeys().grab(bound));
        return hotkeyStatus();
    }

    if (!portalHotkeys().isAvailable()) {
        Statuses commands;
        for (auto const& [action, accelerator] : bound) commands[action] = L"command";
        setStatus(commands);
        return hotkeyStatus();
    }

    portalHotkeys().bind(bound);
    if (!isMainThread()) portalHotkeys().waitForBinding();
    return hotkeyStatus();
}

JsonObject hotkeyStatus() {
    std::lock_guard lock(mutex);
    return cloneObject(status ? status : JsonObject());
}

void pauseHotkeys() {
    if (!core::isWaylandSession()) x11Hotkeys().release();
    else if (portalHotkeys().isAvailable()) portalHotkeys().pause();
}

void unregisterHotkeys() {
    if (!core::isWaylandSession()) x11Hotkeys().release();
    else if (portalHotkeys().isAvailable()) portalHotkeys().release();
}

bool isSnippingToolKeyEnabled() {
    return false;
}

JsonObject setSnippingToolKey(bool) {
    return registerHotkeys();
}

}

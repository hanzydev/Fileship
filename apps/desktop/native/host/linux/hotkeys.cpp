#include "host.h"

#include <shared/log.h>

#include <xcb/xcb.h>
#include <xkbcommon/xkbcommon.h>

#include <algorithm>
#include <cwctype>
#include <map>
#include <mutex>
#include <thread>

namespace fileship::host {

namespace {

constexpr uint16_t LOCKS[] = {0, XCB_MOD_MASK_LOCK, XCB_MOD_MASK_2, XCB_MOD_MASK_LOCK | XCB_MOD_MASK_2};
constexpr uint16_t MODIFIERS = XCB_MOD_MASK_CONTROL | XCB_MOD_MASK_SHIFT | XCB_MOD_MASK_1 | XCB_MOD_MASK_4;

struct Grab {
    xcb_keycode_t keycode = 0;
    uint16_t modifiers = 0;
    std::wstring action;
};

xcb_connection_t* connection = nullptr;
xcb_window_t root = 0;
std::mutex mutex;
std::vector<Grab> grabs;

std::wstring actionOf(xcb_key_press_event_t const& press) {
    std::lock_guard lock(mutex);

    for (auto const& grab : grabs) {
        if (grab.keycode == press.detail && grab.modifiers == (press.state & MODIFIERS)) return grab.action;
    }

    return {};
}

void run() {
    xcb_keycode_t releasedKey = 0;
    xcb_timestamp_t releasedAt = 0;

    while (auto event = xcb_wait_for_event(connection)) {
        auto key = reinterpret_cast<xcb_key_press_event_t*>(event);
        auto type = event->response_type & 0x7f;

        if (type == XCB_KEY_RELEASE) {
            releasedKey = key->detail;
            releasedAt = key->time;
        } else if (type == XCB_KEY_PRESS && (key->detail != releasedKey || key->time != releasedAt)) {
            auto action = actionOf(*key);
            if (!action.empty()) runAction(action);
        }

        free(event);
    }
}

bool connect() {
    static bool const isConnected = [] {
        connection = xcb_connect(nullptr, nullptr);
        if (xcb_connection_has_error(connection)) return false;

        root = xcb_setup_roots_iterator(xcb_get_setup(connection)).data->root;
        std::thread(run).detach();
        return true;
    }();

    return isConnected;
}

std::optional<xcb_keycode_t> keycodeOf(xcb_get_keyboard_mapping_reply_t* mapping, xcb_keycode_t first, uint32_t keysym) {
    auto keysyms = xcb_get_keyboard_mapping_keysyms(mapping);
    int count = xcb_get_keyboard_mapping_keysyms_length(mapping) / mapping->keysyms_per_keycode;

    for (int column = 0; column < mapping->keysyms_per_keycode; column++) {
        for (int index = 0; index < count; index++) {
            if (keysyms[index * mapping->keysyms_per_keycode + column] == keysym) {
                return static_cast<xcb_keycode_t>(first + index);
            }
        }
    }

    return std::nullopt;
}

bool grab(xcb_keycode_t keycode, uint16_t modifiers) {
    bool isGrabbed = true;

    for (auto lock : LOCKS) {
        auto request = xcb_grab_key_checked(connection, 1, root, modifiers | lock, keycode, XCB_GRAB_MODE_ASYNC,
                                            XCB_GRAB_MODE_ASYNC);

        if (auto error = xcb_request_check(connection, request)) {
            isGrabbed = false;
            free(error);
        }
    }

    if (!isGrabbed) {
        for (auto lock : LOCKS) xcb_ungrab_key(connection, keycode, root, modifiers | lock);
    }

    return isGrabbed;
}

}

bool isWayland() {
    return g_getenv("WAYLAND_DISPLAY") != nullptr;
}

std::optional<uint32_t> keysymOf(std::wstring const& key) {
    static std::map<std::wstring, char const*> const names{
        {L"plus", "plus"},
        {L"space", "space"},
        {L"tab", "Tab"},
        {L"capslock", "Caps_Lock"},
        {L"numlock", "Num_Lock"},
        {L"scrolllock", "Scroll_Lock"},
        {L"backspace", "BackSpace"},
        {L"delete", "Delete"},
        {L"insert", "Insert"},
        {L"return", "Return"},
        {L"enter", "Return"},
        {L"up", "Up"},
        {L"down", "Down"},
        {L"left", "Left"},
        {L"right", "Right"},
        {L"home", "Home"},
        {L"end", "End"},
        {L"pageup", "Page_Up"},
        {L"pagedown", "Page_Down"},
        {L"escape", "Escape"},
        {L"esc", "Escape"},
        {L"volumeup", "XF86AudioRaiseVolume"},
        {L"volumedown", "XF86AudioLowerVolume"},
        {L"volumemute", "XF86AudioMute"},
        {L"medianexttrack", "XF86AudioNext"},
        {L"mediaprevioustrack", "XF86AudioPrev"},
        {L"mediastop", "XF86AudioStop"},
        {L"mediaplaypause", "XF86AudioPlay"},
        {L"printscreen", "Print"},
        {L"numdec", "KP_Decimal"},
        {L"numadd", "KP_Add"},
        {L"numsub", "KP_Subtract"},
        {L"nummult", "KP_Multiply"},
        {L"numdiv", "KP_Divide"},
    };

    std::wstring name;
    for (auto character : key) name += static_cast<wchar_t>(std::towlower(character));

    std::string symbol;

    if (auto found = names.find(name); found != names.end()) {
        symbol = found->second;
    } else if (name.size() == 1) {
        auto keysym = xkb_utf32_to_keysym(static_cast<uint32_t>(name[0]));
        return keysym == XKB_KEY_NoSymbol ? std::nullopt : std::optional<uint32_t>(keysym);
    } else if (name.size() == 4 && name.rfind(L"num", 0) == 0 && std::iswdigit(name[3])) {
        symbol = "KP_" + toUtf8(name.substr(3));
    } else if (name.size() > 1 && name[0] == L'f' && std::all_of(name.begin() + 1, name.end(), std::iswdigit)) {
        symbol = "F" + toUtf8(name.substr(1));
    }

    auto keysym = xkb_keysym_from_name(symbol.c_str(), XKB_KEYSYM_NO_FLAGS);
    return keysym == XKB_KEY_NoSymbol ? std::nullopt : std::optional<uint32_t>(keysym);
}

std::vector<std::wstring> bindX11Hotkeys(std::vector<Hotkey> const& hotkeys) {
    std::vector<std::wstring> results;
    if (!connect()) return std::vector<std::wstring>(hotkeys.size(), L"conflict");

    std::lock_guard lock(mutex);

    xcb_ungrab_key(connection, XCB_GRAB_ANY, root, XCB_MOD_MASK_ANY);
    grabs.clear();

    auto setup = xcb_get_setup(connection);
    auto mapping = xcb_get_keyboard_mapping_reply(
        connection,
        xcb_get_keyboard_mapping(connection, setup->min_keycode, setup->max_keycode - setup->min_keycode + 1), nullptr);

    for (auto const& hotkey : hotkeys) {
        auto keysym = keysymOf(hotkey.accelerator.key);
        auto keycode = keysym ? keycodeOf(mapping, setup->min_keycode, *keysym) : std::nullopt;
        bool isReserved = hotkey.accelerator.control && hotkey.accelerator.alt && keysym >= XKB_KEY_F1 &&
                          keysym <= XKB_KEY_F12;

        if (!keycode || isReserved) {
            results.push_back(L"invalid");
            continue;
        }

        uint16_t modifiers = (hotkey.accelerator.control ? XCB_MOD_MASK_CONTROL : 0) |
                             (hotkey.accelerator.alt ? XCB_MOD_MASK_1 : 0) |
                             (hotkey.accelerator.shift ? XCB_MOD_MASK_SHIFT : 0) |
                             (hotkey.accelerator.super ? XCB_MOD_MASK_4 : 0);

        bool isGrabbed = grab(*keycode, modifiers);
        if (isGrabbed) grabs.push_back({*keycode, modifiers, hotkey.action});
        results.push_back(isGrabbed ? L"ok" : L"conflict");
    }

    free(mapping);
    xcb_flush(connection);
    return results;
}

std::vector<std::wstring> bindHotkeys(std::vector<Hotkey> const& hotkeys) {
    return isWayland() ? bindPortalHotkeys(hotkeys) : bindX11Hotkeys(hotkeys);
}

}

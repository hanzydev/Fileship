#include "win32.h"

#include <host/app.h>
#include <host/hotkeys.h>

#include <shared/log.h>

#include <mutex>

namespace fileship::host {

namespace {

constexpr wchar_t const* KEYBOARD_KEY = L"Control Panel\\Keyboard";
constexpr wchar_t const* SNIPPING_VALUE = L"PrintScreenKeyForSnippingEnabled";

std::mutex mutex;
JsonObject status;
std::vector<int> registered;

std::optional<UINT> keyCode(std::wstring const& name) {
    static std::map<std::wstring, UINT> const keys{
        {L"plus", VK_OEM_PLUS},
        {L"space", VK_SPACE},
        {L"tab", VK_TAB},
        {L"capslock", VK_CAPITAL},
        {L"numlock", VK_NUMLOCK},
        {L"scrolllock", VK_SCROLL},
        {L"backspace", VK_BACK},
        {L"delete", VK_DELETE},
        {L"insert", VK_INSERT},
        {L"return", VK_RETURN},
        {L"enter", VK_RETURN},
        {L"up", VK_UP},
        {L"down", VK_DOWN},
        {L"left", VK_LEFT},
        {L"right", VK_RIGHT},
        {L"home", VK_HOME},
        {L"end", VK_END},
        {L"pageup", VK_PRIOR},
        {L"pagedown", VK_NEXT},
        {L"escape", VK_ESCAPE},
        {L"esc", VK_ESCAPE},
        {L"volumeup", VK_VOLUME_UP},
        {L"volumedown", VK_VOLUME_DOWN},
        {L"volumemute", VK_VOLUME_MUTE},
        {L"medianexttrack", VK_MEDIA_NEXT_TRACK},
        {L"mediaprevioustrack", VK_MEDIA_PREV_TRACK},
        {L"mediastop", VK_MEDIA_STOP},
        {L"mediaplaypause", VK_MEDIA_PLAY_PAUSE},
        {L"printscreen", VK_SNAPSHOT},
        {L"numdec", VK_DECIMAL},
        {L"numadd", VK_ADD},
        {L"numsub", VK_SUBTRACT},
        {L"nummult", VK_MULTIPLY},
        {L"numdiv", VK_DIVIDE},
    };

    auto key = lowerKeyName(name);
    if (auto found = keys.find(key); found != keys.end()) return found->second;

    if (key.size() == 1) {
        wchar_t character = key[0];
        if (character >= L'a' && character <= L'z') return static_cast<UINT>(towupper(character));
        if (character >= L'0' && character <= L'9') return static_cast<UINT>(character);

        SHORT scan = VkKeyScanW(name[0]);
        if (scan != -1) return static_cast<UINT>(LOBYTE(scan));
        return std::nullopt;
    }

    if (key.size() >= 2 && key[0] == L'f') {
        int number = _wtoi(key.c_str() + 1);
        if (number >= 1 && number <= 24 && std::to_wstring(number) == key.substr(1)) return VK_F1 + number - 1;
    }

    if (key.size() == 4 && key.rfind(L"num", 0) == 0 && iswdigit(key[3])) return VK_NUMPAD0 + (key[3] - L'0');

    return std::nullopt;
}

UINT windowsModifiers(uint32_t modifiers) {
    return (modifiers & HotkeyControl ? MOD_CONTROL : 0) | (modifiers & HotkeyAlt ? MOD_ALT : 0) |
           (modifiers & HotkeyShift ? MOD_SHIFT : 0) | (modifiers & HotkeySuper ? MOD_WIN : 0);
}

void unregisterAll() {
    for (int id : registered) UnregisterHotKey(mainWindow(), id);
    registered.clear();
}

}

JsonObject registerHotkeys() {
    return runOnMainThread<JsonObject>([] {
        std::lock_guard lock(mutex);
        unregisterAll();
        status = JsonObject();

        auto current = settings();
        if (!jsonBool(current, L"hotkeysEnabled", true)) return cloneObject(status);

        auto hotkeys = jsonObject(current, L"hotkeys");
        auto const& actions = hotkeyActions();

        for (int index = 0; index < static_cast<int>(actions.size()); index++) {
            auto accelerator = jsonString(hotkeys, actions[index].c_str());
            if (accelerator.empty()) continue;

            auto parsed = parseAccelerator(accelerator, keyCode);
            if (!parsed) {
                status.SetNamedValue(actions[index], JsonValue::CreateStringValue(L"invalid"));
                continue;
            }

            int id = index + 1;
            bool isRegistered = RegisterHotKey(mainWindow(), id, windowsModifiers(parsed->modifiers) | MOD_NOREPEAT,
                                               parsed->key) != FALSE;
            if (isRegistered) registered.push_back(id);
            status.SetNamedValue(actions[index], JsonValue::CreateStringValue(isRegistered ? L"ok" : L"conflict"));
        }

        return cloneObject(status);
    });
}

JsonObject hotkeyStatus() {
    std::lock_guard lock(mutex);
    return cloneObject(status ? status : JsonObject());
}

void pauseHotkeys() {
    runOnMainThread<bool>([] {
        std::lock_guard lock(mutex);
        unregisterAll();
        return true;
    });
}

void unregisterHotkeys() {
    std::lock_guard lock(mutex);
    unregisterAll();
}

void handleHotkey(int id) {
    auto const& actions = hotkeyActions();
    if (id >= 1 && id <= static_cast<int>(actions.size())) runAction(actions[id - 1]);
}

bool isSnippingToolKeyEnabled() {
    DWORD value = 0;
    DWORD size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, KEYBOARD_KEY, SNIPPING_VALUE, RRF_RT_REG_DWORD, nullptr, &value, &size) !=
        ERROR_SUCCESS)
        return true;

    return value != 0;
}

JsonObject setSnippingToolKey(bool enabled) {
    DWORD value = enabled ? 1 : 0;
    auto result = RegSetKeyValueW(HKEY_CURRENT_USER, KEYBOARD_KEY, SNIPPING_VALUE, REG_DWORD, &value, sizeof(value));
    if (result != ERROR_SUCCESS) throw std::runtime_error("Could not change the Print Screen key setting");

    return registerHotkeys();
}

}

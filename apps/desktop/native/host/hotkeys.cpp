#include "app.h"
#include "platform.h"

#include <cwctype>
#include <mutex>

namespace fileship::host {

namespace {

constexpr wchar_t const* ACTIONS[] = {
    L"captureMonitor", L"captureRegion",     L"captureWindow",   L"captureText",  L"recordVideo",      L"recordGif",
    L"captureFullscreen", L"captureLastRegion", L"uploadClipboard", L"uploadFile", L"shortenClipboard", L"openMain",
};

std::mutex mutex;
JsonObject status;

std::wstring lower(std::wstring text) {
    for (auto& character : text) character = static_cast<wchar_t>(std::towlower(character));
    return text;
}

std::optional<Accelerator> parseAccelerator(std::wstring const& text) {
    Accelerator result;
    size_t start = 0;

    while (start <= text.size()) {
        auto plus = text.find(L'+', start);
        auto token = text.substr(start, plus == std::wstring::npos ? std::wstring::npos : plus - start);

        if (token.empty() && plus != std::wstring::npos) {
            token = L"Plus";
            plus++;
        }

        auto name = lower(token);

        if (name == L"command" || name == L"cmd" || name == L"commandorcontrol" || name == L"cmdorctrl" ||
            name == L"control" || name == L"ctrl") {
            result.control = true;
        } else if (name == L"alt" || name == L"option") {
            result.alt = true;
        } else if (name == L"altgr") {
            result.control = true;
            result.alt = true;
        } else if (name == L"shift") {
            result.shift = true;
        } else if (name == L"super" || name == L"meta") {
            result.super = true;
        } else {
            if (token.empty() || !result.key.empty()) return std::nullopt;
            result.key = token;
        }

        if (plus == std::wstring::npos) break;
        start = plus + 1;
    }

    if (result.key.empty()) return std::nullopt;
    return result;
}

void unbind() {
    bindHotkeys({});
}

}

JsonObject registerHotkeys() {
    return runOnMainThread<JsonObject>([] {
        std::lock_guard lock(mutex);
        status = JsonObject();

        auto current = settings();
        if (!jsonBool(current, L"hotkeysEnabled", true)) {
            unbind();
            return cloneObject(status);
        }

        auto configured = jsonObject(current, L"hotkeys");
        std::vector<Hotkey> hotkeys;

        for (auto action : ACTIONS) {
            auto accelerator = jsonString(configured, action);
            if (accelerator.empty()) continue;

            if (auto parsed = parseAccelerator(accelerator)) hotkeys.push_back({action, *parsed});
            else status.SetNamedValue(action, JsonValue::CreateStringValue(L"invalid"));
        }

        auto results = bindHotkeys(hotkeys);
        for (size_t index = 0; index < hotkeys.size(); index++) {
            status.SetNamedValue(hotkeys[index].action, JsonValue::CreateStringValue(results[index]));
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
        unbind();
        return true;
    });
}

void unregisterHotkeys() {
    std::lock_guard lock(mutex);
    unbind();
}

}

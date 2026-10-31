#include "hotkeys.h"

#include "app.h"

#include <cwctype>

namespace fileship::host {

std::vector<std::wstring> const& hotkeyActions() {
    static std::vector<std::wstring> const actions{
        L"captureMonitor",    L"captureRegion",     L"captureWindow",   L"captureText",  L"recordVideo",      L"recordGif",
        L"captureFullscreen", L"captureLastRegion", L"uploadClipboard", L"uploadFile", L"shortenClipboard", L"openMain",
    };
    return actions;
}

std::vector<std::pair<std::wstring, std::wstring>> boundHotkeys() {
    std::vector<std::pair<std::wstring, std::wstring>> bound;

    auto current = settings();
    if (!jsonBool(current, L"hotkeysEnabled", true)) return bound;

    auto hotkeys = jsonObject(current, L"hotkeys");
    for (auto const& action : hotkeyActions()) {
        auto accelerator = jsonString(hotkeys, action.c_str());
        if (!accelerator.empty()) bound.emplace_back(action, accelerator);
    }

    return bound;
}

std::wstring lowerKeyName(std::wstring text) {
    for (auto& character : text) character = static_cast<wchar_t>(towlower(character));
    return text;
}

std::optional<Accelerator> parseAccelerator(std::wstring const& text, KeyResolver const& resolve) {
    Accelerator result;
    bool hasKey = false;
    size_t start = 0;

    while (start <= text.size()) {
        auto plus = text.find(L'+', start);
        auto token = text.substr(start, plus == std::wstring::npos ? std::wstring::npos : plus - start);

        if (token.empty() && plus != std::wstring::npos) {
            token = L"Plus";
            plus++;
        }

        auto name = lowerKeyName(token);

        if (name == L"command" || name == L"cmd" || name == L"commandorcontrol" || name == L"cmdorctrl" ||
            name == L"control" || name == L"ctrl") {
            result.modifiers |= HotkeyControl;
        } else if (name == L"alt" || name == L"option") {
            result.modifiers |= HotkeyAlt;
        } else if (name == L"altgr") {
            result.modifiers |= HotkeyControl | HotkeyAlt;
        } else if (name == L"shift") {
            result.modifiers |= HotkeyShift;
        } else if (name == L"super" || name == L"meta") {
            result.modifiers |= HotkeySuper;
        } else {
            auto key = resolve(token);
            if (!key || hasKey) return std::nullopt;
            result.key = *key;
            hasKey = true;
        }

        if (plus == std::wstring::npos) break;
        start = plus + 1;
    }

    if (!hasKey) return std::nullopt;
    return result;
}

}

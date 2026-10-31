#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace fileship::host {

enum HotkeyModifier : uint32_t {
    HotkeyControl = 1,
    HotkeyAlt = 2,
    HotkeyShift = 4,
    HotkeySuper = 8,
};

struct Accelerator {
    uint32_t modifiers = 0;
    uint32_t key = 0;
};

using KeyResolver = std::function<std::optional<uint32_t>(std::wstring const& name)>;

std::vector<std::wstring> const& hotkeyActions();
std::vector<std::pair<std::wstring, std::wstring>> boundHotkeys();
std::wstring lowerKeyName(std::wstring text);
std::optional<Accelerator> parseAccelerator(std::wstring const& text, KeyResolver const& resolve);

}

#pragma once

#include <cstdint>

namespace fileship::ui {

enum class Key : uint32_t {
    None = 0,
    Backspace = 0x100,
    Tab,
    Enter,
    Escape,
    Space,
    Delete,
    Left,
    Up,
    Right,
    Down,
    Home,
    End,
    Numpad0,
    Numpad1,
    Numpad2,
    Numpad3,
    Numpad4,
    Numpad5,
    Numpad6,
    Numpad7,
    Numpad8,
    Numpad9,
};

constexpr Key keyOf(char character) {
    return static_cast<Key>(character);
}

}

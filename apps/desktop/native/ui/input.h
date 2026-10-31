#pragma once

namespace fileship::ui {

enum class Key {
    Unknown,
    Escape,
    Enter,
    Space,
    Tab,
    Backspace,
    Delete,
    Left,
    Right,
    Up,
    Down,
    Home,
    End,
    A,
    C,
    V,
    X,
    Y,
    Z,
    Digit0,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Digit5,
    Digit6,
    Digit7,
    Digit8,
    Digit9,
};

struct Modifiers {
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
};

enum class CursorKind { Arrow, Cross, Hand, IBeam, Move, ResizeNwse, ResizeNesw, ResizeNs, ResizeWe };

inline int digitOf(Key key) {
    return key >= Key::Digit0 && key <= Key::Digit9 ? static_cast<int>(key) - static_cast<int>(Key::Digit0) : -1;
}

}

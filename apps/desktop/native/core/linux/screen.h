#pragma once

#include <core/core.h>

namespace fileship::core {

bool isWayland();

namespace x11 {

bool captureRects(std::vector<Rect> const& rects, bool cursor, std::vector<uint8_t*> const& targets);
std::vector<WindowInfo> listWindows(std::vector<uint64_t> const& ignored);
std::optional<WindowInfo> foregroundWindow(std::vector<uint64_t> const& ignored);
std::vector<uint64_t> processWindows(std::vector<uint32_t> const& processes);
std::vector<DisplayInfo> listDisplays();
Rect virtualScreen();
Point cursorPosition();

}

namespace wayland {

bool captureRects(std::vector<Rect> const& rects, bool cursor, std::vector<uint8_t*> const& targets);
std::vector<DisplayInfo> listDisplays();
Rect virtualScreen();
Point cursorPosition();

}

}

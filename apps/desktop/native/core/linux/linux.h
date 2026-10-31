#pragma once

#include <core/core.h>

#include <string_view>

namespace fileship::core {

bool isWaylandSession();
int64_t displayId(std::string_view connector);
Rect boundsOf(std::vector<DisplayInfo> const& displays);

namespace x11 {

std::vector<DisplayInfo> listDisplays();
std::optional<Point> cursorPosition();
bool captureRects(std::vector<Rect> const& rects, bool cursor, std::vector<uint8_t*> const& targets);
std::vector<WindowInfo> listWindows(std::vector<uint64_t> const& ignored);
std::optional<WindowInfo> foregroundWindow(std::vector<uint64_t> const& ignored);
std::vector<uint64_t> windowsOfProcesses(std::set<uint32_t> const& processes);

}

namespace wayland {

std::vector<DisplayInfo> listDisplays();

}

namespace portal {

bool captureRects(std::vector<Rect> const& rects, std::vector<uint8_t*> const& targets);

}

}

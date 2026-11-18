#include "screen.h"

#include <cstdlib>

namespace fileship::core {

bool isWayland() {
    return std::getenv("WAYLAND_DISPLAY") != nullptr;
}

bool captureRects(std::vector<Rect> const& rects, bool cursor, std::vector<uint8_t*> const& targets) {
    return isWayland() ? wayland::captureRects(rects, cursor, targets) : x11::captureRects(rects, cursor, targets);
}

bool captureRectInto(Rect const& rect, bool cursor, uint8_t* pixels) {
    return captureRects({rect}, cursor, {pixels});
}

std::optional<Bitmap> captureRect(Rect const& rect, bool cursor) {
    if (rect.width <= 0 || rect.height <= 0) return std::nullopt;

    Bitmap bitmap{std::vector<uint8_t>(static_cast<size_t>(rect.width) * rect.height * 4),
                  static_cast<uint32_t>(rect.width), static_cast<uint32_t>(rect.height)};
    if (!captureRectInto(rect, cursor, bitmap.pixels.data())) return std::nullopt;

    return bitmap;
}

std::vector<WindowInfo> listWindows(std::vector<uint64_t> const& ignored, bool) {
    return isWayland() ? std::vector<WindowInfo>() : x11::listWindows(ignored);
}

std::optional<WindowInfo> foregroundWindow(std::vector<uint64_t> const& ignored) {
    return isWayland() ? std::nullopt : x11::foregroundWindow(ignored);
}

std::vector<uint64_t> processWindows(std::vector<uint32_t> const& processes) {
    return isWayland() ? std::vector<uint64_t>() : x11::processWindows(processes);
}

std::vector<DisplayInfo> listDisplays() {
    return isWayland() ? wayland::listDisplays() : x11::listDisplays();
}

Rect virtualScreen() {
    return isWayland() ? wayland::virtualScreen() : x11::virtualScreen();
}

Point cursorPosition() {
    return isWayland() ? wayland::cursorPosition() : x11::cursorPosition();
}

DisplayInfo displayNearest(Point point) {
    auto displays = listDisplays();
    if (displays.empty()) return DisplayInfo{0, virtualScreen(), 1, true};

    auto distance = [point](DisplayInfo const& display) {
        int64_t dx = std::max({display.rect.x - point.x, point.x - (display.rect.x + display.rect.width - 1), 0});
        int64_t dy = std::max({display.rect.y - point.y, point.y - (display.rect.y + display.rect.height - 1), 0});
        return dx * dx + dy * dy;
    };

    return *std::min_element(displays.begin(), displays.end(),
                             [&](auto const& a, auto const& b) { return distance(a) < distance(b); });
}

}

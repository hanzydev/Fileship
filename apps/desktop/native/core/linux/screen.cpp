#include "linux.h"

#include <algorithm>
#include <cstdlib>
#include <limits>

namespace fileship::core {

namespace {

bool hasVariable(char const* name) {
    auto value = std::getenv(name);
    return value && *value;
}

int64_t distanceTo(Rect const& rect, Point point) {
    auto axis = [](int32_t value, int32_t start, int32_t length) -> int64_t {
        if (value < start) return start - value;
        if (value >= start + length) return value - (start + length - 1);
        return 0;
    };

    auto x = axis(point.x, rect.x, rect.width);
    auto y = axis(point.y, rect.y, rect.height);
    return x * x + y * y;
}

Point centerOf(Rect const& rect) {
    return {rect.x + rect.width / 2, rect.y + rect.height / 2};
}

}

bool isWaylandSession() {
    static bool const isWayland = hasVariable("WAYLAND_DISPLAY") || !hasVariable("DISPLAY");
    return isWayland;
}

int64_t displayId(std::string_view connector) {
    uint32_t hash = 2166136261u;

    for (auto character : connector) {
        hash ^= static_cast<uint8_t>(character);
        hash *= 16777619u;
    }

    return hash;
}

Rect boundsOf(std::vector<DisplayInfo> const& displays) {
    if (displays.empty()) return {};

    int32_t left = std::numeric_limits<int32_t>::max();
    int32_t top = std::numeric_limits<int32_t>::max();
    int32_t right = std::numeric_limits<int32_t>::min();
    int32_t bottom = std::numeric_limits<int32_t>::min();

    for (auto const& display : displays) {
        left = std::min(left, display.rect.x);
        top = std::min(top, display.rect.y);
        right = std::max(right, display.rect.x + display.rect.width);
        bottom = std::max(bottom, display.rect.y + display.rect.height);
    }

    return {left, top, right - left, bottom - top};
}

bool captureRects(std::vector<Rect> const& rects, bool cursor, std::vector<uint8_t*> const& targets) {
    if (std::any_of(rects.begin(), rects.end(), [](Rect const& rect) { return rect.width <= 0 || rect.height <= 0; }))
        return false;

    return isWaylandSession() ? portal::captureRects(rects, targets) : x11::captureRects(rects, cursor, targets);
}

bool captureRectInto(Rect const& rect, bool cursor, uint8_t* pixels) {
    return captureRects({rect}, cursor, {pixels});
}

std::optional<Bitmap> captureRect(Rect const& rect, bool cursor) {
    if (rect.width <= 0 || rect.height <= 0) return std::nullopt;

    Bitmap bitmap;
    bitmap.width = static_cast<uint32_t>(rect.width);
    bitmap.height = static_cast<uint32_t>(rect.height);
    bitmap.pixels.resize(static_cast<size_t>(rect.width) * rect.height * 4);

    if (!captureRectInto(rect, cursor, bitmap.pixels.data())) return std::nullopt;
    return bitmap;
}

std::vector<WindowInfo> listWindows(std::vector<uint64_t> const& ignored, bool) {
    if (isWaylandSession()) return {};
    return x11::listWindows(ignored);
}

std::optional<WindowInfo> foregroundWindow(std::vector<uint64_t> const& ignored) {
    if (isWaylandSession()) return std::nullopt;
    return x11::foregroundWindow(ignored);
}

std::vector<uint64_t> windowsOfProcesses(std::set<uint32_t> const& processes) {
    if (isWaylandSession()) return {};
    return x11::windowsOfProcesses(processes);
}

std::vector<DisplayOutput> listOutputs() {
    return {};
}

std::vector<DisplayInfo> listDisplays() {
    return isWaylandSession() ? wayland::listDisplays() : x11::listDisplays();
}

Rect virtualScreen() {
    return boundsOf(listDisplays());
}

DisplayInfo displayNearest(Point point) {
    auto displays = listDisplays();
    if (displays.empty()) return DisplayInfo{0, {}, 1, true};

    return *std::min_element(displays.begin(), displays.end(), [&](DisplayInfo const& a, DisplayInfo const& b) {
        return distanceTo(a.rect, point) < distanceTo(b.rect, point);
    });
}

Point cursorPosition() {
    if (!isWaylandSession()) {
        if (auto position = x11::cursorPosition()) return *position;
    }

    auto displays = listDisplays();
    if (displays.empty()) return {};

    auto primary = std::find_if(displays.begin(), displays.end(), [](DisplayInfo const& display) {
        return display.isPrimary;
    });
    return centerOf(primary == displays.end() ? displays.front().rect : primary->rect);
}

}

#include "recorderPlacement.h"

#include <ui/recorderUi.h>

#include <algorithm>
#include <cmath>

namespace fileship::ui {

namespace {

constexpr double MARGIN = 8;
constexpr double BORDER_PADDING = 3;
constexpr double BORDER_WIDTH = 2;

int pixels(double value) {
    return static_cast<int>(std::lround(value));
}

}

std::optional<ScreenRect> placeRecorderToolbar(ScreenRect const& area, ScreenRect const& monitor, double scale) {
    int width = pixels(RECORDER_WIDTH * scale);
    int height = pixels(RECORDER_HEIGHT * scale);
    int margin = pixels(MARGIN * scale);

    int x = std::clamp(area.x + area.width / 2 - width / 2, monitor.x, monitor.x + monitor.width - width);
    int y = area.y + area.height + margin;
    if (y + height > monitor.y + monitor.height) y = area.y - height - margin;
    if (y < monitor.y) return std::nullopt;

    return ScreenRect{x, y, width, height};
}

std::vector<ScreenRect> recorderBorders(ScreenRect const& area, ScreenRect const& screen, double scale) {
    int pad = pixels(BORDER_PADDING * scale);
    int line = std::max(1, pixels(BORDER_WIDTH * scale));
    int screenRight = screen.x + screen.width;
    int screenBottom = screen.y + screen.height;

    int left = std::max(area.x - pad, screen.x);
    int top = std::max(area.y - pad, screen.y);
    int right = std::min(area.x + area.width + pad, screenRight);
    int bottom = std::min(area.y + area.height + pad, screenBottom);

    std::vector<ScreenRect> sides;
    if (area.y - pad >= screen.y) sides.push_back({left, top, right - left, line});
    if (area.y + area.height + pad <= screenBottom) sides.push_back({left, bottom - line, right - left, line});
    if (area.x - pad >= screen.x) sides.push_back({left, top, line, bottom - top});
    if (area.x + area.width + pad <= screenRight) sides.push_back({right - line, top, line, bottom - top});
    return sides;
}

}

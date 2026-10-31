#include "core.h"

#include <algorithm>

namespace fileship::core {

std::optional<Rect> intersectRects(Rect const& a, Rect const& b) {
    int x = std::max(a.x, b.x);
    int y = std::max(a.y, b.y);
    int right = std::min(a.x + a.width, b.x + b.width);
    int bottom = std::min(a.y + a.height, b.y + b.height);

    if (right <= x || bottom <= y) return std::nullopt;
    return Rect{x, y, right - x, bottom - y};
}

}

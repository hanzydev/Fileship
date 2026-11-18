#include "screen.h"

namespace fileship::core::wayland {

bool captureRects(std::vector<Rect> const&, bool, std::vector<uint8_t*> const&) {
    return false;
}

std::vector<DisplayInfo> listDisplays() {
    return {};
}

Rect virtualScreen() {
    return {};
}

Point cursorPosition() {
    return {};
}

}

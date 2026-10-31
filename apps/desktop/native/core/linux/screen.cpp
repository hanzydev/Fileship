#include <core/core.h>

namespace fileship::core {

bool captureRectInto(Rect const&, bool, uint8_t*) {
    return false;
}

bool captureRects(std::vector<Rect> const&, bool, std::vector<uint8_t*> const&) {
    return false;
}

std::optional<Bitmap> captureRect(Rect const&, bool) {
    return std::nullopt;
}

std::vector<WindowInfo> listWindows(std::vector<uint64_t> const&, bool) {
    return {};
}

std::optional<WindowInfo> foregroundWindow(std::vector<uint64_t> const&) {
    return std::nullopt;
}

std::vector<uint64_t> windowsOfProcesses(std::set<uint32_t> const&) {
    return {};
}

std::vector<DisplayOutput> listOutputs() {
    return {};
}

std::vector<DisplayInfo> listDisplays() {
    return {};
}

Rect virtualScreen() {
    return {};
}

DisplayInfo displayNearest(Point) {
    return {};
}

Point cursorPosition() {
    return {};
}

}

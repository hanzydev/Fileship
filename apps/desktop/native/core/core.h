#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fileship::core {

struct Point {
    int32_t x = 0;
    int32_t y = 0;
};

struct Rect {
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
};

struct WindowInfo {
    Rect frame;
    std::wstring title;
    std::vector<Rect> controls;
};

struct DisplayInfo {
    int64_t id = 0;
    Rect rect;
    double scaleFactor = 1;
    bool isPrimary = false;
};

enum class PixelOrder { Bgra, Rgba };

enum class ImageFormat { Png, Jpeg };

struct EncodeOptions {
    PixelOrder order = PixelOrder::Bgra;
    ImageFormat format = ImageFormat::Png;
    float quality = 0.9f;
    uint32_t maxWidth = 0;
};

struct Bitmap {
    std::vector<uint8_t> pixels;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct WindowShot {
    Bitmap image;
    std::wstring title;
};

bool captureRectInto(Rect const& rect, bool cursor, uint8_t* pixels);
bool captureRects(std::vector<Rect> const& rects, bool cursor, std::vector<uint8_t*> const& targets);
std::optional<Bitmap> captureRect(Rect const& rect, bool cursor);

std::vector<WindowInfo> listWindows(std::vector<uint64_t> const& ignored, bool controls);
std::optional<WindowShot> captureForegroundWindow(std::vector<uint64_t> const& ignored, bool cursor);
std::vector<uint64_t> processWindows(std::vector<uint32_t> const& processes);

std::vector<DisplayInfo> listDisplays();
Rect virtualScreen();
DisplayInfo displayNearest(Point point);
Point cursorPosition();

inline std::optional<Rect> intersectRects(Rect const& a, Rect const& b) {
    int32_t x = std::max(a.x, b.x);
    int32_t y = std::max(a.y, b.y);
    int32_t right = std::min(a.x + a.width, b.x + b.width);
    int32_t bottom = std::min(a.y + a.height, b.y + b.height);

    if (right <= x || bottom <= y) return std::nullopt;
    return Rect{x, y, right - x, bottom - y};
}

std::vector<uint8_t> encodeImage(uint8_t const* pixels, uint32_t width, uint32_t height,
                                 EncodeOptions const& options);
std::optional<Bitmap> decodeImage(uint8_t const* data, size_t size);
std::optional<Bitmap> decodeImageFile(std::filesystem::path const& path);

bool playSound(uint8_t const* wav, size_t size);

}

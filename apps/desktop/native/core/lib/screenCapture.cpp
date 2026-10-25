#include "core.h"

#include <thread>

namespace fileship::core {

namespace {

void drawCursor(HDC context, Rect const& origin) {
    CURSORINFO info{};
    info.cbSize = sizeof(info);
    if (!GetCursorInfo(&info) || !(info.flags & CURSOR_SHOWING) || !info.hCursor) return;

    ICONINFO icon{};
    bool hasIcon = GetIconInfo(info.hCursor, &icon) != FALSE;

    DrawIconEx(context, info.ptScreenPos.x - origin.x - (hasIcon ? static_cast<int>(icon.xHotspot) : 0),
               info.ptScreenPos.y - origin.y - (hasIcon ? static_cast<int>(icon.yHotspot) : 0), info.hCursor,
               0, 0, 0, nullptr, DI_NORMAL);

    if (hasIcon) {
        if (icon.hbmMask) DeleteObject(icon.hbmMask);
        if (icon.hbmColor) DeleteObject(icon.hbmColor);
    }
}

}

bool captureRectInto(Rect const& rect, bool cursor, uint8_t* pixels) {
    HDC screen = GetDC(nullptr);
    HDC memory = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateCompatibleBitmap(screen, rect.width, rect.height);
    HGDIOBJ previous = SelectObject(memory, bitmap);

    bool isCaptured = BitBlt(memory, 0, 0, rect.width, rect.height, screen, rect.x, rect.y,
                             SRCCOPY | CAPTUREBLT) != FALSE;

    if (isCaptured && cursor) drawCursor(memory, rect);

    SelectObject(memory, previous);

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = rect.width;
    info.bmiHeader.biHeight = -rect.height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    if (isCaptured)
        isCaptured = GetDIBits(memory, bitmap, 0, rect.height, pixels, &info, DIB_RGB_COLORS) == rect.height;

    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(nullptr, screen);

    if (!isCaptured) return false;

    size_t length = static_cast<size_t>(rect.width) * rect.height * 4;
    for (size_t index = 3; index < length; index += 4) pixels[index] = 255;

    return true;
}

bool captureRects(std::vector<Rect> const& rects, bool cursor, std::vector<uint8_t*> const& targets) {
    std::vector<char> results(rects.size(), 0);
    std::vector<std::thread> threads;

    for (size_t index = 0; index < rects.size(); index++) {
        threads.emplace_back([&, index] { results[index] = captureRectInto(rects[index], cursor, targets[index]); });
    }

    for (auto& thread : threads) thread.join();

    for (char result : results) {
        if (!result) return false;
    }

    return true;
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

}

#include "linux.h"
#include "x11.h"

#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xfixes.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <algorithm>
#include <cstring>

namespace fileship::core::x11 {

namespace {

struct Channel {
    int shift = 0;
    int bits = 0;
};

Channel channelOf(unsigned long mask) {
    Channel channel;
    if (!mask) return channel;

    while (!(mask & 1)) {
        mask >>= 1;
        channel.shift++;
    }

    while (mask & 1) {
        mask >>= 1;
        channel.bits++;
    }

    return channel;
}

uint8_t channelValue(unsigned long pixel, Channel const& channel) {
    if (!channel.bits) return 0;

    auto maximum = (1ul << channel.bits) - 1;
    return static_cast<uint8_t>(((pixel >> channel.shift) & maximum) * 255 / maximum);
}

void copyImage(XImage* image, uint8_t* target, size_t stride) {
    bool isNative = image->bits_per_pixel == 32 && image->red_mask == 0xff0000 && image->green_mask == 0xff00 &&
                    image->blue_mask == 0xff && image->byte_order == LSBFirst;

    if (isNative) {
        for (int y = 0; y < image->height; y++) {
            std::memcpy(target + y * stride, image->data + static_cast<size_t>(y) * image->bytes_per_line,
                        static_cast<size_t>(image->width) * 4);
        }

        return;
    }

    auto red = channelOf(image->red_mask);
    auto green = channelOf(image->green_mask);
    auto blue = channelOf(image->blue_mask);

    for (int y = 0; y < image->height; y++) {
        auto row = target + y * stride;

        for (int x = 0; x < image->width; x++) {
            auto pixel = XGetPixel(image, x, y);
            row[x * 4] = channelValue(pixel, blue);
            row[x * 4 + 1] = channelValue(pixel, green);
            row[x * 4 + 2] = channelValue(pixel, red);
        }
    }
}

bool grabShared(Display* display, XWindowAttributes const& root, Rect const& part, uint8_t* target, size_t stride) {
    XShmSegmentInfo info{};
    auto image = XShmCreateImage(display, root.visual, static_cast<unsigned int>(root.depth), ZPixmap, nullptr, &info,
                                 static_cast<unsigned int>(part.width), static_cast<unsigned int>(part.height));
    if (!image) return false;

    info.shmid = shmget(IPC_PRIVATE, static_cast<size_t>(image->bytes_per_line) * image->height, IPC_CREAT | 0600);
    info.shmaddr = info.shmid < 0 ? reinterpret_cast<char*>(-1) : static_cast<char*>(shmat(info.shmid, nullptr, 0));

    if (info.shmaddr == reinterpret_cast<char*>(-1)) {
        if (info.shmid >= 0) shmctl(info.shmid, IPC_RMID, nullptr);
        XDestroyImage(image);
        return false;
    }

    image->data = info.shmaddr;
    info.readOnly = False;

    bool isAttached = XShmAttach(display, &info) && !syncErrors(display);
    shmctl(info.shmid, IPC_RMID, nullptr);

    bool isCaptured =
        isAttached && XShmGetImage(display, root.root, image, part.x, part.y, AllPlanes) && !syncErrors(display);
    if (isCaptured) copyImage(image, target, stride);

    if (isAttached) {
        XShmDetach(display, &info);
        syncErrors(display);
    }

    shmdt(info.shmaddr);
    image->data = nullptr;
    XDestroyImage(image);
    return isCaptured;
}

bool grabCopy(Display* display, XWindowAttributes const& root, Rect const& part, uint8_t* target, size_t stride) {
    auto image = XGetImage(display, root.root, part.x, part.y, static_cast<unsigned int>(part.width),
                           static_cast<unsigned int>(part.height), AllPlanes, ZPixmap);
    if (!image || syncErrors(display)) {
        if (image) XDestroyImage(image);
        return false;
    }

    copyImage(image, target, stride);
    XDestroyImage(image);
    return true;
}

void drawCursor(Display* display, std::vector<Rect> const& rects, std::vector<uint8_t*> const& targets) {
    int eventBase = 0;
    int errorBase = 0;
    int major = 0;
    int minor = 0;
    if (!XFixesQueryExtension(display, &eventBase, &errorBase) || !XFixesQueryVersion(display, &major, &minor)) return;

    auto cursor = XFixesGetCursorImage(display);
    if (!cursor) return;

    int left = cursor->x - cursor->xhot;
    int top = cursor->y - cursor->yhot;

    for (size_t index = 0; index < rects.size(); index++) {
        auto const& rect = rects[index];

        for (int row = 0; row < cursor->height; row++) {
            int y = top + row - rect.y;
            if (y < 0 || y >= rect.height) continue;

            for (int column = 0; column < cursor->width; column++) {
                int x = left + column - rect.x;
                if (x < 0 || x >= rect.width) continue;

                auto argb = static_cast<uint32_t>(cursor->pixels[row * cursor->width + column]);
                uint32_t alpha = argb >> 24;
                if (!alpha) continue;

                auto pixel = targets[index] + (static_cast<size_t>(y) * rect.width + x) * 4;
                for (int channel = 0; channel < 3; channel++) {
                    uint32_t source = (argb >> (channel * 8)) & 0xff;
                    pixel[channel] = static_cast<uint8_t>(std::min<uint32_t>(255, source + pixel[channel] * (255 - alpha) / 255));
                }
            }
        }
    }

    XFree(cursor);
}

}

bool captureRects(std::vector<Rect> const& rects, bool cursor, std::vector<uint8_t*> const& targets) {
    bool isCaptured = false;

    withDisplay([&](Display* display) {
        XWindowAttributes root{};
        if (!XGetWindowAttributes(display, DefaultRootWindow(display), &root)) return;

        auto screen = rootRect(display);
        int major = 0;
        int minor = 0;
        Bool pixmaps = False;
        bool useShared = XShmQueryVersion(display, &major, &minor, &pixmaps);
        isCaptured = true;

        for (size_t index = 0; index < rects.size() && isCaptured; index++) {
            auto const& rect = rects[index];
            auto stride = static_cast<size_t>(rect.width) * 4;
            auto part = intersectRects(rect, screen);

            if (!part || part->width != rect.width || part->height != rect.height)
                std::memset(targets[index], 0, stride * rect.height);
            if (!part) continue;

            auto target = targets[index] + static_cast<size_t>(part->y - rect.y) * stride +
                          static_cast<size_t>(part->x - rect.x) * 4;
            if (useShared && grabShared(display, root, *part, target, stride)) continue;

            useShared = false;
            isCaptured = grabCopy(display, root, *part, target, stride);
        }

        if (isCaptured && cursor) drawCursor(display, rects, targets);
    });

    if (!isCaptured) return false;

    for (size_t index = 0; index < rects.size(); index++) {
        size_t length = static_cast<size_t>(rects[index].width) * rects[index].height * 4;
        for (size_t offset = 3; offset < length; offset += 4) targets[index][offset] = 255;
    }

    return true;
}

}

#include "screen.h"

#include <shared/utf8.h>

#include <xcb/randr.h>
#include <xcb/xcb.h>
#include <xcb/xfixes.h>

#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <memory>

namespace fileship::core::x11 {

namespace {

template <typename Reply> using Owned = std::unique_ptr<Reply, decltype(&free)>;

template <typename Reply> Owned<Reply> own(Reply* reply) {
    return Owned<Reply>(reply, free);
}

class Connection {
public:
    Connection() : connection(xcb_connect(nullptr, nullptr)) {
        if (xcb_connection_has_error(connection)) return;

        screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
        root = screen->root;
    }

    ~Connection() {
        xcb_disconnect(connection);
    }

    explicit operator bool() const {
        return screen != nullptr;
    }

    operator xcb_connection_t*() const {
        return connection;
    }

    xcb_atom_t atom(char const* name) const {
        auto reply = own(xcb_intern_atom_reply(
            connection, xcb_intern_atom(connection, 0, static_cast<uint16_t>(std::strlen(name)), name), nullptr));
        return reply->atom;
    }

    std::vector<uint8_t> property(xcb_window_t window, xcb_atom_t name) const {
        auto reply = own(xcb_get_property_reply(
            connection, xcb_get_property(connection, 0, window, name, XCB_GET_PROPERTY_TYPE_ANY, 0, 1 << 20), nullptr));
        if (!reply) return {};

        auto data = static_cast<uint8_t const*>(xcb_get_property_value(reply.get()));
        return std::vector<uint8_t>(data, data + xcb_get_property_value_length(reply.get()));
    }

    std::vector<uint32_t> values(xcb_window_t window, char const* name) const {
        auto data = property(window, atom(name));
        std::vector<uint32_t> result(data.size() / 4);
        std::memcpy(result.data(), data.data(), result.size() * 4);
        return result;
    }

    xcb_connection_t* connection;
    xcb_screen_t* screen = nullptr;
    xcb_window_t root = 0;
};

Rect screenRect(Connection const& x) {
    return Rect{0, 0, x.screen->width_in_pixels, x.screen->height_in_pixels};
}

double scaleFactor(Connection const& x) {
    auto data = x.property(x.root, XCB_ATOM_RESOURCE_MANAGER);
    std::string resources(data.begin(), data.end());

    auto start = resources.find("Xft.dpi:");
    if (start == std::string::npos) return 1;

    return std::max(std::atof(resources.c_str() + start + 8) / 96, 1.0);
}

void drawCursor(Connection const& x, Rect const& rect, uint8_t* pixels) {
    own(xcb_xfixes_query_version_reply(x, xcb_xfixes_query_version(x, 4, 0), nullptr));

    auto cursor = own(xcb_xfixes_get_cursor_image_reply(x, xcb_xfixes_get_cursor_image(x), nullptr));
    if (!cursor) return;

    auto image = xcb_xfixes_get_cursor_image_cursor_image(cursor.get());
    int left = cursor->x - cursor->xhot - rect.x;
    int top = cursor->y - cursor->yhot - rect.y;

    for (int row = 0; row < cursor->height; row++) {
        for (int column = 0; column < cursor->width; column++) {
            int px = left + column;
            int py = top + row;
            if (px < 0 || py < 0 || px >= rect.width || py >= rect.height) continue;

            uint32_t source = image[row * cursor->width + column];
            uint32_t alpha = source >> 24;
            auto target = pixels + (static_cast<size_t>(py) * rect.width + px) * 4;

            for (int channel = 0; channel < 3; channel++) {
                uint32_t value = (source >> (channel * 8)) & 0xff;
                target[channel] = static_cast<uint8_t>(value + target[channel] * (255 - alpha) / 255);
            }
        }
    }
}

std::optional<WindowInfo> describe(Connection const& x, xcb_window_t window) {
    auto geometry = own(xcb_get_geometry_reply(x, xcb_get_geometry(x, window), nullptr));
    auto origin = own(xcb_translate_coordinates_reply(x, xcb_translate_coordinates(x, window, x.root, 0, 0), nullptr));
    if (!geometry || !origin) return std::nullopt;

    WindowInfo info;
    info.frame = Rect{origin->dst_x, origin->dst_y, geometry->width, geometry->height};

    if (auto frame = x.values(window, "_NET_FRAME_EXTENTS"); frame.size() == 4) {
        info.frame = Rect{info.frame.x - static_cast<int>(frame[0]), info.frame.y - static_cast<int>(frame[2]),
                          info.frame.width + static_cast<int>(frame[0] + frame[1]),
                          info.frame.height + static_cast<int>(frame[2] + frame[3])};
    }

    if (auto shadow = x.values(window, "_GTK_FRAME_EXTENTS"); shadow.size() == 4) {
        info.frame = Rect{info.frame.x + static_cast<int>(shadow[0]), info.frame.y + static_cast<int>(shadow[2]),
                          info.frame.width - static_cast<int>(shadow[0] + shadow[1]),
                          info.frame.height - static_cast<int>(shadow[2] + shadow[3])};
    }

    auto title = x.property(window, x.atom("_NET_WM_NAME"));
    if (title.empty()) title = x.property(window, XCB_ATOM_WM_NAME);
    info.title = toWide(std::string(title.begin(), title.end()));

    return info;
}

bool contains(std::vector<uint64_t> const& values, uint64_t value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

}

bool captureRects(std::vector<Rect> const& rects, bool cursor, std::vector<uint8_t*> const& targets) {
    Connection x;
    if (!x) return false;

    for (size_t index = 0; index < rects.size(); index++) {
        auto const& rect = rects[index];
        auto pixels = targets[index];
        std::memset(pixels, 0, static_cast<size_t>(rect.width) * rect.height * 4);

        auto visible = intersectRects(rect, screenRect(x));
        if (!visible) continue;

        auto image = own(xcb_get_image_reply(
            x,
            xcb_get_image(x, XCB_IMAGE_FORMAT_Z_PIXMAP, x.root, static_cast<int16_t>(visible->x),
                          static_cast<int16_t>(visible->y), static_cast<uint16_t>(visible->width),
                          static_cast<uint16_t>(visible->height), ~0u),
            nullptr));
        if (!image) return false;

        auto source = xcb_get_image_data(image.get());

        for (int row = 0; row < visible->height; row++) {
            auto line = pixels +
                        (static_cast<size_t>(visible->y - rect.y + row) * rect.width + (visible->x - rect.x)) * 4;
            std::memcpy(line, source + static_cast<size_t>(row) * visible->width * 4,
                        static_cast<size_t>(visible->width) * 4);
            for (int column = 0; column < visible->width; column++) line[column * 4 + 3] = 255;
        }

        if (cursor) drawCursor(x, rect, pixels);
    }

    return true;
}

std::vector<WindowInfo> listWindows(std::vector<uint64_t> const& ignored) {
    Connection x;
    if (!x) return {};

    std::vector<WindowInfo> windows;
    auto stacking = x.values(x.root, "_NET_CLIENT_LIST_STACKING");
    auto desktops = x.values(x.root, "_NET_CURRENT_DESKTOP");
    auto hidden = x.atom("_NET_WM_STATE_HIDDEN");

    for (auto window = stacking.rbegin(); window != stacking.rend(); window++) {
        if (contains(ignored, *window)) continue;

        auto states = x.values(*window, "_NET_WM_STATE");
        if (std::find(states.begin(), states.end(), hidden) != states.end()) continue;

        auto desktop = x.values(*window, "_NET_WM_DESKTOP");
        if (!desktop.empty() && !desktops.empty() && desktop[0] != desktops[0] && desktop[0] != 0xffffffff) continue;

        if (auto info = describe(x, *window)) windows.push_back(std::move(*info));
    }

    return windows;
}

std::optional<WindowInfo> foregroundWindow(std::vector<uint64_t> const& ignored) {
    Connection x;
    if (!x) return std::nullopt;

    auto active = x.values(x.root, "_NET_ACTIVE_WINDOW");
    if (active.empty() || !active[0] || contains(ignored, active[0])) return std::nullopt;

    return describe(x, active[0]);
}

std::vector<uint64_t> processWindows(std::vector<uint32_t> const& processes) {
    Connection x;
    if (!x) return {};

    std::vector<uint64_t> windows;

    for (auto window : x.values(x.root, "_NET_CLIENT_LIST")) {
        auto pid = x.values(window, "_NET_WM_PID");
        if (pid.empty()) continue;

        if (pid[0] == static_cast<uint32_t>(getpid()) ||
            std::find(processes.begin(), processes.end(), pid[0]) != processes.end()) {
            windows.push_back(window);
        }
    }

    return windows;
}

std::vector<DisplayInfo> listDisplays() {
    Connection x;
    if (!x) return {};

    std::vector<DisplayInfo> displays;
    double scale = scaleFactor(x);
    auto monitors = own(xcb_randr_get_monitors_reply(x, xcb_randr_get_monitors(x, x.root, 1), nullptr));

    if (monitors) {
        for (auto monitor = xcb_randr_get_monitors_monitors_iterator(monitors.get()); monitor.rem;
             xcb_randr_monitor_info_next(&monitor)) {
            displays.push_back(DisplayInfo{monitor.data->name,
                                           Rect{monitor.data->x, monitor.data->y, monitor.data->width, monitor.data->height},
                                           scale, monitor.data->primary != 0});
        }
    }

    if (displays.empty()) displays.push_back(DisplayInfo{0, screenRect(x), scale, true});
    if (std::none_of(displays.begin(), displays.end(), [](auto const& display) { return display.isPrimary; })) {
        displays[0].isPrimary = true;
    }

    return displays;
}

Rect virtualScreen() {
    Connection x;
    return x ? screenRect(x) : Rect{};
}

Point cursorPosition() {
    Connection x;
    if (!x) return {};

    auto pointer = own(xcb_query_pointer_reply(x, xcb_query_pointer(x, x.root), nullptr));
    return pointer ? Point{pointer->root_x, pointer->root_y} : Point{};
}

}

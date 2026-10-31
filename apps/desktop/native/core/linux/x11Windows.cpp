#include "linux.h"
#include "x11.h"

#include <shared/utf8.h>

#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <algorithm>

namespace fileship::core::x11 {

namespace {

bool contains(std::vector<long> const& values, Atom value) {
    return std::find(values.begin(), values.end(), static_cast<long>(value)) != values.end();
}

bool isIgnored(std::vector<uint64_t> const& ignored, Window window) {
    return std::find(ignored.begin(), ignored.end(), static_cast<uint64_t>(window)) != ignored.end();
}

std::wstring titleOf(Display* display, Window window) {
    auto title = textProperty(display, window, atom(display, "_NET_WM_NAME"), atom(display, "UTF8_STRING"));

    XTextProperty property{};
    if (title.empty() && XGetWMName(display, window, &property) && property.value) {
        char** list = nullptr;
        int count = 0;
        if (Xutf8TextPropertyToTextList(display, &property, &list, &count) >= Success && list && count > 0)
            title = list[0];
        if (list) XFreeStringList(list);
        XFree(property.value);
    }

    return toWide(title);
}

std::optional<WindowInfo> describe(Display* display, Window window, bool isVisibleOnly) {
    XWindowAttributes attributes{};
    if (!XGetWindowAttributes(display, window, &attributes)) return std::nullopt;
    if (isVisibleOnly && attributes.map_state != IsViewable) return std::nullopt;

    auto state = longProperty(display, window, atom(display, "_NET_WM_STATE"), XA_ATOM);
    if (isVisibleOnly && contains(state, atom(display, "_NET_WM_STATE_HIDDEN"))) return std::nullopt;

    auto types = longProperty(display, window, atom(display, "_NET_WM_WINDOW_TYPE"), XA_ATOM);
    if (contains(types, atom(display, "_NET_WM_WINDOW_TYPE_DESKTOP")) ||
        contains(types, atom(display, "_NET_WM_WINDOW_TYPE_DOCK")))
        return std::nullopt;

    int x = 0;
    int y = 0;
    Window child = None;
    if (!XTranslateCoordinates(display, window, attributes.root, 0, 0, &x, &y, &child)) return std::nullopt;

    Rect frame{x, y, attributes.width, attributes.height};

    auto extents = longProperty(display, window, atom(display, "_NET_FRAME_EXTENTS"), XA_CARDINAL);
    if (extents.size() == 4) {
        frame.x -= static_cast<int32_t>(extents[0]);
        frame.y -= static_cast<int32_t>(extents[2]);
        frame.width += static_cast<int32_t>(extents[0] + extents[1]);
        frame.height += static_cast<int32_t>(extents[2] + extents[3]);
    }

    auto shadows = longProperty(display, window, atom(display, "_GTK_FRAME_EXTENTS"), XA_CARDINAL);
    if (shadows.size() == 4) {
        frame.x += static_cast<int32_t>(shadows[0]);
        frame.y += static_cast<int32_t>(shadows[2]);
        frame.width -= static_cast<int32_t>(shadows[0] + shadows[1]);
        frame.height -= static_cast<int32_t>(shadows[2] + shadows[3]);
    }

    if (frame.width <= 0 || frame.height <= 0) return std::nullopt;
    return WindowInfo{frame, titleOf(display, window), {}};
}

std::vector<Window> stackedWindows(Display* display, Window root) {
    std::vector<Window> windows;

    auto clients = longProperty(display, root, atom(display, "_NET_CLIENT_LIST_STACKING"), XA_WINDOW);
    if (!clients.empty()) {
        windows.assign(clients.rbegin(), clients.rend());
        return windows;
    }

    Window parent = None;
    Window* children = nullptr;
    unsigned int count = 0;
    if (!XQueryTree(display, root, &root, &parent, &children, &count)) return windows;

    for (unsigned int index = count; index > 0; index--) {
        XWindowAttributes attributes{};
        if (XGetWindowAttributes(display, children[index - 1], &attributes) && !attributes.override_redirect)
            windows.push_back(children[index - 1]);
    }

    if (children) XFree(children);
    return windows;
}

}

std::vector<WindowInfo> listWindows(std::vector<uint64_t> const& ignored) {
    std::vector<WindowInfo> windows;

    withDisplay([&](Display* display) {
        for (auto window : stackedWindows(display, DefaultRootWindow(display))) {
            if (isIgnored(ignored, window)) continue;
            if (auto info = describe(display, window, true)) windows.push_back(std::move(*info));
        }
    });

    return windows;
}

std::optional<WindowInfo> foregroundWindow(std::vector<uint64_t> const& ignored) {
    std::optional<WindowInfo> result;

    withDisplay([&](Display* display) {
        auto active =
            longProperty(display, DefaultRootWindow(display), atom(display, "_NET_ACTIVE_WINDOW"), XA_WINDOW);
        if (active.empty() || !active[0]) return;

        auto window = static_cast<Window>(active[0]);
        if (!isIgnored(ignored, window)) result = describe(display, window, false);
    });

    return result;
}

std::vector<uint64_t> windowsOfProcesses(std::set<uint32_t> const& processes) {
    std::vector<uint64_t> windows;

    withDisplay([&](Display* display) {
        auto pid = atom(display, "_NET_WM_PID");

        for (auto window : stackedWindows(display, DefaultRootWindow(display))) {
            auto owner = longProperty(display, window, pid, XA_CARDINAL);
            if (owner.size() == 1 && processes.contains(static_cast<uint32_t>(owner[0])))
                windows.push_back(static_cast<uint64_t>(window));
        }
    });

    return windows;
}

}

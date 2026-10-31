#include "x11.h"

#include "linux.h"

#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <map>
#include <mutex>
#include <utility>

namespace fileship::core::x11 {

namespace {

std::mutex errorMutex;
std::map<Display*, int> errors;
std::atomic<XErrorHandler> previousHandler = nullptr;
thread_local bool isChaining = false;

std::mutex sharedMutex;
Display* shared = nullptr;

std::mutex atomMutex;
std::map<std::string, Atom, std::less<>> atoms;

int handleError(Display* display, XErrorEvent* event) {
    {
        std::lock_guard lock(errorMutex);
        if (auto found = errors.find(display); found != errors.end()) {
            if (!found->second) found->second = event->error_code;
            return 0;
        }
    }

    auto previous = previousHandler.load();
    if (!previous || isChaining) return 0;

    isChaining = true;
    int result = previous(display, event);
    isChaining = false;
    return result;
}

void installErrorHandler() {
    auto current = XSetErrorHandler(handleError);
    if (current != handleError) previousHandler = current;
}

double dpiScale(Display* display, Window root) {
    auto resources = "\n" + textProperty(display, root, XA_RESOURCE_MANAGER, XA_STRING);
    auto position = resources.find("\nXft.dpi:");
    if (position == std::string::npos) return 1;

    double dpi = std::strtod(resources.c_str() + position + 9, nullptr);
    return dpi >= 48 ? dpi / 96 : 1;
}

bool hasMonitors(Display* display) {
    int eventBase = 0;
    int errorBase = 0;
    int major = 0;
    int minor = 0;

    return XRRQueryExtension(display, &eventBase, &errorBase) && XRRQueryVersion(display, &major, &minor) &&
           (major > 1 || (major == 1 && minor >= 5));
}

}

Display* openDisplay() {
    static std::once_flag threads;
    std::call_once(threads, [] { XInitThreads(); });

    installErrorHandler();
    auto display = XOpenDisplay(nullptr);
    if (!display) return nullptr;

    std::lock_guard lock(errorMutex);
    errors[display] = 0;
    return display;
}

void closeDisplay(Display* display) {
    syncErrors(display);

    {
        std::lock_guard lock(errorMutex);
        errors.erase(display);
    }

    XCloseDisplay(display);
}

int syncErrors(Display* display) {
    installErrorHandler();
    XSync(display, False);

    std::lock_guard lock(errorMutex);
    return std::exchange(errors[display], 0);
}

bool withDisplay(std::function<void(Display*)> const& work) {
    std::lock_guard lock(sharedMutex);
    if (!shared) shared = openDisplay();
    if (!shared) return false;

    work(shared);
    syncErrors(shared);
    return true;
}

Atom atom(Display* display, char const* name) {
    {
        std::lock_guard lock(atomMutex);
        if (auto found = atoms.find(name); found != atoms.end()) return found->second;
    }

    auto value = XInternAtom(display, name, False);

    std::lock_guard lock(atomMutex);
    atoms[name] = value;
    return value;
}

std::string atomName(Display* display, Atom value) {
    auto name = value ? XGetAtomName(display, value) : nullptr;
    if (!name) return {};

    std::string result(name);
    XFree(name);
    return result;
}

std::vector<long> longProperty(Display* display, Window window, Atom property, Atom type) {
    Atom actualType = None;
    int format = 0;
    unsigned long count = 0;
    unsigned long remaining = 0;
    unsigned char* data = nullptr;

    if (XGetWindowProperty(display, window, property, 0, 1 << 16, False, type, &actualType, &format, &count,
                           &remaining, &data) != Success ||
        !data)
        return {};

    std::vector<long> values;
    if (format == 32) values.assign(reinterpret_cast<long*>(data), reinterpret_cast<long*>(data) + count);
    XFree(data);
    return values;
}

std::string textProperty(Display* display, Window window, Atom property, Atom type) {
    Atom actualType = None;
    int format = 0;
    unsigned long count = 0;
    unsigned long remaining = 0;
    unsigned char* data = nullptr;

    if (XGetWindowProperty(display, window, property, 0, 1 << 22, False, type, &actualType, &format, &count,
                           &remaining, &data) != Success ||
        !data)
        return {};

    std::string text;
    if (format == 8) text.assign(reinterpret_cast<char*>(data), count);
    XFree(data);
    return text;
}

Rect rootRect(Display* display) {
    Window root = None;
    int x = 0;
    int y = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int border = 0;
    unsigned int depth = 0;

    XGetGeometry(display, DefaultRootWindow(display), &root, &x, &y, &width, &height, &border, &depth);
    return {0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height)};
}

std::vector<DisplayInfo> listDisplays() {
    std::vector<DisplayInfo> displays;

    withDisplay([&](Display* display) {
        auto root = DefaultRootWindow(display);
        auto scale = dpiScale(display, root);

        if (hasMonitors(display)) {
            int count = 0;
            auto monitors = XRRGetMonitors(display, root, True, &count);

            for (int index = 0; index < count; index++) {
                auto const& monitor = monitors[index];
                displays.push_back(DisplayInfo{
                    displayId(atomName(display, monitor.name)),
                    Rect{monitor.x, monitor.y, monitor.width, monitor.height},
                    scale,
                    monitor.primary != 0,
                });
            }

            if (monitors) XRRFreeMonitors(monitors);
        }

        if (displays.empty()) displays.push_back(DisplayInfo{0, rootRect(display), scale, true});
    });

    if (!displays.empty() &&
        std::none_of(displays.begin(), displays.end(), [](DisplayInfo const& display) { return display.isPrimary; }))
        displays.front().isPrimary = true;

    return displays;
}

std::optional<Point> cursorPosition() {
    std::optional<Point> position;

    withDisplay([&](Display* display) {
        Window root = None;
        Window child = None;
        int rootX = 0;
        int rootY = 0;
        int windowX = 0;
        int windowY = 0;
        unsigned int mask = 0;

        if (XQueryPointer(display, DefaultRootWindow(display), &root, &child, &rootX, &rootY, &windowX, &windowY,
                          &mask))
            position = Point{rootX, rootY};
    });

    return position;
}

}

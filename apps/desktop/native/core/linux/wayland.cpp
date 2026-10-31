#include "linux.h"

#include <wayland-client.h>
#include <wayland/xdg-output-unstable-v1-client.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>

namespace fileship::core::wayland {

namespace {

struct Output {
    wl_output* output = nullptr;
    uint32_t version = 1;
    zxdg_output_v1* xdgOutput = nullptr;
    std::string name;
    Rect geometry;
    int32_t transform = 0;
    int32_t scale = 1;
    std::optional<Point> logicalPosition;
    std::optional<Point> logicalSize;
};

struct State {
    std::vector<std::unique_ptr<Output>> outputs;
    zxdg_output_manager_v1* manager = nullptr;
};

wl_output_listener const outputListener{
    [](void* data, wl_output*, int32_t x, int32_t y, int32_t, int32_t, int32_t, char const*, char const*,
       int32_t transform) {
        auto& output = *static_cast<Output*>(data);
        output.geometry.x = x;
        output.geometry.y = y;
        output.transform = transform;
    },
    [](void* data, wl_output*, uint32_t flags, int32_t width, int32_t height, int32_t) {
        if (!(flags & WL_OUTPUT_MODE_CURRENT)) return;

        auto& output = *static_cast<Output*>(data);
        output.geometry.width = width;
        output.geometry.height = height;
    },
    [](void*, wl_output*) {},
    [](void* data, wl_output*, int32_t factor) { static_cast<Output*>(data)->scale = std::max(factor, 1); },
    [](void* data, wl_output*, char const* name) { static_cast<Output*>(data)->name = name; },
    [](void*, wl_output*, char const*) {},
};

zxdg_output_v1_listener const xdgOutputListener{
    [](void* data, zxdg_output_v1*, int32_t x, int32_t y) {
        static_cast<Output*>(data)->logicalPosition = Point{x, y};
    },
    [](void* data, zxdg_output_v1*, int32_t width, int32_t height) {
        static_cast<Output*>(data)->logicalSize = Point{width, height};
    },
    [](void*, zxdg_output_v1*) {},
    [](void* data, zxdg_output_v1*, char const* name) {
        auto& output = *static_cast<Output*>(data);
        if (output.name.empty()) output.name = name;
    },
    [](void*, zxdg_output_v1*, char const*) {},
};

wl_registry_listener const registryListener{
    [](void* data, wl_registry* registry, uint32_t name, char const* interface, uint32_t version) {
        auto& state = *static_cast<State*>(data);

        if (!std::strcmp(interface, wl_output_interface.name)) {
            auto output = std::make_unique<Output>();
            output->version = std::min(version, 4u);
            output->output =
                static_cast<wl_output*>(wl_registry_bind(registry, name, &wl_output_interface, output->version));
            wl_output_add_listener(output->output, &outputListener, output.get());
            state.outputs.push_back(std::move(output));
        } else if (!std::strcmp(interface, zxdg_output_manager_v1_interface.name)) {
            state.manager = static_cast<zxdg_output_manager_v1*>(
                wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface, std::min(version, 3u)));
        }
    },
    [](void*, wl_registry*, uint32_t) {},
};

Rect logicalRect(Output const& output) {
    if (output.logicalPosition && output.logicalSize)
        return {output.logicalPosition->x, output.logicalPosition->y, output.logicalSize->x, output.logicalSize->y};

    bool isTurned = output.transform & 1;
    int32_t width = isTurned ? output.geometry.height : output.geometry.width;
    int32_t height = isTurned ? output.geometry.width : output.geometry.height;
    return {output.geometry.x, output.geometry.y, width / output.scale, height / output.scale};
}

double scaleOf(Output const& output, Rect const& logical) {
    bool isTurned = output.transform & 1;
    int32_t width = isTurned ? output.geometry.height : output.geometry.width;
    if (width <= 0 || logical.width <= 0) return output.scale;
    return static_cast<double>(width) / logical.width;
}

}

std::vector<DisplayInfo> listDisplays() {
    auto display = wl_display_connect(nullptr);
    if (!display) return {};

    State state;
    auto registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registryListener, &state);
    wl_display_roundtrip(display);

    if (state.manager) {
        for (auto& output : state.outputs) {
            output->xdgOutput = zxdg_output_manager_v1_get_xdg_output(state.manager, output->output);
            zxdg_output_v1_add_listener(output->xdgOutput, &xdgOutputListener, output.get());
        }
    }

    wl_display_roundtrip(display);
    wl_display_roundtrip(display);

    double factor = 1;
    std::vector<std::pair<Rect, std::string>> logical;

    for (auto const& output : state.outputs) {
        auto rect = logicalRect(*output);
        if (rect.width <= 0 || rect.height <= 0) continue;

        factor = std::max(factor, scaleOf(*output, rect));
        logical.emplace_back(rect, output->name);
    }

    std::vector<DisplayInfo> displays;
    for (auto const& [rect, name] : logical) {
        displays.push_back(DisplayInfo{
            displayId(name),
            Rect{static_cast<int32_t>(std::lround(rect.x * factor)), static_cast<int32_t>(std::lround(rect.y * factor)),
                 static_cast<int32_t>(std::lround(rect.width * factor)),
                 static_cast<int32_t>(std::lround(rect.height * factor))},
            factor,
            rect.x <= 0 && rect.y <= 0 && rect.x + rect.width > 0 && rect.y + rect.height > 0,
        });
    }

    if (!displays.empty() &&
        std::none_of(displays.begin(), displays.end(), [](DisplayInfo const& info) { return info.isPrimary; }))
        displays.front().isPrimary = true;

    for (auto& output : state.outputs) {
        if (output->xdgOutput) zxdg_output_v1_destroy(output->xdgOutput);
        if (output->version >= 3) wl_output_release(output->output);
        else wl_output_destroy(output->output);
    }

    if (state.manager) zxdg_output_manager_v1_destroy(state.manager);
    wl_registry_destroy(registry);
    wl_display_disconnect(display);
    return displays;
}

}

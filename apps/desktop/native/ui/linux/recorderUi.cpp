#include "cairo.h"
#include "recorderPlacement.h"
#include "recorderWindows.h"

#include <ui/fixture.h>
#include <ui/recorderToolbar.h>
#include <ui/uiThread.h>

#include <glib-unix.h>
#include <xcb/shape.h>
#include <xcb/xcb.h>

#include <algorithm>
#include <cmath>

namespace fileship::ui {

namespace {

constexpr guint FRAME_MS = 33;

uint32_t pixelOf(Color const& value) {
    auto channel = [](float component) { return static_cast<uint32_t>(std::lround(component * 255)); };
    return channel(value.r) << 16 | channel(value.g) << 8 | channel(value.b);
}

class RecorderWindows {
public:
    RecorderWindows(ScreenRect const& area, ScreenRect const& place, double scale, ThemeMode theme,
                    RecorderUiCallbacks callbacks, bool showBorder)
        : resources(std::static_pointer_cast<CairoResources>(Resources::acquire())),
          bar(*resources, theme, std::move(callbacks)),
          width(place.width),
          height(place.height) {
        bar.scale = scale;
        bar.state.phase = RecorderPhase::Preparing;

        connection = xcb_connect(nullptr, nullptr);
        screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;

        if (showBorder) createBorders(area);

        uint32_t values[] = {1, XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_LEAVE_WINDOW |
                                    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE};
        toolbar = xcb_generate_id(connection);
        xcb_create_window(connection, XCB_COPY_FROM_PARENT, toolbar, screen->root, static_cast<int16_t>(place.x),
                          static_cast<int16_t>(place.y), static_cast<uint16_t>(width), static_cast<uint16_t>(height), 0,
                          XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                          XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK, values);
        context = xcb_generate_id(connection);
        xcb_create_gc(connection, context, toolbar, 0, nullptr);

        shape();
        xcb_map_window(connection, toolbar);
        render();

        watch = g_unix_fd_add(xcb_get_file_descriptor(connection), G_IO_IN, handleEvents, this);
        timer = g_timeout_add(FRAME_MS, handleTimer, this);
    }

    ~RecorderWindows() {
        g_source_remove(timer);
        g_source_remove(watch);
        xcb_disconnect(connection);
    }

    void update(RecorderViewState const& state) {
        bar.state = state;
        render();
    }

private:
    void createBorders(ScreenRect const& area) {
        ScreenRect bounds{0, 0, screen->width_in_pixels, screen->height_in_pixels};
        uint32_t values[] = {pixelOf(bar.borderColor()), 1};

        for (auto const& side : recorderBorders(area, bounds, bar.scale)) {
            bool isHorizontal = side.width > side.height;
            int line = isHorizontal ? side.height : side.width;
            int length = isHorizontal ? side.width : side.height;
            int dash = line * 3;
            std::vector<xcb_rectangle_t> dashes;

            for (int start = 0; start < length; start += dash * 2) {
                auto size = static_cast<uint16_t>(std::min(dash, length - start));
                auto offset = static_cast<int16_t>(start);
                auto thickness = static_cast<uint16_t>(line);
                dashes.push_back(isHorizontal ? xcb_rectangle_t{offset, 0, size, thickness}
                                              : xcb_rectangle_t{0, offset, thickness, size});
            }

            auto window = xcb_generate_id(connection);
            xcb_create_window(connection, XCB_COPY_FROM_PARENT, window, screen->root, static_cast<int16_t>(side.x),
                              static_cast<int16_t>(side.y), static_cast<uint16_t>(side.width),
                              static_cast<uint16_t>(side.height), 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                              XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT, values);
            xcb_shape_rectangles(connection, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_BOUNDING, XCB_CLIP_ORDERING_UNSORTED, window,
                                 0, 0, static_cast<uint32_t>(dashes.size()), dashes.data());
            xcb_map_window(connection, window);
        }
    }

    void shape() {
        auto pill = bar.pill();
        if (pill.left == shaped.left && pill.right == shaped.right) return;
        shaped = pill;

        double top = std::round(pill.top * bar.scale);
        double bottom = std::round(pill.bottom * bar.scale);
        double left = pill.left * bar.scale;
        double right = pill.right * bar.scale;
        double radius = (bottom - top) / 2;
        std::vector<xcb_rectangle_t> rows;

        for (double row = top; row < bottom; row++) {
            double distance = row + 0.5 - (top + radius);
            double inset = radius - std::sqrt(radius * radius - distance * distance);
            auto start = static_cast<int16_t>(std::floor(left + inset));
            auto end = static_cast<int16_t>(std::ceil(right - inset));
            rows.push_back({start, static_cast<int16_t>(row), static_cast<uint16_t>(end - start), 1});
        }

        xcb_shape_rectangles(connection, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_BOUNDING, XCB_CLIP_ORDERING_Y_SORTED, toolbar, 0,
                             0, static_cast<uint32_t>(rows.size()), rows.data());
    }

    void render() {
        lastFrame = nowMs();
        shape();

        auto surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
        auto cr = cairo_create(surface);
        {
            CairoPainter painter(cr, *resources);
            painter.clear(bar.surfaceColor());
            bar.paint(painter, lastFrame);
        }
        cairo_destroy(cr);
        cairo_surface_flush(surface);

        xcb_put_image(connection, XCB_IMAGE_FORMAT_Z_PIXMAP, toolbar, context, static_cast<uint16_t>(width),
                      static_cast<uint16_t>(height), 0, 0, 0, screen->root_depth,
                      static_cast<uint32_t>(cairo_image_surface_get_stride(surface) * height),
                      cairo_image_surface_get_data(surface));
        xcb_flush(connection);
        cairo_surface_destroy(surface);
    }

    void handle(xcb_generic_event_t* event) {
        switch (event->response_type & 0x7f) {
        case XCB_MOTION_NOTIFY: {
            auto motion = reinterpret_cast<xcb_motion_notify_event_t*>(event);
            bar.pointerMove(motion->event_x, motion->event_y);
            break;
        }
        case XCB_LEAVE_NOTIFY:
            bar.pointerLeave();
            break;
        case XCB_BUTTON_PRESS: {
            auto press = reinterpret_cast<xcb_button_press_event_t*>(event);
            if (press->detail == XCB_BUTTON_INDEX_1) bar.pointerDown(press->event_x, press->event_y);
            break;
        }
        case XCB_BUTTON_RELEASE: {
            auto release = reinterpret_cast<xcb_button_release_event_t*>(event);
            if (release->detail == XCB_BUTTON_INDEX_1) bar.pointerUp(release->event_x, release->event_y);
            break;
        }
        default:
            break;
        }
    }

    bool readEvents() {
        bool hasEvents = false;

        while (auto event = xcb_poll_for_event(connection)) {
            handle(event);
            free(event);
            hasEvents = true;
        }

        return hasEvents;
    }

    static gboolean handleEvents(gint, GIOCondition, gpointer data) {
        auto self = static_cast<RecorderWindows*>(data);
        if (self->readEvents()) self->render();
        return G_SOURCE_CONTINUE;
    }

    static gboolean handleTimer(gpointer data) {
        auto self = static_cast<RecorderWindows*>(data);
        double now = nowMs();
        if (self->readEvents() || self->bar.isAnimating(now) || now - self->lastFrame >= 250) self->render();
        return G_SOURCE_CONTINUE;
    }

    std::shared_ptr<CairoResources> resources;
    RecorderToolbar bar;
    xcb_connection_t* connection = nullptr;
    xcb_screen_t* screen = nullptr;
    xcb_window_t toolbar = 0;
    xcb_gcontext_t context = 0;
    RectF shaped{};
    int width;
    int height;
    guint watch = 0;
    guint timer = 0;
    double lastFrame = 0;
};

std::unique_ptr<RecorderWindows> current;

}

FixtureImage renderRecorderFixture(RecorderViewState const& state, ThemeMode theme, double scale) {
    auto resources = Resources::acquire();
    auto canvas = Canvas::create(*resources);
    RecorderToolbar bar(*resources, theme, {});
    bar.state = state;
    bar.scale = scale;

    FixtureImage image;
    image.width = static_cast<int>(std::round(RECORDER_WIDTH * scale));
    image.height = static_cast<int>(std::round(RECORDER_HEIGHT * scale));
    image.bgra = canvas->render(image.width, image.height, [&](Painter& painter) { bar.paint(painter, nowMs()); });
    return image;
}

void showRecorderWindows(ScreenRect area, ScreenRect toolbar, double scale, ThemeMode theme,
                         RecorderUiCallbacks callbacks, bool showBorder) {
    runOnUi([area, toolbar, scale, theme, callbacks = std::move(callbacks), showBorder]() mutable {
        current = std::make_unique<RecorderWindows>(area, toolbar, scale, theme, std::move(callbacks), showBorder);
    });
}

void updateRecorderUi(RecorderViewState const& state) {
    runOnUi([state] {
        if (current) current->update(state);
    });
}

void hideRecorderUi() {
    runOnUi([] { current.reset(); });
}

}

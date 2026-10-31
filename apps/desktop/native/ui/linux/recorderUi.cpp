#define GDK_DISABLE_DEPRECATION_WARNINGS

#include "cairo.h"
#include "gtk.h"

#include <ui/recorderToolbar.h>
#include <ui/uiThread.h>

#include <shared/log.h>

#include <gdk/x11/gdkx.h>
#include <gtk/gtk.h>

#include <algorithm>
#include <cmath>

namespace fileship::ui {

namespace {

constexpr char const* TRANSPARENT_STYLE = "window.fileship-recorder { background: transparent; }";

struct MonitorPart {
    ScreenRect bounds;
    ScreenRect work;
    double scale = 1;
    ScreenRect part;
    std::wstring sides;
};

std::optional<ScreenRect> intersect(ScreenRect const& a, ScreenRect const& b) {
    int left = std::max(a.x, b.x);
    int top = std::max(a.y, b.y);
    int right = std::min(a.x + a.width, b.x + b.width);
    int bottom = std::min(a.y + a.height, b.y + b.height);
    if (right <= left || bottom <= top) return std::nullopt;
    return ScreenRect{left, top, right - left, bottom - top};
}

bool isX11() {
    return GDK_IS_X11_DISPLAY(gdk_display_get_default());
}

void addStyle() {
    static bool isAdded = false;
    if (isAdded) return;

    auto provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, TRANSPARENT_STYLE);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    isAdded = true;
}

std::vector<MonitorPart> monitorParts(ScreenRect const& area) {
    std::vector<MonitorPart> parts;
    auto monitors = gdk_display_get_monitors(gdk_display_get_default());

    for (guint index = 0; index < g_list_model_get_n_items(monitors); index++) {
        auto monitor = GDK_MONITOR(g_list_model_get_item(monitors, index));
        GdkRectangle geometry{};
        gdk_monitor_get_geometry(monitor, &geometry);
        double scale = gdk_monitor_get_scale(monitor);

        GdkRectangle work = geometry;
        if (GDK_IS_X11_MONITOR(monitor)) gdk_x11_monitor_get_workarea(monitor, &work);
        g_object_unref(monitor);

        auto physical = [&](GdkRectangle const& rect) {
            return ScreenRect{static_cast<int>(std::lround(rect.x * scale)), static_cast<int>(std::lround(rect.y * scale)),
                              static_cast<int>(std::lround(rect.width * scale)),
                              static_cast<int>(std::lround(rect.height * scale))};
        };

        MonitorPart item;
        item.bounds = physical(geometry);
        item.work = physical(work);
        item.scale = scale;
        auto part = intersect(item.bounds, area);
        if (!part) continue;

        item.part = *part;
        if (part->y == area.y) item.sides += L't';
        if (part->x + part->width == area.x + area.width) item.sides += L'r';
        if (part->y + part->height == area.y + area.height) item.sides += L'b';
        if (part->x == area.x) item.sides += L'l';
        parts.push_back(item);
    }

    return parts;
}

class Popup {
public:
    Popup(ScreenRect const& rect, double scale, bool isClickThrough) : rect(rect), scale(scale) {
        window = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(window), "Fileship");
        gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
        gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
        gtk_widget_add_css_class(window, "fileship-recorder");
        gtk_window_set_default_size(GTK_WINDOW(window), std::max(1, static_cast<int>(std::lround(rect.width / scale))),
                                    std::max(1, static_cast<int>(std::lround(rect.height / scale))));

        area = gtk_drawing_area_new();
        gtk_window_set_child(GTK_WINDOW(window), area);
        gtk_widget_realize(window);

        auto surface = gtk_native_get_surface(GTK_NATIVE(window));
        if (isClickThrough) {
            auto empty = cairo_region_create();
            gdk_surface_set_input_region(surface, empty);
            cairo_region_destroy(empty);
        }

        if (GDK_IS_X11_SURFACE(surface)) {
            auto display = gdk_x11_display_get_xdisplay(gdk_surface_get_display(surface));
            auto xid = gdk_x11_surface_get_xid(surface);
            XSetWindowAttributes attributes{};
            attributes.override_redirect = True;
            XChangeWindowAttributes(display, xid, CWOverrideRedirect, &attributes);
            XMoveWindow(display, xid, rect.x, rect.y);
        }
    }

    ~Popup() {
        if (window) gtk_window_destroy(GTK_WINDOW(window));
    }

    Popup(Popup const&) = delete;
    Popup& operator=(Popup const&) = delete;

    void show() {
        gtk_window_present(GTK_WINDOW(window));

        auto surface = gtk_native_get_surface(GTK_NATIVE(window));
        if (GDK_IS_X11_SURFACE(surface)) {
            auto display = gdk_x11_display_get_xdisplay(gdk_surface_get_display(surface));
            auto xid = gdk_x11_surface_get_xid(surface);
            XMoveWindow(display, xid, rect.x, rect.y);
            XRaiseWindow(display, xid);
            XFlush(display);
        }
    }

    void setInput(RectF const& pill) {
        auto surface = gtk_native_get_surface(GTK_NATIVE(window));
        cairo_rectangle_int_t box{static_cast<int>(std::floor(pill.left / scale)),
                                  static_cast<int>(std::floor(pill.top / scale)),
                                  static_cast<int>(std::ceil((pill.right - pill.left) / scale)),
                                  static_cast<int>(std::ceil((pill.bottom - pill.top) / scale))};
        auto region = cairo_region_create_rectangle(&box);
        gdk_surface_set_input_region(surface, region);
        cairo_region_destroy(region);
    }

    ScreenRect rect;
    double scale = 1;
    GtkWidget* window = nullptr;
    GtkWidget* area = nullptr;
};

class RecorderUi {
public:
    RecorderUi(ScreenRect area, ThemeMode theme, RecorderUiCallbacks value, bool showBorder)
        : toolbar(theme, std::move(value)) {
        addStyle();

        auto parts = monitorParts(area);
        MonitorPart const* host = nullptr;
        for (auto const& part : parts) {
            auto areaOf = [](ScreenRect const& rect) { return static_cast<int64_t>(rect.width) * rect.height; };
            if (!host || areaOf(part.part) > areaOf(host->part)) host = &part;
        }

        if (showBorder && isX11()) {
            for (auto const& part : parts) createBorders(part);
        }

        createToolbar(host, area);
    }

    ~RecorderUi() {
        if (frame) g_source_remove(frame);
    }

    void update(RecorderViewState const& value) {
        toolbar.setState(value);
        redraw();
    }

private:
    void createBorders(MonitorPart const& part) {
        if (part.sides.empty()) return;

        auto pad = [&](wchar_t side) {
            return part.sides.find(side) != std::wstring::npos
                       ? static_cast<int>(std::lround(RECORDER_BORDER_PADDING * part.scale))
                       : 0;
        };
        int left = std::max(part.part.x - pad(L'l'), part.bounds.x);
        int top = std::max(part.part.y - pad(L't'), part.bounds.y);
        int right = std::min(part.part.x + part.part.width + pad(L'r'), part.bounds.x + part.bounds.width);
        int bottom = std::min(part.part.y + part.part.height + pad(L'b'), part.bounds.y + part.bounds.height);
        int width = std::max(1, static_cast<int>(std::lround(RECORDER_BORDER_WIDTH * part.scale)));

        for (wchar_t side : part.sides) {
            ScreenRect rect{left, top, right - left, bottom - top};
            if (side == L't') rect.height = width;
            if (side == L'b') rect = {left, bottom - width, right - left, width};
            if (side == L'l') rect.width = width;
            if (side == L'r') rect = {right - width, top, width, bottom - top};

            auto border = std::make_unique<Border>();
            border->owner = this;
            border->isHorizontal = side == L't' || side == L'b';
            border->offset = border->isHorizontal ? static_cast<float>(rect.x - left) : static_cast<float>(rect.y - top);
            border->dash = static_cast<float>(width * 3);
            border->popup = std::make_unique<Popup>(rect, part.scale, true);

            gtk_drawing_area_set_draw_func(
                GTK_DRAWING_AREA(border->popup->area),
                [](GtkDrawingArea*, cairo_t* context, int width, int height, gpointer data) {
                    auto& border = *static_cast<Border*>(data);
                    auto& toolbar = border.owner->toolbar;
                    auto painter = createPainter(context, toolbar.resources());
                    paintRecorderBorder(*painter, color(toolbar.palette().theme.destructive), border.isHorizontal,
                                        width, height, border.offset, border.dash);
                },
                border.get(), nullptr);

            border->popup->show();
            borders.push_back(std::move(border));
        }
    }

    void createToolbar(MonitorPart const* host, ScreenRect const& area) {
        auto part = host ? host->part : area;
        auto work = host ? host->work : area;
        double scale = host ? host->scale : 1;

        double width = std::round(RECORDER_TOOLBAR_WIDTH * scale);
        double height = std::round(RECORDER_TOOLBAR_HEIGHT * scale);
        double margin = 8 * scale;
        double centerX = part.x + part.width / 2.0;
        double x = std::round(std::min(std::max(centerX - width / 2, static_cast<double>(work.x)),
                                       work.x + work.width - width));
        double y = part.y + part.height + margin;
        if (y + height > work.y + work.height) y = part.y - height - margin;
        if (y < work.y) y = part.y + part.height - height - margin;

        ScreenRect rect{static_cast<int>(x), static_cast<int>(std::round(y)), static_cast<int>(width),
                        static_cast<int>(height)};
        window = std::make_unique<Popup>(rect, scale, false);

        gtk_drawing_area_set_draw_func(
            GTK_DRAWING_AREA(window->area),
            [](GtkDrawingArea*, cairo_t* context, int, int, gpointer data) {
                auto self = static_cast<RecorderUi*>(data);
                auto painter = createPainter(context, self->toolbar.resources());
                self->toolbar.paint(*painter, nowMs());
            },
            this, nullptr);

        auto motion = gtk_event_controller_motion_new();
        g_signal_connect(motion, "motion", G_CALLBACK(+[](GtkEventControllerMotion*, double x, double y, gpointer data) {
                             auto self = static_cast<RecorderUi*>(data);
                             self->toolbar.pointerMove(x, y);
                             self->redraw();
                         }),
                         this);
        g_signal_connect(motion, "leave", G_CALLBACK(+[](GtkEventControllerMotion*, gpointer data) {
                             auto self = static_cast<RecorderUi*>(data);
                             self->toolbar.pointerLeave();
                             self->redraw();
                         }),
                         this);
        gtk_widget_add_controller(window->area, motion);

        auto click = gtk_gesture_click_new();
        g_signal_connect(click, "pressed", G_CALLBACK(+[](GtkGestureClick*, int, double x, double y, gpointer data) {
                             auto self = static_cast<RecorderUi*>(data);
                             self->toolbar.pointerDown(x, y);
                             self->redraw();
                         }),
                         this);
        g_signal_connect(click, "released", G_CALLBACK(+[](GtkGestureClick*, int, double x, double y, gpointer data) {
                             auto self = static_cast<RecorderUi*>(data);
                             self->toolbar.pointerUp(x, y);
                             self->redraw();
                         }),
                         this);
        gtk_widget_add_controller(window->area, GTK_EVENT_CONTROLLER(click));

        window->setInput(toolbar.pill());
        window->show();
        toolbar.notifyVisible();

        frame = g_timeout_add(16, [](gpointer data) -> gboolean {
            auto self = static_cast<RecorderUi*>(data);
            double now = nowMs();
            if (self->toolbar.isAnimating(now) || now - self->lastFrame >= 250) {
                self->lastFrame = now;
                self->redraw();
            }
            return G_SOURCE_CONTINUE;
        }, this);
    }

    void redraw() {
        if (!window) return;
        window->setInput(toolbar.pill());
        gtk_widget_queue_draw(window->area);
    }

    struct Border {
        RecorderUi* owner = nullptr;
        bool isHorizontal = false;
        float offset = 0;
        float dash = 0;
        std::unique_ptr<Popup> popup;
    };

    RecorderToolbar toolbar;
    std::unique_ptr<Popup> window;
    std::vector<std::unique_ptr<Border>> borders;
    guint frame = 0;
    double lastFrame = 0;
};

std::unique_ptr<RecorderUi> current;

}

void showRecorderUi(ScreenRect area, ThemeMode theme, RecorderUiCallbacks callbacks, bool showBorder) {
    runOnUi([area, theme, callbacks = std::move(callbacks), showBorder]() mutable {
        current.reset();
        if (!initGtk()) {
            logError(L"Could not show the recorder toolbar: GTK is not available");
            return;
        }

        current = std::make_unique<RecorderUi>(area, theme, std::move(callbacks), showBorder);
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

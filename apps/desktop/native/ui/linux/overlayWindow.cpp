#include "cairo.h"
#include "gtk.h"

#include <ui/overlayController.h>
#include <shared/log.h>
#include <shared/system.h>
#include <shared/utf8.h>

#include <gtk/gtk.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace fileship::ui {

namespace {

Key keyOf(guint keyval) {
    if (keyval >= GDK_KEY_0 && keyval <= GDK_KEY_9)
        return static_cast<Key>(static_cast<int>(Key::Digit0) + static_cast<int>(keyval - GDK_KEY_0));
    if (keyval >= GDK_KEY_KP_0 && keyval <= GDK_KEY_KP_9)
        return static_cast<Key>(static_cast<int>(Key::Digit0) + static_cast<int>(keyval - GDK_KEY_KP_0));

    switch (gdk_keyval_to_lower(keyval)) {
    case GDK_KEY_Escape:
        return Key::Escape;
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
        return Key::Enter;
    case GDK_KEY_space:
        return Key::Space;
    case GDK_KEY_Tab:
        return Key::Tab;
    case GDK_KEY_BackSpace:
        return Key::Backspace;
    case GDK_KEY_Delete:
    case GDK_KEY_KP_Delete:
        return Key::Delete;
    case GDK_KEY_Left:
        return Key::Left;
    case GDK_KEY_Right:
        return Key::Right;
    case GDK_KEY_Up:
        return Key::Up;
    case GDK_KEY_Down:
        return Key::Down;
    case GDK_KEY_Home:
        return Key::Home;
    case GDK_KEY_End:
        return Key::End;
    case GDK_KEY_a:
        return Key::A;
    case GDK_KEY_c:
        return Key::C;
    case GDK_KEY_v:
        return Key::V;
    case GDK_KEY_x:
        return Key::X;
    case GDK_KEY_y:
        return Key::Y;
    case GDK_KEY_z:
        return Key::Z;
    default:
        return Key::Unknown;
    }
}

Modifiers modifiersOf(GdkModifierType state) {
    return {(state & GDK_CONTROL_MASK) != 0, (state & GDK_SHIFT_MASK) != 0, (state & GDK_ALT_MASK) != 0};
}

char const* cursorName(CursorKind kind) {
    switch (kind) {
    case CursorKind::Arrow:
        return "default";
    case CursorKind::Hand:
        return "pointer";
    case CursorKind::IBeam:
        return "text";
    case CursorKind::Move:
        return "move";
    case CursorKind::ResizeNwse:
        return "nwse-resize";
    case CursorKind::ResizeNesw:
        return "nesw-resize";
    case CursorKind::ResizeNs:
        return "ns-resize";
    case CursorKind::ResizeWe:
        return "ew-resize";
    default:
        return "crosshair";
    }
}

template <typename Result> Result waitFor(std::function<void(GAsyncReadyCallback, gpointer)> start,
                                          std::function<Result(GObject*, GAsyncResult*)> finish) {
    struct State {
        GMainLoop* loop = g_main_loop_new(nullptr, FALSE);
        std::function<Result(GObject*, GAsyncResult*)> finish;
        Result result{};
    } state{.finish = std::move(finish)};

    start(
        [](GObject* source, GAsyncResult* result, gpointer data) {
            auto& state = *static_cast<State*>(data);
            state.result = state.finish(source, result);
            g_main_loop_quit(state.loop);
        },
        &state);

    g_main_loop_run(state.loop);
    g_main_loop_unref(state.loop);
    return std::move(state.result);
}

class GtkOverlayWindow : public OverlayWindow {
public:
    explicit GtkOverlayWindow(OverlayController& controller) : controller(controller) {}

    ~GtkOverlayWindow() override {
        destroy();
    }

    bool open() override {
        if (!initGtk()) return false;

        auto display = gdk_display_get_default();
        auto monitors = gdk_display_get_monitors(display);
        auto rects = controller.displayRects();
        if (rects.empty()) rects.push_back(controller.screenBounds());

        im = gtk_im_multicontext_new();
        g_signal_connect(im, "commit", G_CALLBACK(+[](GtkIMContext*, char* text, gpointer data) {
                             auto self = static_cast<GtkOverlayWindow*>(data);
                             self->controller.commit(toWide(text));
                             self->controller.afterInput();
                         }),
                         this);
        g_signal_connect(im, "preedit-changed", G_CALLBACK(+[](GtkIMContext* context, gpointer data) {
                             auto self = static_cast<GtkOverlayWindow*>(data);
                             char* text = nullptr;
                             gtk_im_context_get_preedit_string(context, &text, nullptr, nullptr);
                             self->controller.composition(toWide(text ? text : ""));
                             g_free(text);
                             self->caretMoved();
                             self->invalidate();
                         }),
                         this);

        for (auto const& rect : rects) {
            auto surface = std::make_unique<Surface>();
            surface->owner = this;
            surface->rect = rect;
            surface->monitor = monitorFor(monitors, rect);

            surface->window = gtk_window_new();
            gtk_window_set_title(GTK_WINDOW(surface->window), "Fileship");
            gtk_window_set_decorated(GTK_WINDOW(surface->window), FALSE);

            surface->picture = gtk_picture_new();
            gtk_picture_set_content_fit(GTK_PICTURE(surface->picture), GTK_CONTENT_FIT_FILL);
            gtk_picture_set_can_shrink(GTK_PICTURE(surface->picture), TRUE);
            gtk_widget_set_focusable(surface->picture, TRUE);
            gtk_window_set_child(GTK_WINDOW(surface->window), surface->picture);

            auto events = gtk_event_controller_legacy_new();
            g_signal_connect(events, "event", G_CALLBACK(handleEvent), surface.get());
            gtk_widget_add_controller(surface->picture, events);

            surface->keys = gtk_event_controller_key_new();
            g_signal_connect(surface->keys, "key-pressed", G_CALLBACK(handleKey), surface.get());
            gtk_widget_add_controller(surface->window, surface->keys);

            g_signal_connect(surface->window, "close-request", G_CALLBACK(+[](GtkWindow*, gpointer data) {
                                 static_cast<Surface*>(data)->owner->controller.close();
                                 return TRUE;
                             }),
                             surface.get());

            surfaces.push_back(std::move(surface));
        }

        for (auto& surface : surfaces) paint(*surface);

        Surface* primary = surfaces.front().get();
        for (auto& surface : surfaces) {
            if (surface->monitor) gtk_window_fullscreen_on_monitor(GTK_WINDOW(surface->window), surface->monitor);
            else gtk_window_fullscreen(GTK_WINDOW(surface->window));
            if (surface.get() != primary) gtk_window_present(GTK_WINDOW(surface->window));
        }
        gtk_window_present(GTK_WINDOW(primary->window));
        gtk_widget_grab_focus(primary->picture);
        return true;
    }

    void destroy() override {
        if (frame) g_source_remove(frame);
        if (caretTimer) g_source_remove(caretTimer);
        frame = 0;
        caretTimer = 0;

        for (auto& surface : surfaces) gtk_window_destroy(GTK_WINDOW(surface->window));
        surfaces.clear();

        if (im) g_object_unref(im);
        im = nullptr;
    }

    void invalidate() override {
        if (frame || surfaces.empty()) return;

        frame = g_idle_add(
            [](gpointer data) {
                auto self = static_cast<GtkOverlayWindow*>(data);
                self->frame = 0;
                for (auto& surface : self->surfaces) self->paint(*surface);
                if (self->controller.isAnimating()) {
                    self->frame = g_timeout_add(16, [](gpointer data) {
                        auto self = static_cast<GtkOverlayWindow*>(data);
                        self->frame = 0;
                        self->invalidate();
                        return G_SOURCE_REMOVE;
                    }, self);
                }
                return G_SOURCE_REMOVE;
            },
            this);
    }

    void textInputChanged(bool isEditing) override {
        for (auto& surface : surfaces)
            gtk_event_controller_key_set_im_context(GTK_EVENT_CONTROLLER_KEY(surface->keys), isEditing ? im : nullptr);

        if (isEditing) {
            if (!surfaces.empty()) gtk_im_context_set_client_widget(im, surfaces.front()->picture);
            gtk_im_context_focus_in(im);
            caretMoved();
            caretTimer = g_timeout_add(50, [](gpointer data) -> gboolean {
                auto self = static_cast<GtkOverlayWindow*>(data);
                bool visible = self->controller.caretVisible();
                if (visible != self->wasCaretVisible) {
                    self->wasCaretVisible = visible;
                    self->invalidate();
                }
                return G_SOURCE_CONTINUE;
            }, this);
        } else {
            gtk_im_context_focus_out(im);
            gtk_im_context_set_client_widget(im, nullptr);
            if (caretTimer) g_source_remove(caretTimer);
            caretTimer = 0;
        }
    }

    void caretMoved() override {
        auto caret = controller.caretRect();
        if (!caret || surfaces.empty()) return;

        auto const& surface = *surfaces.front();
        double scale = surfaceScale(surface);
        double ratio = controller.scale();
        auto const& bounds = controller.screenBounds();
        GdkRectangle area{static_cast<int>((caret->x * ratio + bounds.x - surface.rect.x) / scale),
                          static_cast<int>((caret->y * ratio + bounds.y - surface.rect.y) / scale), 1,
                          static_cast<int>(caret->height * ratio / scale)};
        gtk_im_context_set_cursor_location(im, &area);
    }

    std::optional<std::string> pickColor(std::string const& current) override {
        if (surfaces.empty()) return std::nullopt;

        auto parsed = parseHex(current);
        GdkRGBA initial{parsed.r, parsed.g, parsed.b, 1};
        auto dialog = gtk_color_dialog_new();
        gtk_color_dialog_set_with_alpha(dialog, FALSE);
        auto parent = GTK_WINDOW(surfaces.front()->window);

        auto chosen = waitFor<std::optional<std::string>>(
            [&](GAsyncReadyCallback done, gpointer data) {
                gtk_color_dialog_choose_rgba(dialog, parent, &initial, nullptr, done, data);
            },
            [](GObject* source, GAsyncResult* result) -> std::optional<std::string> {
                auto value = gtk_color_dialog_choose_rgba_finish(GTK_COLOR_DIALOG(source), result, nullptr);
                if (!value) return std::nullopt;
                auto hex = toHex(Color(value->red, value->green, value->blue));
                gdk_rgba_free(value);
                return hex;
            });

        g_object_unref(dialog);
        return chosen;
    }

    void copyText(std::wstring const& text) override {
        if (surfaces.empty()) return;
        gdk_clipboard_set_text(gtk_widget_get_clipboard(surfaces.front()->picture), toUtf8(text).c_str());
    }

    std::wstring pasteText() override {
        if (surfaces.empty()) return {};

        auto clipboard = gtk_widget_get_clipboard(surfaces.front()->picture);
        return waitFor<std::wstring>(
            [&](GAsyncReadyCallback done, gpointer data) { gdk_clipboard_read_text_async(clipboard, nullptr, done, data); },
            [](GObject* source, GAsyncResult* result) {
                auto text = gdk_clipboard_read_text_finish(GDK_CLIPBOARD(source), result, nullptr);
                std::wstring value = text ? toWide(text) : std::wstring();
                g_free(text);
                return value;
            });
    }

private:
    struct Surface {
        GtkOverlayWindow* owner = nullptr;
        ScreenRect rect;
        GdkMonitor* monitor = nullptr;
        GtkWidget* window = nullptr;
        GtkWidget* picture = nullptr;
        GtkEventController* keys = nullptr;
    };

    static GdkMonitor* monitorFor(GListModel* monitors, ScreenRect const& rect) {
        GdkMonitor* best = nullptr;
        double distance = std::numeric_limits<double>::max();

        for (guint index = 0; index < g_list_model_get_n_items(monitors); index++) {
            auto monitor = GDK_MONITOR(g_list_model_get_item(monitors, index));
            GdkRectangle geometry{};
            gdk_monitor_get_geometry(monitor, &geometry);
            double scale = gdk_monitor_get_scale(monitor);
            double gap = std::hypot(geometry.x * scale - rect.x, geometry.y * scale - rect.y) +
                         std::abs(geometry.width * scale - rect.width);
            if (gap < distance) {
                distance = gap;
                best = monitor;
            }
            g_object_unref(monitor);
        }

        return best;
    }

    static double surfaceScale(Surface const& surface) {
        int width = gtk_widget_get_width(surface.picture);
        if (width > 0) return static_cast<double>(surface.rect.width) / width;
        return surface.monitor ? gdk_monitor_get_scale(surface.monitor) : 1;
    }

    Point localPoint(Surface const& surface, double x, double y) const {
        double scale = surfaceScale(surface);
        auto const& bounds = controller.screenBounds();
        double ratio = controller.scale();
        return {(x * scale + surface.rect.x - bounds.x) / ratio, (y * scale + surface.rect.y - bounds.y) / ratio};
    }

    void paint(Surface& surface) {
        int width = surface.rect.width;
        int height = surface.rect.height;
        if (width <= 0 || height <= 0) return;

        auto target = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        auto context = cairo_create(target);
        auto const& bounds = controller.screenBounds();
        cairo_translate(context, bounds.x - surface.rect.x, bounds.y - surface.rect.y);

        {
            auto painter = createPainter(context, controller.resources());
            controller.render(*painter);
        }

        cairo_destroy(context);
        cairo_surface_flush(target);

        auto stride = cairo_image_surface_get_stride(target);
        auto bytes = g_bytes_new_with_free_func(
            cairo_image_surface_get_data(target), static_cast<gsize>(stride) * height,
            [](gpointer data) { cairo_surface_destroy(static_cast<cairo_surface_t*>(data)); }, target);
        auto texture = gdk_memory_texture_new(width, height, GDK_MEMORY_B8G8R8A8_PREMULTIPLIED, bytes, stride);
        g_bytes_unref(bytes);

        gtk_picture_set_paintable(GTK_PICTURE(surface.picture), GDK_PAINTABLE(texture));
        g_object_unref(texture);
    }

    static gboolean handleEvent(GtkEventControllerLegacy*, GdkEvent* event, gpointer data) {
        auto& surface = *static_cast<Surface*>(data);
        auto& self = *surface.owner;
        auto& controller = self.controller;
        if (controller.isFinished()) return FALSE;

        double x = 0;
        double y = 0;
        gdk_event_get_position(event, &x, &y);
        auto point = self.localPoint(surface, x, y);
        bool shift = (gdk_event_get_modifier_state(event) & GDK_SHIFT_MASK) != 0;

        try {
            switch (gdk_event_get_event_type(event)) {
            case GDK_MOTION_NOTIFY:
                controller.pointerMove(point, shift);
                gtk_widget_set_cursor_from_name(surface.picture, cursorName(controller.cursorAt(point)));
                self.invalidate();
                return TRUE;
            case GDK_BUTTON_PRESS:
                if (gdk_button_event_get_button(event) != GDK_BUTTON_PRIMARY) return FALSE;
                self.isPressed = true;
                gtk_widget_grab_focus(surface.picture);
                controller.pointerDown(point, shift);
                controller.afterInput();
                return TRUE;
            case GDK_BUTTON_RELEASE:
                if (gdk_button_event_get_button(event) == GDK_BUTTON_SECONDARY) {
                    controller.rightClick(point);
                    controller.afterInput();
                    return TRUE;
                }
                if (gdk_button_event_get_button(event) != GDK_BUTTON_PRIMARY) return FALSE;
                self.isPressed = false;
                controller.pointerUp(point);
                controller.afterInput();
                return TRUE;
            case GDK_LEAVE_NOTIFY:
                if (!self.isPressed) {
                    controller.pointerLeave();
                    self.invalidate();
                }
                return FALSE;
            default:
                return FALSE;
            }
        } catch (...) {
            logError(L"The capture overlay failed: " + describeException(std::current_exception()));
            controller.close();
            return TRUE;
        }
    }

    static gboolean handleKey(GtkEventControllerKey*, guint keyval, guint, GdkModifierType state, gpointer data) {
        auto& surface = *static_cast<Surface*>(data);
        auto& self = *surface.owner;
        auto& controller = self.controller;
        if (controller.isFinished()) return FALSE;

        auto modifiers = modifiersOf(state);
        bool isEditing = controller.isEditingText();

        try {
            controller.keyDown(keyOf(keyval), modifiers);
            if (!isEditing && !controller.isFinished()) {
                auto character = gdk_keyval_to_unicode(keyval);
                if (character >= 32) controller.character(static_cast<wchar_t>(character), modifiers);
            }
            controller.afterInput();
        } catch (...) {
            logError(L"The capture overlay failed: " + describeException(std::current_exception()));
            controller.close();
        }

        return TRUE;
    }

    OverlayController& controller;
    std::vector<std::unique_ptr<Surface>> surfaces;
    GtkIMContext* im = nullptr;
    guint frame = 0;
    guint caretTimer = 0;
    bool isPressed = false;
    bool wasCaretVisible = false;
};

}

std::unique_ptr<OverlayWindow> createOverlayWindow(OverlayController& controller) {
    return std::make_unique<GtkOverlayWindow>(controller);
}

}

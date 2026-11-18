#include "cairo.h"

#include <ui/overlayWindow.h>

#include <shared/utf8.h>

#include <gtk/gtk.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace fileship::ui {

namespace {

char const* cursorName(Cursor cursor) {
    switch (cursor) {
    case Cursor::Hand:
        return "pointer";
    case Cursor::Cross:
        return "crosshair";
    case Cursor::Text:
        return "text";
    case Cursor::Move:
        return "move";
    case Cursor::ResizeNwse:
        return "nwse-resize";
    case Cursor::ResizeNesw:
        return "nesw-resize";
    case Cursor::ResizeNs:
        return "ns-resize";
    case Cursor::ResizeWe:
        return "ew-resize";
    default:
        return "default";
    }
}

uint32_t latinKey(guint value) {
    guint upper = gdk_keyval_to_upper(value);
    return (upper >= 'A' && upper <= 'Z') || (upper >= '0' && upper <= '9') ? upper : 0;
}

Key keyFrom(GdkEvent* event) {
    guint value = gdk_key_event_get_keyval(event);

    switch (value) {
    case GDK_KEY_BackSpace:
        return Key::Backspace;
    case GDK_KEY_Tab:
    case GDK_KEY_ISO_Left_Tab:
        return Key::Tab;
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
        return Key::Enter;
    case GDK_KEY_Escape:
        return Key::Escape;
    case GDK_KEY_space:
        return Key::Space;
    case GDK_KEY_Delete:
        return Key::Delete;
    case GDK_KEY_Left:
        return Key::Left;
    case GDK_KEY_Up:
        return Key::Up;
    case GDK_KEY_Right:
        return Key::Right;
    case GDK_KEY_Down:
        return Key::Down;
    case GDK_KEY_Home:
        return Key::Home;
    case GDK_KEY_End:
        return Key::End;
    default:
        break;
    }

    if (value >= GDK_KEY_KP_0 && value <= GDK_KEY_KP_9)
        return static_cast<Key>(static_cast<uint32_t>(Key::Numpad0) + (value - GDK_KEY_KP_0));
    if (auto code = latinKey(value)) return static_cast<Key>(code);

    GdkKeymapKey* keys = nullptr;
    guint* values = nullptr;
    int count = 0;
    Key result = Key::None;

    if (gdk_display_map_keycode(gdk_event_get_display(event), gdk_key_event_get_keycode(event), &keys, &values,
                                &count)) {
        for (int index = 0; index < count && result == Key::None; index++)
            if (keys[index].level == 0) result = static_cast<Key>(latinKey(values[index]));
        g_free(keys);
        g_free(values);
    }

    return result;
}

class GtkOverlay : public OverlayWindow {
public:
    GtkOverlay(CairoCanvas& canvas, OverlayRequest const& request, double ratio, OverlayClient& client,
               Annotator& annotator)
        : canvas(canvas), request(request), bounds(request.bounds), ratio(ratio), client(client),
          annotator(annotator) {}

    ~GtkOverlay() override {
        close();
    }

    bool create() {
        if (!gtk_init_check()) return false;

        auto display = gdk_display_get_default();
        bool isWayland = strcmp(G_OBJECT_TYPE_NAME(display), "GdkWaylandDisplay") == 0;
        double largest = 1;
        for (auto const& snapshot : request.displays) largest = std::max(largest, snapshot.scaleFactor);

        auto monitors = gdk_display_get_monitors(display);
        for (guint index = 0; index < g_list_model_get_n_items(monitors); index++) {
            auto monitor = GDK_MONITOR(g_list_model_get_item(monitors, index));
            GdkRectangle geometry{};
            gdk_monitor_get_geometry(monitor, &geometry);
            double origin = isWayland ? largest : gdk_monitor_get_scale_factor(monitor);

            for (auto const& snapshot : request.displays) {
                if (std::lround(snapshot.rect.x / origin) != geometry.x ||
                    std::lround(snapshot.rect.y / origin) != geometry.y)
                    continue;
                addScreen(monitor, geometry, snapshot.rect);
                break;
            }

            g_object_unref(monitor);
        }

        if (screens.empty()) return false;

        input = gtk_im_multicontext_new();
        g_signal_connect(input, "commit", G_CALLBACK(handleCommit), this);
        g_signal_connect(input, "preedit-changed", G_CALLBACK(handlePreedit), this);
        g_signal_connect(input, "preedit-end", G_CALLBACK(handlePreeditEnd), this);

        auto holdsCursor = [&](Screen const& screen) {
            auto const& rect = screen.physical;
            return request.cursor.x >= rect.x && request.cursor.y >= rect.y && request.cursor.x < rect.x + rect.width &&
                   request.cursor.y < rect.y + rect.height;
        };

        std::stable_partition(screens.begin(), screens.end(), [&](auto const& screen) { return !holdsCursor(*screen); });
        for (auto& screen : screens) gtk_window_present(GTK_WINDOW(screen->window));
        focused = screens.back().get();
        return true;
    }

    void close() override {
        if (isClosed) return;
        isClosed = true;

        if (caretTimer) g_source_remove(caretTimer);
        for (auto& screen : screens) gtk_window_destroy(GTK_WINDOW(screen->window));
        if (input) g_object_unref(input);
    }

    std::optional<std::string> pickColor(std::string const& current) override {
        struct Choice {
            bool isDone = false;
            std::optional<std::string> color;
        } choice;

        auto parsed = parseHex(current);
        GdkRGBA initial{parsed.r, parsed.g, parsed.b, 1};
        auto dialog = gtk_color_dialog_new();
        gtk_color_dialog_set_with_alpha(dialog, FALSE);
        gtk_color_dialog_choose_rgba(
            dialog, GTK_WINDOW(focused->window), &initial, nullptr,
            [](GObject* source, GAsyncResult* result, gpointer data) {
                auto chosen = static_cast<Choice*>(data);
                if (auto color = gtk_color_dialog_choose_rgba_finish(GTK_COLOR_DIALOG(source), result, nullptr)) {
                    chosen->color = toHex(Color(color->red, color->green, color->blue));
                    gdk_rgba_free(color);
                }
                chosen->isDone = true;
            },
            &choice);

        while (!choice.isDone) g_main_context_iteration(nullptr, TRUE);
        g_object_unref(dialog);
        return choice.color;
    }

private:
    struct Screen {
        GtkOverlay* owner = nullptr;
        GdkRectangle geometry{};
        ScreenRect physical;
        GtkWidget* window = nullptr;
        GtkWidget* area = nullptr;
        char const* cursor = nullptr;
    };

    void addScreen(GdkMonitor* monitor, GdkRectangle const& geometry, ScreenRect const& physical) {
        auto screen = std::make_unique<Screen>();
        screen->owner = this;
        screen->geometry = geometry;
        screen->physical = physical;

        screen->window = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(screen->window), "Fileship");
        gtk_window_set_decorated(GTK_WINDOW(screen->window), FALSE);
        g_signal_connect(screen->window, "close-request", G_CALLBACK(handleClose), screen.get());

        screen->area = gtk_drawing_area_new();
        gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(screen->area), draw, screen.get(), nullptr);
        gtk_window_set_child(GTK_WINDOW(screen->window), screen->area);

        auto events = gtk_event_controller_legacy_new();
        g_signal_connect(events, "event", G_CALLBACK(handleEvent), screen.get());
        gtk_widget_add_controller(screen->window, events);

        gtk_window_fullscreen_on_monitor(GTK_WINDOW(screen->window), monitor);
        screens.push_back(std::move(screen));
    }

    void invalidate() {
        for (auto& screen : screens) gtk_widget_queue_draw(screen->area);
    }

    Point pointOf(Screen const& screen, GdkEvent* event) const {
        double x = 0;
        double y = 0;
        double left = 0;
        double top = 0;
        gdk_event_get_position(event, &x, &y);
        gtk_native_get_surface_transform(GTK_NATIVE(screen.window), &left, &top);
        x += screen.geometry.x - left;
        y += screen.geometry.y - top;

        Screen const* target = &screen;
        for (auto const& other : screens) {
            auto const& area = other->geometry;
            if (x >= area.x && y >= area.y && x < area.x + area.width && y < area.y + area.height) target = other.get();
        }

        double scale = static_cast<double>(target->physical.width) / target->geometry.width;
        return {(target->physical.x + (x - target->geometry.x) * scale - bounds.x) / ratio,
                (target->physical.y + (y - target->geometry.y) * scale - bounds.y) / ratio};
    }

    void updateCursor(Screen& screen, Point point) {
        auto name = cursorName(client.cursorAt(point));
        if (name == screen.cursor) return;

        screen.cursor = name;
        gtk_widget_set_cursor_from_name(screen.window, name);
    }

    void placeInput() {
        float height = 0;
        auto caret = annotator.editorCaret(height);
        auto const& screen = *focused;
        double scale = static_cast<double>(screen.geometry.width) / screen.physical.width;

        GdkRectangle area{static_cast<int>((caret.x * ratio + bounds.x - screen.physical.x) * scale),
                          static_cast<int>((caret.y * ratio + bounds.y - screen.physical.y) * scale), 1,
                          static_cast<int>(height * ratio * scale)};
        gtk_im_context_set_cursor_location(input, &area);
    }

    void updateInput() {
        bool editing = annotator.isEditingText();

        if (editing != isEditing) {
            isEditing = editing;

            if (editing) {
                gtk_im_context_set_client_widget(input, focused->area);
                gtk_im_context_focus_in(input);
                caretTimer = g_timeout_add(50, handleCaretTimer, this);
            } else {
                gtk_im_context_reset(input);
                gtk_im_context_focus_out(input);
                g_source_remove(caretTimer);
                caretTimer = 0;
            }
        }

        if (editing) placeInput();
    }

    void handleKey(GdkEvent* event) {
        auto state = gdk_event_get_modifier_state(event);
        Modifiers modifiers{(state & GDK_CONTROL_MASK) != 0, (state & GDK_SHIFT_MASK) != 0,
                            (state & GDK_ALT_MASK) != 0};

        if (annotator.isEditingText() && gtk_im_context_filter_keypress(input, event)) return;

        client.keyDown(keyFrom(event), modifiers);
        if (isClosed || annotator.isEditingText()) return;

        uint32_t character = gdk_keyval_to_unicode(gdk_key_event_get_keyval(event));
        if (character > 32 && character != 127) client.character(static_cast<wchar_t>(character), modifiers);
    }

    static void draw(GtkDrawingArea*, cairo_t* cr, int width, int, gpointer data) {
        auto screen = static_cast<Screen*>(data);
        auto self = screen->owner;
        if (self->isClosed) return;

        double scale = static_cast<double>(width) / screen->physical.width;
        cairo_scale(cr, scale, scale);
        cairo_translate(cr, self->bounds.x - screen->physical.x, self->bounds.y - screen->physical.y);

        CairoPainter painter(cr, self->canvas.resources());
        self->client.paint(painter);

        if (self->client.isAnimating() && !self->animation)
            self->animation = gtk_widget_add_tick_callback(self->screens.front()->area, handleFrame, self, nullptr);
    }

    static gboolean handleFrame(GtkWidget*, GdkFrameClock*, gpointer data) {
        auto self = static_cast<GtkOverlay*>(data);
        self->invalidate();
        if (self->client.isAnimating()) return G_SOURCE_CONTINUE;

        self->animation = 0;
        return G_SOURCE_REMOVE;
    }

    static gboolean handleEvent(GtkEventControllerLegacy*, GdkEvent* event, gpointer data) {
        auto screen = static_cast<Screen*>(data);
        auto self = screen->owner;
        if (self->isClosed) return TRUE;

        auto type = gdk_event_get_event_type(event);
        bool shift = (gdk_event_get_modifier_state(event) & GDK_SHIFT_MASK) != 0;

        switch (type) {
        case GDK_ENTER_NOTIFY:
        case GDK_MOTION_NOTIFY: {
            auto point = self->pointOf(*screen, event);
            self->focused = screen;
            self->client.pointerMove(point, shift);
            self->updateCursor(*screen, point);
            break;
        }
        case GDK_LEAVE_NOTIFY:
            self->client.pointerLeave();
            break;
        case GDK_BUTTON_PRESS:
            if (gdk_button_event_get_button(event) != GDK_BUTTON_PRIMARY) return TRUE;
            self->focused = screen;
            self->client.pointerDown(self->pointOf(*screen, event), shift);
            break;
        case GDK_BUTTON_RELEASE: {
            auto button = gdk_button_event_get_button(event);
            auto point = self->pointOf(*screen, event);
            if (button == GDK_BUTTON_PRIMARY) {
                self->client.pointerUp(point);
            } else if (button == GDK_BUTTON_SECONDARY) {
                self->client.rightClick(point);
            } else {
                return TRUE;
            }
            if (!self->isClosed) self->updateCursor(*screen, point);
            break;
        }
        case GDK_KEY_PRESS:
            self->handleKey(event);
            break;
        case GDK_KEY_RELEASE:
            if (self->annotator.isEditingText()) gtk_im_context_filter_keypress(self->input, event);
            return TRUE;
        default:
            return FALSE;
        }

        if (self->isClosed) return TRUE;
        self->updateInput();
        self->invalidate();
        return TRUE;
    }

    static gboolean handleClose(GtkWindow*, gpointer data) {
        static_cast<Screen*>(data)->owner->client.cancel();
        return TRUE;
    }

    static gboolean handleCaretTimer(gpointer data) {
        auto self = static_cast<GtkOverlay*>(data);
        bool visible = self->annotator.caretVisible();
        if (visible != self->wasCaretVisible) {
            self->wasCaretVisible = visible;
            self->invalidate();
        }
        return G_SOURCE_CONTINUE;
    }

    static void handleCommit(GtkIMContext*, char const* text, gpointer data) {
        auto self = static_cast<GtkOverlay*>(data);
        self->annotator.editorCommit(toWide(text));
        self->invalidate();
    }

    static void handlePreedit(GtkIMContext* context, gpointer data) {
        auto self = static_cast<GtkOverlay*>(data);
        char* text = nullptr;
        gtk_im_context_get_preedit_string(context, &text, nullptr, nullptr);
        self->annotator.editorComposition(toWide(text));
        g_free(text);

        if (self->annotator.isEditingText()) self->placeInput();
        self->invalidate();
    }

    static void handlePreeditEnd(GtkIMContext*, gpointer data) {
        auto self = static_cast<GtkOverlay*>(data);
        self->annotator.editorComposition(L"");
        self->invalidate();
    }

    CairoCanvas& canvas;
    OverlayRequest const& request;
    ScreenRect bounds;
    double ratio;
    OverlayClient& client;
    Annotator& annotator;
    std::vector<std::unique_ptr<Screen>> screens;
    Screen* focused = nullptr;
    GtkIMContext* input = nullptr;
    guint caretTimer = 0;
    guint animation = 0;
    bool isClosed = false;
    bool isEditing = false;
    bool wasCaretVisible = false;
};

}

std::unique_ptr<OverlayWindow> OverlayWindow::open(Canvas& canvas, OverlayRequest const& request, double ratio,
                                                   OverlayClient& client, Annotator& annotator) {
    auto window = std::make_unique<GtkOverlay>(static_cast<CairoCanvas&>(canvas), request, ratio, client, annotator);
    if (!window->create()) return nullptr;
    return window;
}

}

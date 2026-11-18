#include <ui/platform.h>

#include <shared/utf8.h>

#include <gtk/gtk.h>

namespace fileship::ui {

double caretBlinkMs() {
    gboolean blinks = TRUE;
    int cycle = 1200;
    g_object_get(gtk_settings_get_default(), "gtk-cursor-blink", &blinks, "gtk-cursor-blink-time", &cycle, nullptr);
    return blinks ? cycle / 2.0 : 0;
}

std::optional<std::wstring> clipboardText() {
    struct Read {
        bool isDone = false;
        std::optional<std::wstring> text;
    } read;

    gdk_clipboard_read_text_async(
        gdk_display_get_clipboard(gdk_display_get_default()), nullptr,
        [](GObject* source, GAsyncResult* result, gpointer data) {
            auto state = static_cast<Read*>(data);
            if (char* text = gdk_clipboard_read_text_finish(GDK_CLIPBOARD(source), result, nullptr)) {
                state->text = toWide(text);
                g_free(text);
            }
            state->isDone = true;
        },
        &read);

    while (!read.isDone) g_main_context_iteration(nullptr, TRUE);
    return read.text;
}

void setClipboardText(std::wstring const& text) {
    gdk_clipboard_set_text(gdk_display_get_clipboard(gdk_display_get_default()), toUtf8(text).c_str());
}

}

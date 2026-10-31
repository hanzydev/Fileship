#include "linux.h"

#include <host/app.h>
#include <shared/log.h>

#include <gtk/gtk.h>

namespace fileship::host {

namespace {

GdkClipboard* clipboard() {
    return ensureGtk() ? gdk_display_get_clipboard(gdk_display_get_default()) : nullptr;
}

template <typename Result>
Result readClipboard(std::function<void(GdkClipboard*, GAsyncReadyCallback, gpointer)> start,
                     std::function<Result(GdkClipboard*, GAsyncResult*)> finish) {
    return runOnMainThread<Result>([start, finish] {
        auto board = clipboard();
        if (!board) return Result();

        struct State {
            GMainLoop* loop = g_main_loop_new(nullptr, FALSE);
            std::function<Result(GdkClipboard*, GAsyncResult*)> finish;
            Result result{};
        } state{.finish = finish};

        start(
            board,
            [](GObject* source, GAsyncResult* result, gpointer data) {
                auto& state = *static_cast<State*>(data);
                state.result = state.finish(GDK_CLIPBOARD(source), result);
                g_main_loop_quit(state.loop);
            },
            &state);

        g_main_loop_run(state.loop);
        g_main_loop_unref(state.loop);
        return std::move(state.result);
    });
}

}

void writeClipboardText(std::wstring const& text) {
    postToMainThread([text = toUtf8(text)] {
        if (auto board = clipboard()) gdk_clipboard_set_text(board, text.c_str());
    });
}

bool writeClipboardImage(std::vector<uint8_t> const& png, core::Bitmap const&) {
    return runOnMainThread<bool>([&png] {
        auto board = clipboard();
        if (!board) return false;

        auto bytes = g_bytes_new(png.data(), png.size());
        auto texture = gdk_texture_new_from_bytes(bytes, nullptr);
        g_bytes_unref(bytes);
        if (!texture) return false;

        gdk_clipboard_set_texture(board, texture);
        g_object_unref(texture);
        return true;
    });
}

std::wstring readClipboardText() {
    return readClipboard<std::wstring>(
        [](GdkClipboard* board, GAsyncReadyCallback done, gpointer data) {
            gdk_clipboard_read_text_async(board, nullptr, done, data);
        },
        [](GdkClipboard* board, GAsyncResult* result) {
            auto text = gdk_clipboard_read_text_finish(board, result, nullptr);
            std::wstring value = text ? toWide(text) : std::wstring();
            g_free(text);
            return value;
        });
}

std::optional<core::Bitmap> readClipboardImage() {
    return readClipboard<std::optional<core::Bitmap>>(
        [](GdkClipboard* board, GAsyncReadyCallback done, gpointer data) {
            gdk_clipboard_read_texture_async(board, nullptr, done, data);
        },
        [](GdkClipboard* board, GAsyncResult* result) -> std::optional<core::Bitmap> {
            auto texture = gdk_clipboard_read_texture_finish(board, result, nullptr);
            if (!texture) return std::nullopt;

            auto png = gdk_texture_save_to_png_bytes(texture);
            g_object_unref(texture);

            gsize size = 0;
            auto data = static_cast<uint8_t const*>(g_bytes_get_data(png, &size));
            auto bitmap = core::decodeImage(data, size);
            g_bytes_unref(png);
            return bitmap;
        });
}

std::optional<std::filesystem::path> readClipboardFilePath() {
    return readClipboard<std::optional<std::filesystem::path>>(
        [](GdkClipboard* board, GAsyncReadyCallback done, gpointer data) {
            gdk_clipboard_read_value_async(board, GDK_TYPE_FILE_LIST, G_PRIORITY_DEFAULT, nullptr, done, data);
        },
        [](GdkClipboard* board, GAsyncResult* result) -> std::optional<std::filesystem::path> {
            auto value = gdk_clipboard_read_value_finish(board, result, nullptr);
            if (!value) return std::nullopt;

            std::optional<std::filesystem::path> path;
            auto files = static_cast<GSList*>(g_value_get_boxed(value));
            if (files) {
                if (auto local = g_file_get_path(G_FILE(files->data))) {
                    path = std::filesystem::path(local);
                    g_free(local);
                }
            }

            return path;
        });
}

}

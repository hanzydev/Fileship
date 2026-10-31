#include "clipboard.h"
#include "linux.h"

#include <core/linux/linux.h>
#include <host/app.h>
#include <shared/log.h>

#include <gtk/gtk.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <sstream>
#include <thread>

namespace fileship::host {

namespace {

constexpr int READ_TIMEOUT_MS = 5000;

std::vector<std::string> const TEXT_TYPES{"text/plain;charset=utf-8", "UTF8_STRING", "text/plain", "STRING", "TEXT"};
std::vector<std::string> const FILE_TYPES{"text/uri-list", "x-special/gnome-copied-files"};

ClipboardBackend* backend() {
    static ClipboardBackend* const instance = []() -> ClipboardBackend* {
        if (!core::isWaylandSession()) return nullptr;

        if (auto control = createDataControlClipboard()) {
            logInfo(L"Using the Wayland data control clipboard");
            return control.release();
        }

        auto display = std::getenv("DISPLAY");
        if (display && *display) {
            if (auto selection = createX11Clipboard()) {
                logInfo(L"Using the X11 clipboard through Xwayland");
                return selection.release();
            }
        }

        return nullptr;
    }();

    return instance;
}

std::optional<std::string> readFirst(ClipboardBackend& clipboard, std::vector<std::string> const& preferred) {
    auto available = clipboard.types();

    for (auto const& type : preferred) {
        if (std::find(available.begin(), available.end(), type) != available.end()) return clipboard.read(type);
    }

    return std::nullopt;
}

GdkClipboard* gdkClipboard() {
    return ensureGtk() ? gdk_display_get_clipboard(gdk_display_get_default()) : nullptr;
}

template <typename Result>
Result readGdkClipboard(std::function<void(GdkClipboard*, GAsyncReadyCallback, gpointer)> start,
                        std::function<Result(GdkClipboard*, GAsyncResult*)> finish) {
    return runOnMainThread<Result>([start, finish] {
        auto board = gdkClipboard();
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

std::optional<std::filesystem::path> firstFile(std::string const& list) {
    std::istringstream lines(list);
    std::string line;

    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.rfind("file://", 0) != 0) continue;

        if (auto path = g_filename_from_uri(line.c_str(), nullptr, nullptr)) {
            std::filesystem::path result(path);
            g_free(path);
            return result;
        }
    }

    return std::nullopt;
}

}

std::optional<std::string> readPipe(int fd) {
    std::string data;
    char buffer[65536];
    pollfd descriptor{fd, POLLIN, 0};

    while (true) {
        int ready = poll(&descriptor, 1, READ_TIMEOUT_MS);
        if (ready < 0 && errno == EINTR) continue;
        if (ready <= 0) break;

        auto count = ::read(fd, buffer, sizeof(buffer));
        if (count < 0 && (errno == EINTR || errno == EAGAIN)) continue;
        if (count <= 0) {
            close(fd);
            if (count < 0) return std::nullopt;
            return data;
        }

        data.append(buffer, static_cast<size_t>(count));
    }

    close(fd);
    return std::nullopt;
}

void writePipe(int fd, ClipboardData data) {
    std::thread([fd, data] {
        sigset_t pipeSignal;
        sigemptyset(&pipeSignal);
        sigaddset(&pipeSignal, SIGPIPE);
        pthread_sigmask(SIG_BLOCK, &pipeSignal, nullptr);

        size_t offset = 0;
        while (offset < data->size()) {
            auto count = ::write(fd, data->data() + offset, data->size() - offset);
            if (count < 0 && errno == EINTR) continue;
            if (count <= 0) break;
            offset += static_cast<size_t>(count);
        }

        close(fd);

        timespec immediately{};
        while (sigtimedwait(&pipeSignal, nullptr, &immediately) == SIGPIPE) {
        }
    }).detach();
}

void writeClipboardText(std::wstring const& text) {
    if (auto clipboard = backend()) {
        clipboard->write(TEXT_TYPES, std::make_shared<std::string const>(toUtf8(text)));
        return;
    }

    postToMainThread([text = toUtf8(text)] {
        if (auto board = gdkClipboard()) gdk_clipboard_set_text(board, text.c_str());
    });
}

bool writeClipboardImage(std::vector<uint8_t> const& png, core::Bitmap const&) {
    if (auto clipboard = backend())
        return clipboard->write({"image/png"}, std::make_shared<std::string const>(png.begin(), png.end()));

    return runOnMainThread<bool>([&png] {
        auto board = gdkClipboard();
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
    if (auto clipboard = backend()) {
        auto text = readFirst(*clipboard, TEXT_TYPES);
        return text ? toWide(*text) : std::wstring();
    }

    return readGdkClipboard<std::wstring>(
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
    if (auto clipboard = backend()) {
        std::vector<std::string> images{"image/png"};
        for (auto const& type : clipboard->types()) {
            if (type.rfind("image/", 0) == 0 && type != "image/png") images.push_back(type);
        }

        for (auto const& type : images) {
            auto data = clipboard->read(type);
            if (!data) continue;

            if (auto bitmap = core::decodeImage(reinterpret_cast<uint8_t const*>(data->data()), data->size()))
                return bitmap;
        }

        return std::nullopt;
    }

    return readGdkClipboard<std::optional<core::Bitmap>>(
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
    if (auto clipboard = backend()) {
        auto available = clipboard->types();

        for (auto const& type : FILE_TYPES) {
            if (std::find(available.begin(), available.end(), type) == available.end()) continue;
            if (auto list = clipboard->read(type)) return firstFile(*list);
        }

        return std::nullopt;
    }

    return readGdkClipboard<std::optional<std::filesystem::path>>(
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

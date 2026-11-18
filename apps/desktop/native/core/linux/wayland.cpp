#include "screen.h"

#include <shared/linux/portal.h>
#include <shared/log.h>
#include <shared/utf8.h>

#include <gio/gunixfdlist.h>
#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <mutex>
#include <stdexcept>

namespace fileship::core::wayland {

namespace {

constexpr char const* INTERFACE = "org.freedesktop.portal.ScreenCast";
constexpr uint32_t MONITORS = 1;
constexpr uint32_t PERSISTENT = 2;
constexpr int64_t GRAB_LIFETIME_MS = 1500;

enum class CursorMode : uint32_t { Hidden = 1, Embedded = 2, Metadata = 4 };

struct Reader;

struct Screen {
    uint32_t node = 0;
    Rect logical;
    DisplayInfo display;
    Bitmap frame;
    std::optional<Point> cursor;
    bool hasBuffer = false;

    pw_stream* stream = nullptr;
    spa_hook listener{};
    spa_video_info_raw format{};
    Reader* reader = nullptr;
};

struct Reader {
    pw_main_loop* loop = nullptr;
    std::vector<Screen>* screens = nullptr;
    CursorMode mode = CursorMode::Hidden;

    bool isDone() const {
        return std::all_of(screens->begin(), screens->end(), [this](Screen const& screen) {
            return mode == CursorMode::Metadata ? screen.hasBuffer : !screen.frame.pixels.empty();
        });
    }
};

struct Grab {
    std::vector<Screen> screens;
    CursorMode mode = CursorMode::Hidden;
    int64_t takenAt = 0;
};

std::mutex mutex;
std::optional<Grab> current;
CursorMode lastMode = CursorMode::Hidden;
std::string restoreToken;
std::function<void(std::string const&)> onRestoreToken;

int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

void closeSession(std::string const& session) {
    g_dbus_connection_call(sessionBus(), PORTAL_NAME, session.c_str(), "org.freedesktop.portal.Session", "Close",
                           nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr, nullptr);
}

std::vector<Screen> startSession(CursorMode mode, std::string& session, int& remote) {
    auto created = portalWait(INTERFACE, "CreateSession", [](char const* token) {
        GVariantBuilder options;
        g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string(token));
        g_variant_builder_add(&options, "{sv}", "session_handle_token", g_variant_new_string(portalToken().c_str()));
        return g_variant_new("(a{sv})", &options);
    });
    if (created.response) throw std::runtime_error("The system refused a screen capture session");

    gchar const* handle = nullptr;
    g_variant_lookup(created.results, "session_handle", "&s", &handle);
    session = handle;

    auto selected = portalWait(INTERFACE, "SelectSources", [&](char const* token) {
        GVariantBuilder options;
        g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string(token));
        g_variant_builder_add(&options, "{sv}", "types", g_variant_new_uint32(MONITORS));
        g_variant_builder_add(&options, "{sv}", "multiple", g_variant_new_boolean(TRUE));
        g_variant_builder_add(&options, "{sv}", "cursor_mode", g_variant_new_uint32(static_cast<uint32_t>(mode)));
        g_variant_builder_add(&options, "{sv}", "persist_mode", g_variant_new_uint32(PERSISTENT));

        if (!restoreToken.empty()) {
            g_variant_builder_add(&options, "{sv}", "restore_token", g_variant_new_string(restoreToken.c_str()));
        }

        return g_variant_new("(oa{sv})", session.c_str(), &options);
    });
    if (selected.response) throw std::runtime_error("The system refused the screen capture sources");

    auto started = portalWait(INTERFACE, "Start", [&](char const* token) {
        GVariantBuilder options;
        g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string(token));
        return g_variant_new("(osa{sv})", session.c_str(), "", &options);
    });
    if (started.response) throw std::runtime_error("Screen capture was not allowed");

    gchar const* token = nullptr;
    if (g_variant_lookup(started.results, "restore_token", "&s", &token) && restoreToken != token) {
        restoreToken = token;
        onRestoreToken(restoreToken);
    }

    std::vector<Screen> screens;
    GVariant* streams = g_variant_lookup_value(started.results, "streams", G_VARIANT_TYPE("a(ua{sv})"));
    GVariantIter iterator;
    guint32 node = 0;
    GVariant* properties = nullptr;
    g_variant_iter_init(&iterator, streams);

    while (g_variant_iter_next(&iterator, "(u@a{sv})", &node, &properties)) {
        Screen screen;
        screen.node = node;
        g_variant_lookup(properties, "position", "(ii)", &screen.logical.x, &screen.logical.y);
        g_variant_lookup(properties, "size", "(ii)", &screen.logical.width, &screen.logical.height);
        screens.push_back(std::move(screen));
        g_variant_unref(properties);
    }

    g_variant_unref(streams);

    GVariantBuilder none;
    g_variant_builder_init(&none, G_VARIANT_TYPE("a{sv}"));
    GUnixFDList* descriptors = nullptr;
    GError* error = nullptr;
    GVariant* opened = g_dbus_connection_call_with_unix_fd_list_sync(
        sessionBus(), PORTAL_NAME, PORTAL_PATH, INTERFACE, "OpenPipeWireRemote",
        g_variant_new("(oa{sv})", session.c_str(), &none), G_VARIANT_TYPE("(h)"), G_DBUS_CALL_FLAGS_NONE, -1, nullptr,
        &descriptors, nullptr, &error);

    if (!opened) {
        std::string message = error->message;
        g_error_free(error);
        throw std::runtime_error(message);
    }

    gint32 index = 0;
    g_variant_get(opened, "(h)", &index);
    remote = g_unix_fd_list_get(descriptors, index, nullptr);

    g_variant_unref(opened);
    g_object_unref(descriptors);
    return screens;
}

void readFrame(Screen& screen, spa_data const& data) {
    auto width = screen.format.size.width;
    auto height = screen.format.size.height;
    bool isRgb = screen.format.format == SPA_VIDEO_FORMAT_RGBA || screen.format.format == SPA_VIDEO_FORMAT_RGBx;
    auto source = SPA_PTROFF(data.data, data.chunk->offset, uint8_t);

    screen.frame.width = width;
    screen.frame.height = height;
    screen.frame.pixels.resize(static_cast<size_t>(width) * height * 4);

    for (uint32_t row = 0; row < height; row++) {
        auto line = source + static_cast<size_t>(row) * data.chunk->stride;
        auto target = screen.frame.pixels.data() + static_cast<size_t>(row) * width * 4;

        for (uint32_t column = 0; column < width; column++) {
            target[column * 4] = line[column * 4 + (isRgb ? 2 : 0)];
            target[column * 4 + 1] = line[column * 4 + 1];
            target[column * 4 + 2] = line[column * 4 + (isRgb ? 0 : 2)];
            target[column * 4 + 3] = 255;
        }
    }
}

void handleFormat(void* data, uint32_t id, spa_pod const* param) {
    auto& screen = *static_cast<Screen*>(data);
    if (!param || id != SPA_PARAM_Format) return;

    spa_format_video_raw_parse(param, &screen.format);

    uint8_t storage[1024];
    spa_pod_builder builder = SPA_POD_BUILDER_INIT(storage, sizeof(storage));
    auto cursorSize = [](int side) {
        return static_cast<int>(sizeof(spa_meta_cursor) + sizeof(spa_meta_bitmap)) + side * side * 4;
    };

    spa_pod const* params[2];
    params[0] = static_cast<spa_pod const*>(spa_pod_builder_add_object(
        &builder, SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers, SPA_PARAM_BUFFERS_dataType,
        SPA_POD_CHOICE_FLAGS_Int((1 << SPA_DATA_MemPtr) | (1 << SPA_DATA_MemFd))));
    params[1] = static_cast<spa_pod const*>(spa_pod_builder_add_object(
        &builder, SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta, SPA_PARAM_META_type, SPA_POD_Id(SPA_META_Cursor),
        SPA_PARAM_META_size, SPA_POD_CHOICE_RANGE_Int(cursorSize(64), cursorSize(1), cursorSize(384))));

    pw_stream_update_params(screen.stream, params, screen.reader->mode == CursorMode::Metadata ? 2 : 1);
}

void handleProcess(void* data) {
    auto& screen = *static_cast<Screen*>(data);
    auto buffer = pw_stream_dequeue_buffer(screen.stream);
    if (!buffer) return;

    auto const& first = buffer->buffer->datas[0];
    auto cursor =
        static_cast<spa_meta_cursor*>(spa_buffer_find_meta_data(buffer->buffer, SPA_META_Cursor, sizeof(spa_meta_cursor)));

    screen.hasBuffer = true;
    if (cursor && spa_meta_cursor_is_valid(cursor)) screen.cursor = Point{cursor->position.x, cursor->position.y};
    if (screen.frame.pixels.empty() && first.data && first.chunk->size) readFrame(screen, first);

    pw_stream_queue_buffer(screen.stream, buffer);
    if (screen.reader->isDone()) pw_main_loop_quit(screen.reader->loop);
}

pw_stream_events const STREAM_EVENTS = [] {
    pw_stream_events events{};
    events.version = PW_VERSION_STREAM_EVENTS;
    events.param_changed = handleFormat;
    events.process = handleProcess;
    return events;
}();

void handleTimeout(void* data, uint64_t) {
    pw_main_loop_quit(static_cast<pw_main_loop*>(data));
}

void readStreams(std::vector<Screen>& screens, CursorMode mode, int remote, long timeoutMs) {
    pw_init(nullptr, nullptr);

    auto loop = pw_main_loop_new(nullptr);
    auto context = pw_context_new(pw_main_loop_get_loop(loop), nullptr, 0);
    auto core = pw_context_connect_fd(context, remote, nullptr, 0);
    Reader reader{loop, &screens, mode};

    spa_rectangle size = SPA_RECTANGLE(1920, 1080);
    spa_rectangle smallest = SPA_RECTANGLE(1, 1);
    spa_rectangle largest = SPA_RECTANGLE(16384, 16384);
    spa_fraction rate = SPA_FRACTION(30, 1);
    spa_fraction slowest = SPA_FRACTION(0, 1);
    spa_fraction fastest = SPA_FRACTION(1000, 1);

    for (auto& screen : screens) {
        screen.reader = &reader;
        screen.stream = pw_stream_new(core, "fileship",
                                      pw_properties_new(PW_KEY_MEDIA_TYPE, "Video", PW_KEY_MEDIA_CATEGORY, "Capture",
                                                        PW_KEY_MEDIA_ROLE, "Screen", nullptr));
        pw_stream_add_listener(screen.stream, &screen.listener, &STREAM_EVENTS, &screen);

        uint8_t storage[1024];
        spa_pod_builder builder = SPA_POD_BUILDER_INIT(storage, sizeof(storage));
        spa_pod const* params[1];
        params[0] = static_cast<spa_pod const*>(spa_pod_builder_add_object(
            &builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat, SPA_FORMAT_mediaType,
            SPA_POD_Id(SPA_MEDIA_TYPE_video), SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
            SPA_FORMAT_VIDEO_format,
            SPA_POD_CHOICE_ENUM_Id(5, SPA_VIDEO_FORMAT_BGRx, SPA_VIDEO_FORMAT_BGRx, SPA_VIDEO_FORMAT_BGRA,
                                   SPA_VIDEO_FORMAT_RGBx, SPA_VIDEO_FORMAT_RGBA),
            SPA_FORMAT_VIDEO_size, SPA_POD_CHOICE_RANGE_Rectangle(&size, &smallest, &largest),
            SPA_FORMAT_VIDEO_framerate, SPA_POD_CHOICE_RANGE_Fraction(&rate, &slowest, &fastest)));

        pw_stream_connect(screen.stream, PW_DIRECTION_INPUT, screen.node,
                          static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS), params,
                          1);
    }

    auto timer = pw_loop_add_timer(pw_main_loop_get_loop(loop), handleTimeout, loop);
    timespec timeout{timeoutMs / 1000, (timeoutMs % 1000) * 1'000'000};
    pw_loop_update_timer(pw_main_loop_get_loop(loop), timer, &timeout, nullptr, false);

    pw_main_loop_run(loop);

    for (auto& screen : screens) pw_stream_destroy(screen.stream);
    pw_core_disconnect(core);
    pw_context_destroy(context);
    pw_main_loop_destroy(loop);
}

std::vector<Screen> read(CursorMode mode, long timeoutMs) {
    std::string session;
    int remote = -1;
    auto screens = startSession(mode, session, remote);

    readStreams(screens, mode, remote, timeoutMs);
    closeSession(session);
    return screens;
}

Grab const& grab(std::optional<CursorMode> mode = {}) {
    bool isFresh = current && nowMs() - current->takenAt <= GRAB_LIFETIME_MS && (!mode || current->mode == *mode);
    if (isFresh) return *current;

    auto screens = read(mode.value_or(lastMode), 5000);
    std::erase_if(screens, [](Screen const& screen) { return screen.frame.pixels.empty(); });
    if (screens.empty()) throw std::runtime_error("The system sent no screen image");

    double largest = 1;
    for (auto& screen : screens) {
        screen.display.scaleFactor = static_cast<double>(screen.frame.width) / screen.logical.width;
        largest = std::max(largest, screen.display.scaleFactor);
    }

    for (auto& screen : screens) {
        screen.display.id = screen.logical.x * 100000LL + screen.logical.y;
        screen.display.rect = Rect{static_cast<int32_t>(std::lround(screen.logical.x * largest)),
                                   static_cast<int32_t>(std::lround(screen.logical.y * largest)),
                                   static_cast<int32_t>(screen.frame.width), static_cast<int32_t>(screen.frame.height)};
        screen.display.isPrimary = screen.logical.x == 0 && screen.logical.y == 0;
    }

    current = Grab{std::move(screens), mode.value_or(lastMode), nowMs()};
    return *current;
}

}

void setRestoreToken(std::string token, std::function<void(std::string const&)> onChanged) {
    std::lock_guard lock(mutex);
    restoreToken = std::move(token);
    onRestoreToken = std::move(onChanged);
}

bool captureRects(std::vector<Rect> const& rects, bool cursor, std::vector<uint8_t*> const& targets) {
    std::lock_guard lock(mutex);
    lastMode = cursor ? CursorMode::Embedded : CursorMode::Hidden;

    try {
        auto const& screens = grab(lastMode).screens;

        for (size_t index = 0; index < rects.size(); index++) {
            auto const& rect = rects[index];
            std::memset(targets[index], 0, static_cast<size_t>(rect.width) * rect.height * 4);

            for (auto const& screen : screens) {
                auto visible = intersectRects(rect, screen.display.rect);
                if (!visible) continue;

                for (int row = 0; row < visible->height; row++) {
                    auto from = screen.frame.pixels.data() +
                                (static_cast<size_t>(visible->y - screen.display.rect.y + row) * screen.frame.width +
                                 (visible->x - screen.display.rect.x)) *
                                    4;
                    auto to = targets[index] +
                              (static_cast<size_t>(visible->y - rect.y + row) * rect.width + (visible->x - rect.x)) * 4;
                    std::memcpy(to, from, static_cast<size_t>(visible->width) * 4);
                }
            }
        }

        current.reset();
        return true;
    } catch (std::exception const& error) {
        logWarn(L"Could not capture the screen: " + toWide(error.what()));
        return false;
    }
}

std::vector<DisplayInfo> listDisplays() {
    std::lock_guard lock(mutex);
    std::vector<DisplayInfo> displays;

    try {
        for (auto const& screen : grab().screens) displays.push_back(screen.display);
    } catch (std::exception const& error) {
        logWarn(L"Could not read the screens: " + toWide(error.what()));
    }

    return displays;
}

Rect virtualScreen() {
    int32_t left = 0;
    int32_t top = 0;
    int32_t right = 0;
    int32_t bottom = 0;

    for (auto const& display : listDisplays()) {
        left = std::min(left, display.rect.x);
        top = std::min(top, display.rect.y);
        right = std::max(right, display.rect.x + display.rect.width);
        bottom = std::max(bottom, display.rect.y + display.rect.height);
    }

    return Rect{left, top, right - left, bottom - top};
}

Point cursorPosition() {
    std::lock_guard lock(mutex);

    try {
        auto const& screens = grab().screens;
        if (screens.size() == 1) return Point{screens[0].display.rect.x, screens[0].display.rect.y};

        for (auto const& probe : read(CursorMode::Metadata, 500)) {
            if (!probe.cursor) continue;

            for (auto const& screen : screens) {
                if (screen.logical.x != probe.logical.x || screen.logical.y != probe.logical.y) continue;
                return Point{screen.display.rect.x + probe.cursor->x, screen.display.rect.y + probe.cursor->y};
            }
        }
    } catch (std::exception const& error) {
        logWarn(L"Could not read the pointer position: " + toWide(error.what()));
    }

    return {};
}

}

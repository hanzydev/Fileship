#include "screen.h"

#include <shared/linux/portal.h>
#include <shared/log.h>
#include <shared/utf8.h>

#include <gio/gunixfdlist.h>
#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <mutex>
#include <stdexcept>

namespace fileship::core::wayland {

namespace {

constexpr char const* INTERFACE = "org.freedesktop.portal.ScreenCast";
constexpr uint32_t PERSISTENT = 2;
constexpr int64_t GRAB_LIFETIME_MS = 1500;

enum class Source : uint32_t { Screens = 1, Window = 2 };

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
    FrameStream::State* live = nullptr;
};

struct Reader {
    pw_main_loop* loop = nullptr;
    std::vector<Screen>* screens = nullptr;
    Source source = Source::Screens;
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
struct Refusal {
    int64_t at = 0;
    std::string message;
    bool isCancelled = false;
};

std::optional<Refusal> refusal;
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

std::vector<Screen> startSession(Source source, CursorMode mode, std::string& session, int& remote) {
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
        g_variant_builder_add(&options, "{sv}", "types", g_variant_new_uint32(static_cast<uint32_t>(source)));
        g_variant_builder_add(&options, "{sv}", "multiple", g_variant_new_boolean(source == Source::Screens));
        g_variant_builder_add(&options, "{sv}", "cursor_mode", g_variant_new_uint32(static_cast<uint32_t>(mode)));

        if (source == Source::Screens) {
            g_variant_builder_add(&options, "{sv}", "persist_mode", g_variant_new_uint32(PERSISTENT));
            if (!restoreToken.empty())
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
    if (started.response == 1) throw CaptureCancelled();
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

void readFrame(Screen& screen, spa_data const& data, spa_meta_region const* crop) {
    auto format = screen.format.format;
    bool isRgb = format == SPA_VIDEO_FORMAT_RGBA || format == SPA_VIDEO_FORMAT_RGBx;
    bool hasAlpha = screen.reader->source == Source::Window &&
                    (format == SPA_VIDEO_FORMAT_BGRA || format == SPA_VIDEO_FORMAT_RGBA);
    bool isCropped = crop && spa_meta_region_is_valid(crop);
    uint32_t left = isCropped ? crop->region.position.x : 0;
    uint32_t top = isCropped ? crop->region.position.y : 0;
    uint32_t width = isCropped ? crop->region.size.width : screen.format.size.width;
    uint32_t height = isCropped ? crop->region.size.height : screen.format.size.height;
    auto source = SPA_PTROFF(data.data, data.chunk->offset, uint8_t);

    screen.frame.width = width;
    screen.frame.height = height;
    screen.frame.pixels.resize(static_cast<size_t>(width) * height * 4);

    for (uint32_t row = 0; row < height; row++) {
        auto line = source + static_cast<size_t>(top + row) * data.chunk->stride + static_cast<size_t>(left) * 4;
        auto target = screen.frame.pixels.data() + static_cast<size_t>(row) * width * 4;

        for (uint32_t column = 0; column < width; column++) {
            uint32_t alpha = hasAlpha ? line[column * 4 + 3] : 255;
            auto straight = [alpha](uint8_t value) {
                return static_cast<uint8_t>(alpha ? std::min(255u, value * 255u / alpha) : 0);
            };

            target[column * 4] = straight(line[column * 4 + (isRgb ? 2 : 0)]);
            target[column * 4 + 1] = straight(line[column * 4 + 1]);
            target[column * 4 + 2] = straight(line[column * 4 + (isRgb ? 0 : 2)]);
            target[column * 4 + 3] = static_cast<uint8_t>(alpha);
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

    std::vector<spa_pod const*> params;
    params.push_back(static_cast<spa_pod const*>(spa_pod_builder_add_object(
        &builder, SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers, SPA_PARAM_BUFFERS_dataType,
        SPA_POD_CHOICE_FLAGS_Int((1 << SPA_DATA_MemPtr) | (1 << SPA_DATA_MemFd)))));

    if (screen.reader->mode == CursorMode::Metadata) {
        params.push_back(static_cast<spa_pod const*>(spa_pod_builder_add_object(
            &builder, SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta, SPA_PARAM_META_type, SPA_POD_Id(SPA_META_Cursor),
            SPA_PARAM_META_size, SPA_POD_CHOICE_RANGE_Int(cursorSize(64), cursorSize(1), cursorSize(384)))));
    }

    if (screen.reader->source == Source::Window) {
        params.push_back(static_cast<spa_pod const*>(spa_pod_builder_add_object(
            &builder, SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta, SPA_PARAM_META_type, SPA_POD_Id(SPA_META_VideoCrop),
            SPA_PARAM_META_size, SPA_POD_Int(sizeof(spa_meta_region)))));
    }

    pw_stream_update_params(screen.stream, params.data(), static_cast<uint32_t>(params.size()));
}

void handleProcess(void* data) {
    auto& screen = *static_cast<Screen*>(data);
    auto buffer = pw_stream_dequeue_buffer(screen.stream);
    if (!buffer) return;

    auto const& first = buffer->buffer->datas[0];
    auto cursor =
        static_cast<spa_meta_cursor*>(spa_buffer_find_meta_data(buffer->buffer, SPA_META_Cursor, sizeof(spa_meta_cursor)));

    auto crop = static_cast<spa_meta_region*>(
        spa_buffer_find_meta_data(buffer->buffer, SPA_META_VideoCrop, sizeof(spa_meta_region)));

    screen.hasBuffer = true;
    if (cursor && spa_meta_cursor_is_valid(cursor)) screen.cursor = Point{cursor->position.x, cursor->position.y};
    if (screen.frame.pixels.empty() && first.data && first.chunk->size) readFrame(screen, first, crop);

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

void connectStream(Screen& screen, pw_core* core, Reader& reader, pw_stream_events const& events) {
    spa_rectangle size = SPA_RECTANGLE(1920, 1080);
    spa_rectangle smallest = SPA_RECTANGLE(1, 1);
    spa_rectangle largest = SPA_RECTANGLE(16384, 16384);
    spa_fraction rate = SPA_FRACTION(30, 1);
    spa_fraction slowest = SPA_FRACTION(0, 1);
    spa_fraction fastest = SPA_FRACTION(1000, 1);
    std::array<spa_video_format, 4> formats{SPA_VIDEO_FORMAT_BGRx, SPA_VIDEO_FORMAT_BGRA, SPA_VIDEO_FORMAT_RGBx,
                                            SPA_VIDEO_FORMAT_RGBA};
    if (reader.source == Source::Window) std::swap(formats[0], formats[1]);

    screen.reader = &reader;
    screen.stream = pw_stream_new(core, "fileship",
                                  pw_properties_new(PW_KEY_MEDIA_TYPE, "Video", PW_KEY_MEDIA_CATEGORY, "Capture",
                                                    PW_KEY_MEDIA_ROLE, "Screen", nullptr));
    pw_stream_add_listener(screen.stream, &screen.listener, &events, &screen);

    uint8_t storage[1024];
    spa_pod_builder builder = SPA_POD_BUILDER_INIT(storage, sizeof(storage));
    spa_pod const* params[1];
    params[0] = static_cast<spa_pod const*>(spa_pod_builder_add_object(
        &builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat, SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
        SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw), SPA_FORMAT_VIDEO_format,
        SPA_POD_CHOICE_ENUM_Id(5, formats[0], formats[0], formats[1], formats[2], formats[3]), SPA_FORMAT_VIDEO_size,
        SPA_POD_CHOICE_RANGE_Rectangle(&size, &smallest, &largest), SPA_FORMAT_VIDEO_framerate,
        SPA_POD_CHOICE_RANGE_Fraction(&rate, &slowest, &fastest)));

    pw_stream_connect(screen.stream, PW_DIRECTION_INPUT, screen.node,
                      static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS), params, 1);
}

void readStreams(std::vector<Screen>& screens, Source source, CursorMode mode, int remote, long timeoutMs) {
    pw_init(nullptr, nullptr);

    auto loop = pw_main_loop_new(nullptr);
    auto context = pw_context_new(pw_main_loop_get_loop(loop), nullptr, 0);
    auto core = pw_context_connect_fd(context, remote, nullptr, 0);
    Reader reader{loop, &screens, source, mode};

    for (auto& screen : screens) connectStream(screen, core, reader, STREAM_EVENTS);

    auto timer = pw_loop_add_timer(pw_main_loop_get_loop(loop), handleTimeout, loop);
    timespec timeout{timeoutMs / 1000, (timeoutMs % 1000) * 1'000'000};
    pw_loop_update_timer(pw_main_loop_get_loop(loop), timer, &timeout, nullptr, false);

    pw_main_loop_run(loop);

    for (auto& screen : screens) pw_stream_destroy(screen.stream);
    pw_core_disconnect(core);
    pw_context_destroy(context);
    pw_main_loop_destroy(loop);
}

std::vector<Screen> read(Source source, CursorMode mode, long timeoutMs) {
    std::string session;
    int remote = -1;
    auto screens = startSession(source, mode, session, remote);

    if (!screens.empty()) readStreams(screens, source, mode, remote, timeoutMs);
    closeSession(session);
    return screens;
}

Grab const& grab(std::optional<CursorMode> mode = {}) {
    bool isFresh = current && nowMs() - current->takenAt <= GRAB_LIFETIME_MS && (!mode || current->mode == *mode);
    if (isFresh) return *current;
    if (refusal && nowMs() - refusal->at <= GRAB_LIFETIME_MS) {
        if (refusal->isCancelled) throw CaptureCancelled();
        throw std::runtime_error(refusal->message);
    }

    std::vector<Screen> screens;
    try {
        screens = read(Source::Screens, mode.value_or(lastMode), 5000);
        std::erase_if(screens, [](Screen const& screen) { return screen.frame.pixels.empty(); });
        if (screens.empty()) throw std::runtime_error("The system sent no screen image");
    } catch (CaptureCancelled const&) {
        refusal = Refusal{nowMs(), {}, true};
        throw;
    } catch (std::exception const& error) {
        refusal = Refusal{nowMs(), error.what(), false};
        throw;
    }

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

void handleLiveProcess(void* data);

pw_stream_events const LIVE_EVENTS = [] {
    pw_stream_events events{};
    events.version = PW_VERSION_STREAM_EVENTS;
    events.param_changed = handleFormat;
    events.process = handleLiveProcess;
    return events;
}();

}

struct FrameStream::State {
    Rect rect;
    std::string session;
    std::vector<Screen> screens;
    Reader reader;
    pw_thread_loop* loop = nullptr;
    pw_context* context = nullptr;
    pw_core* core = nullptr;
    std::mutex mutex;
    std::vector<uint8_t> frame;
    bool hasFrame = false;

    void blit(Screen const& screen, spa_data const& data) {
        Rect drawn{screen.display.rect.x, screen.display.rect.y, static_cast<int32_t>(screen.format.size.width),
                   static_cast<int32_t>(screen.format.size.height)};
        auto visible = intersectRects(rect, drawn);
        if (!visible) return;

        bool isRgb = screen.format.format == SPA_VIDEO_FORMAT_RGBA || screen.format.format == SPA_VIDEO_FORMAT_RGBx;
        auto source = SPA_PTROFF(data.data, data.chunk->offset, uint8_t);

        std::lock_guard lock(mutex);

        for (int32_t row = 0; row < visible->height; row++) {
            auto from = source + static_cast<size_t>(visible->y - drawn.y + row) * data.chunk->stride +
                        static_cast<size_t>(visible->x - drawn.x) * 4;
            auto to = frame.data() +
                      (static_cast<size_t>(visible->y - rect.y + row) * rect.width + (visible->x - rect.x)) * 4;

            if (!isRgb) {
                std::memcpy(to, from, static_cast<size_t>(visible->width) * 4);
                continue;
            }

            for (int32_t column = 0; column < visible->width; column++) {
                to[column * 4] = from[column * 4 + 2];
                to[column * 4 + 1] = from[column * 4 + 1];
                to[column * 4 + 2] = from[column * 4];
            }
        }

        hasFrame = true;
    }
};

namespace {

void handleLiveProcess(void* data) {
    auto& screen = *static_cast<Screen*>(data);
    auto buffer = pw_stream_dequeue_buffer(screen.stream);
    if (!buffer) return;

    auto const& first = buffer->buffer->datas[0];
    if (first.data && first.chunk->size) screen.live->blit(screen, first);
    pw_stream_queue_buffer(screen.stream, buffer);
}

}

FrameStream::FrameStream(Rect const& rect, bool cursor) : state(std::make_unique<State>()) {
    std::lock_guard lock(mutex);
    auto mode = cursor ? CursorMode::Embedded : CursorMode::Hidden;
    auto known = grab().screens;
    int remote = -1;

    state->rect = rect;
    state->frame.resize(static_cast<size_t>(rect.width) * rect.height * 4);
    state->screens = startSession(Source::Screens, mode, state->session, remote);
    if (state->screens.empty()) {
        closeSession(state->session);
        throw std::runtime_error("Screen capture was not allowed");
    }

    for (auto& screen : state->screens) {
        for (auto const& other : known) {
            if (other.logical.x == screen.logical.x && other.logical.y == screen.logical.y) screen.display = other.display;
        }
    }

    std::erase_if(state->screens, [&](Screen const& screen) { return !intersectRects(rect, screen.display.rect); });

    pw_init(nullptr, nullptr);
    state->reader = Reader{nullptr, &state->screens, Source::Screens, mode};
    state->loop = pw_thread_loop_new("fileship-recording", nullptr);
    state->context = pw_context_new(pw_thread_loop_get_loop(state->loop), nullptr, 0);
    pw_thread_loop_start(state->loop);

    pw_thread_loop_lock(state->loop);
    state->core = pw_context_connect_fd(state->context, remote, nullptr, 0);
    for (auto& screen : state->screens) {
        screen.live = state.get();
        connectStream(screen, state->core, state->reader, LIVE_EVENTS);
    }
    pw_thread_loop_unlock(state->loop);
}

FrameStream::~FrameStream() {
    pw_thread_loop_stop(state->loop);
    for (auto& screen : state->screens) pw_stream_destroy(screen.stream);
    pw_core_disconnect(state->core);
    pw_context_destroy(state->context);
    pw_thread_loop_destroy(state->loop);
    closeSession(state->session);
}

bool FrameStream::copy(uint8_t* target) {
    std::lock_guard lock(state->mutex);
    if (!state->hasFrame) return false;

    std::memcpy(target, state->frame.data(), state->frame.size());
    return true;
}

void setRestoreToken(std::string token, std::function<void(std::string const&)> onChanged) {
    std::lock_guard lock(mutex);
    restoreToken = std::move(token);
    onRestoreToken = std::move(onChanged);
}

bool chooseScreens() {
    std::lock_guard lock(mutex);
    auto previous = restoreToken;
    restoreToken.clear();
    current.reset();
    refusal.reset();

    try {
        grab();
        return true;
    } catch (std::exception const&) {
        restoreToken = previous;
        return false;
    }
}

std::optional<WindowShot> captureWindow(bool cursor) {
    std::lock_guard lock(mutex);
    auto windows = read(Source::Window, cursor ? CursorMode::Embedded : CursorMode::Hidden, 5000);
    if (windows.empty()) return std::nullopt;
    if (windows[0].frame.pixels.empty()) throw std::runtime_error("The system sent no window image");

    return WindowShot{std::move(windows[0].frame), {}};
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
    } catch (CaptureCancelled const&) {
        throw;
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

        for (auto const& probe : read(Source::Screens, CursorMode::Metadata, 500)) {
            if (!probe.cursor) continue;

            for (auto const& screen : screens) {
                if (screen.logical.x != probe.logical.x || screen.logical.y != probe.logical.y) continue;
                return Point{screen.display.rect.x + probe.cursor->x, screen.display.rect.y + probe.cursor->y};
            }
        }
    } catch (CaptureCancelled const&) {
        throw;
    } catch (std::exception const& error) {
        logWarn(L"Could not read the pointer position: " + toWide(error.what()));
    }

    return {};
}

}

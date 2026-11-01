#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wpedantic"

#include <host/recording/platform.h>

#include <core/core.h>
#include <host/services.h>
#include <shared/i18n.h>
#include <shared/log.h>
#include <shared/utf8.h>

#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>
#include <spa/param/buffers.h>

#include <fcntl.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace fileship::host::recording {

namespace {

constexpr char const* PORTAL = "org.freedesktop.portal.Desktop";
constexpr char const* PORTAL_PATH = "/org/freedesktop/portal/desktop";
constexpr char const* SCREEN_CAST = "org.freedesktop.portal.ScreenCast";
constexpr char const* REQUEST = "org.freedesktop.portal.Request";
constexpr uint32_t SOURCE_MONITOR = 1;
constexpr uint32_t CURSOR_HIDDEN = 1;
constexpr uint32_t CURSOR_EMBEDDED = 2;
constexpr uint32_t PERSIST_PERMANENTLY = 2;
constexpr guint REQUEST_TIMEOUT_MS = 30'000;
constexpr guint START_TIMEOUT_MS = 300'000;
constexpr wchar_t const* TOKEN_KEY = L"screenCastToken";

std::atomic<uint32_t> requests = 0;

struct Stream {
    uint32_t node = 0;
    std::optional<std::pair<int, int>> position;
};

class Portal {
public:
    Portal() : bus(g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr)), context(g_main_context_new()) {
        if (!bus) throw std::runtime_error("No session bus");
    }

    ~Portal() {
        closeSession();
        g_main_context_unref(context);
        g_object_unref(bus);
    }

    Portal(Portal const&) = delete;
    Portal& operator=(Portal const&) = delete;

    uint32_t cursorModes() {
        auto reply = g_dbus_connection_call_sync(bus, PORTAL, PORTAL_PATH, "org.freedesktop.DBus.Properties", "Get",
                                                 g_variant_new("(ss)", SCREEN_CAST, "AvailableCursorModes"),
                                                 G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, nullptr);
        if (!reply) return 0;

        GVariant* value = nullptr;
        g_variant_get(reply, "(v)", &value);
        uint32_t modes = g_variant_is_of_type(value, G_VARIANT_TYPE_UINT32) ? g_variant_get_uint32(value) : 0;
        g_variant_unref(value);
        g_variant_unref(reply);
        return modes;
    }

    void createSession() {
        auto token = nextToken();
        GVariantBuilder options;
        g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);
        g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string(token.c_str()));
        g_variant_builder_add(&options, "{sv}", "session_handle_token", g_variant_new_string(nextToken().c_str()));

        auto results = request("CreateSession", g_variant_new("(a{sv})", &options), token, REQUEST_TIMEOUT_MS);
        gchar const* handle = nullptr;
        if (g_variant_lookup(results, "session_handle", "&s", &handle)) session = handle;
        g_variant_unref(results);
        if (session.empty()) throw std::runtime_error("The portal returned no session");
    }

    void selectSources(bool cursor, std::string const& restoreToken) {
        auto token = nextToken();
        auto modes = cursorModes();
        GVariantBuilder options;
        g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);
        g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string(token.c_str()));
        g_variant_builder_add(&options, "{sv}", "types", g_variant_new_uint32(SOURCE_MONITOR));
        g_variant_builder_add(&options, "{sv}", "multiple", g_variant_new_boolean(FALSE));
        g_variant_builder_add(&options, "{sv}", "persist_mode", g_variant_new_uint32(PERSIST_PERMANENTLY));
        if (!restoreToken.empty()) {
            g_variant_builder_add(&options, "{sv}", "restore_token", g_variant_new_string(restoreToken.c_str()));
        }

        auto mode = cursor ? CURSOR_EMBEDDED : CURSOR_HIDDEN;
        if (modes & mode) g_variant_builder_add(&options, "{sv}", "cursor_mode", g_variant_new_uint32(mode));

        g_variant_unref(request("SelectSources", g_variant_new("(oa{sv})", session.c_str(), &options), token,
                                REQUEST_TIMEOUT_MS));
    }

    std::vector<Stream> start(std::string& restoreToken) {
        auto token = nextToken();
        GVariantBuilder options;
        g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);
        g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string(token.c_str()));

        auto results =
            request("Start", g_variant_new("(osa{sv})", session.c_str(), "", &options), token, START_TIMEOUT_MS);

        gchar const* restore = nullptr;
        if (g_variant_lookup(results, "restore_token", "&s", &restore)) restoreToken = restore;

        std::vector<Stream> streams;
        GVariantIter* iterator = nullptr;
        if (g_variant_lookup(results, "streams", "a(ua{sv})", &iterator)) {
            uint32_t node = 0;
            GVariant* properties = nullptr;
            while (g_variant_iter_next(iterator, "(u@a{sv})", &node, &properties)) {
                Stream stream{node, std::nullopt};
                int x = 0;
                int y = 0;
                if (g_variant_lookup(properties, "position", "(ii)", &x, &y)) stream.position = std::pair{x, y};
                streams.push_back(stream);
                g_variant_unref(properties);
            }
            g_variant_iter_free(iterator);
        }

        g_variant_unref(results);
        return streams;
    }

    int openRemote() {
        GVariantBuilder options;
        g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);
        GUnixFDList* fds = nullptr;
        GError* error = nullptr;

        auto reply = g_dbus_connection_call_with_unix_fd_list_sync(
            bus, PORTAL, PORTAL_PATH, SCREEN_CAST, "OpenPipeWireRemote",
            g_variant_new("(oa{sv})", session.c_str(), &options), G_VARIANT_TYPE("(h)"), G_DBUS_CALL_FLAGS_NONE, 10'000,
            nullptr, &fds, nullptr, &error);
        if (!reply) {
            std::string message = error ? error->message : "OpenPipeWireRemote failed";
            if (error) g_error_free(error);
            throw std::runtime_error(message);
        }

        int32_t index = 0;
        g_variant_get(reply, "(h)", &index);
        g_variant_unref(reply);

        int fd = g_unix_fd_list_get(fds, index, nullptr);
        g_object_unref(fds);
        if (fd < 0) throw std::runtime_error("The portal returned no PipeWire remote");
        return fd;
    }

    void closeSession() {
        if (session.empty()) return;
        auto reply = g_dbus_connection_call_sync(bus, PORTAL, session.c_str(), "org.freedesktop.portal.Session",
                                                 "Close", nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, 5000, nullptr,
                                                 nullptr);
        if (reply) g_variant_unref(reply);
        session.clear();
    }

private:
    struct Response {
        GMainLoop* loop = nullptr;
        bool isDone = false;
        uint32_t code = 2;
        GVariant* results = nullptr;
    };

    std::string nextToken() {
        return "fileship_cast_" + std::to_string(getpid()) + "_" + std::to_string(++requests);
    }

    std::string requestPath(std::string const& token) {
        std::string sender = g_dbus_connection_get_unique_name(bus) + 1;
        std::replace(sender.begin(), sender.end(), '.', '_');
        return std::string(PORTAL_PATH) + "/request/" + sender + "/" + token;
    }

    GVariant* request(char const* method, GVariant* parameters, std::string const& token, guint timeoutMs) {
        g_main_context_push_thread_default(context);

        Response response;
        response.loop = g_main_loop_new(context, FALSE);
        auto handler = [](GDBusConnection*, gchar const*, gchar const*, gchar const*, gchar const*,
                          GVariant* values, gpointer data) {
            auto& response = *static_cast<Response*>(data);
            g_variant_get(values, "(u@a{sv})", &response.code, &response.results);
            response.isDone = true;
            g_main_loop_quit(response.loop);
        };

        auto path = requestPath(token);
        auto subscription = g_dbus_connection_signal_subscribe(bus, PORTAL, REQUEST, "Response", path.c_str(), nullptr,
                                                               G_DBUS_SIGNAL_FLAGS_NONE, handler, &response, nullptr);

        GError* error = nullptr;
        auto reply = g_dbus_connection_call_sync(bus, PORTAL, PORTAL_PATH, SCREEN_CAST, method, parameters,
                                                 G_VARIANT_TYPE("(o)"), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);

        if (reply) {
            gchar const* handle = nullptr;
            g_variant_get(reply, "(&o)", &handle);
            if (path != handle) {
                g_dbus_connection_signal_unsubscribe(bus, subscription);
                path = handle;
                subscription = g_dbus_connection_signal_subscribe(bus, PORTAL, REQUEST, "Response", path.c_str(),
                                                                  nullptr, G_DBUS_SIGNAL_FLAGS_NONE, handler,
                                                                  &response, nullptr);
            }
            g_variant_unref(reply);

            auto timeout = g_timeout_source_new(timeoutMs);
            g_source_set_callback(
                timeout,
                [](gpointer loop) {
                    g_main_loop_quit(static_cast<GMainLoop*>(loop));
                    return G_SOURCE_REMOVE;
                },
                response.loop, nullptr);
            g_source_attach(timeout, context);
            if (!response.isDone) g_main_loop_run(response.loop);
            g_source_destroy(timeout);
            g_source_unref(timeout);

            if (!response.isDone) {
                auto closed = g_dbus_connection_call_sync(bus, PORTAL, path.c_str(), REQUEST, "Close", nullptr,
                                                          nullptr, G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, nullptr);
                if (closed) g_variant_unref(closed);
            }
        }

        g_dbus_connection_signal_unsubscribe(bus, subscription);
        while (g_main_context_iteration(context, FALSE)) {
        }
        g_main_context_pop_thread_default(context);
        g_main_loop_unref(response.loop);

        if (!reply) {
            std::string message = error ? error->message : "The portal call failed";
            if (error) g_error_free(error);
            throw std::runtime_error(message);
        }

        if (!response.isDone || response.code != 0) {
            if (response.results) g_variant_unref(response.results);
            throw std::runtime_error(
                toUtf8(t(response.code == 1 ? L"recording.screenCastDenied" : L"recording.screenCastFailed")));
        }

        return response.results;
    }

    GDBusConnection* bus = nullptr;
    GMainContext* context = nullptr;
    std::string session;
};

class FrameWriter {
public:
    FrameWriter(std::filesystem::path path, int fps, std::function<bool(std::vector<uint8_t>&)> latest)
        : path(std::move(path)), interval(std::chrono::microseconds(1'000'000 / std::max(1, fps))),
          latest(std::move(latest)), wakeFd(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)) {
        unlink(this->path.c_str());
        if (mkfifo(this->path.c_str(), 0600) != 0) {
            close(wakeFd);
            throw std::runtime_error("Could not create the video pipe (" + std::string(std::strerror(errno)) + ")");
        }
        thread = std::thread([this] { run(); });
    }

    ~FrameWriter() {
        isStopping = true;
        wake();
        int unblock = open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (thread.joinable()) thread.join();
        if (unblock >= 0) close(unblock);
        close(wakeFd);
        unlink(path.c_str());
    }

    FrameWriter(FrameWriter const&) = delete;
    FrameWriter& operator=(FrameWriter const&) = delete;

    void end() {
        isEnding = true;
        wake();
    }

private:
    void wake() {
        uint64_t one = 1;
        [[maybe_unused]] auto ignored = ::write(wakeFd, &one, sizeof(one));
    }

    bool sleepUntil(std::chrono::steady_clock::time_point time) {
        for (;;) {
            if (isStopping || isEnding) return false;
            auto left = std::chrono::duration_cast<std::chrono::milliseconds>(time - std::chrono::steady_clock::now());
            if (left.count() <= 0) return true;

            pollfd item{wakeFd, POLLIN, 0};
            poll(&item, 1, static_cast<int>(left.count()));
            uint64_t value = 0;
            [[maybe_unused]] auto ignored = read(wakeFd, &value, sizeof(value));
        }
    }

    bool writeAll(int fd, std::vector<uint8_t> const& frame) {
        size_t offset = 0;
        while (offset < frame.size()) {
            if (isStopping) return false;
            auto count = ::write(fd, frame.data() + offset, frame.size() - offset);
            if (count > 0) {
                offset += static_cast<size_t>(count);
                continue;
            }
            if (count < 0 && errno == EINTR) continue;
            if (count < 0 && errno == EAGAIN) {
                pollfd items[] = {{fd, POLLOUT, 0}, {wakeFd, POLLIN, 0}};
                poll(items, 2, -1);
                if (items[0].revents & (POLLERR | POLLHUP)) return false;
                continue;
            }
            return false;
        }
        return true;
    }

    void run() {
        int fd = -1;
        while ((fd = open(path.c_str(), O_WRONLY | O_CLOEXEC)) < 0 && errno == EINTR) {
        }
        if (fd < 0) return;
        if (isStopping) {
            close(fd);
            return;
        }

        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
        std::vector<uint8_t> frame;
        auto next = std::chrono::steady_clock::now();

        while (!isStopping) {
            if (latest(frame) && !writeAll(fd, frame)) break;
            next += interval;
            auto now = std::chrono::steady_clock::now();
            if (next < now) next = now;
            if (!sleepUntil(next)) break;
        }

        close(fd);
    }

    std::string path;
    std::chrono::microseconds interval;
    std::function<bool(std::vector<uint8_t>&)> latest;
    int wakeFd = -1;
    std::atomic<bool> isStopping = false;
    std::atomic<bool> isEnding = false;
    std::thread thread;
};

class ScreenCast : public VideoCapture {
public:
    ScreenCast(RecordTarget const& target, bool cursor, std::function<void()> onEnded) : onEnded(std::move(onEnded)) {
        try {
            connect(openSession(target, cursor));

            std::unique_lock lock(mutex);
            if (!frameReady.wait_for(lock, std::chrono::seconds(5), [&] { return !frame.empty() || isFailed; }) ||
                isFailed) {
                throw std::runtime_error(toUtf8(t(L"recording.screenCastFailed")));
            }
        } catch (...) {
            disconnect();
            throw;
        }
    }

    ~ScreenCast() override {
        writer.reset();
        disconnect();
    }

    FrameInfo frameInfo() const override {
        std::lock_guard lock(mutex);
        return {width, height, pixelFormat};
    }

    std::wstring startSegment(std::filesystem::path const& folder, std::wstring const& name, int fps) override {
        auto path = folder / (name + L".raw");
        writer = std::make_unique<FrameWriter>(path, fps, [this](std::vector<uint8_t>& target) {
            std::lock_guard lock(mutex);
            if (frame.empty()) return false;
            target = frame;
            return true;
        });
        return path.wstring();
    }

    void endSegment() override {
        if (writer) writer->end();
    }

    void closeSegment() override {
        writer.reset();
    }

private:
    Stream openSession(RecordTarget const& target, bool cursor) {
        auto stored = storeValue(TOKEN_KEY);
        std::string token = stored.ValueType() == JsonValueType::String ? toUtf8(std::wstring(stored.GetString())) : "";

        for (int attempt = 0; attempt < 2; attempt++) {
            portal = std::make_unique<Portal>();
            portal->createSession();
            portal->selectSources(cursor, attempt == 0 ? token : std::string());

            std::string restore;
            auto streams = portal->start(restore);
            if (streams.empty()) throw std::runtime_error(toUtf8(t(L"recording.screenCastFailed")));

            if (!restore.empty()) setStoreValue(TOKEN_KEY, JsonValue::CreateStringValue(toWide(restore)));

            auto stream = streams.front();
            if (holds(stream, target)) return stream;

            portal.reset();
            if (token.empty() || attempt == 1) break;
        }

        throw std::runtime_error(toUtf8(t(L"recording.screenCastMonitor")));
    }

    static bool holds(Stream const& stream, RecordTarget const& target) {
        if (!stream.position || !target.output) return true;

        auto displays = core::listDisplays();
        double scale = displays.empty() ? 1 : displays.front().scaleFactor;
        auto const& bounds = target.output->bounds;
        bool isMatch = std::abs(std::lround(stream.position->first * scale) - bounds.x) <= 2 &&
                       std::abs(std::lround(stream.position->second * scale) - bounds.y) <= 2;
        if (!isMatch) {
            logWarn(L"The shared screen at " + std::to_wstring(stream.position->first) + L"," +
                    std::to_wstring(stream.position->second) + L" does not hold the area on the display at " +
                    std::to_wstring(bounds.x) + L"," + std::to_wstring(bounds.y));
        }
        return isMatch;
    }

    void connect(Stream const& stream) {
        int fd = portal->openRemote();

        pw_init(nullptr, nullptr);
        loop = pw_thread_loop_new("fileship-cast", nullptr);
        context = pw_context_new(pw_thread_loop_get_loop(loop), nullptr, 0);
        if (!loop || !context || pw_thread_loop_start(loop) < 0) {
            close(fd);
            throw std::runtime_error(toUtf8(t(L"recording.screenCastFailed")));
        }

        pw_thread_loop_lock(loop);
        core = pw_context_connect_fd(context, fd, nullptr, 0);
        if (!core) {
            pw_thread_loop_unlock(loop);
            throw std::runtime_error(toUtf8(t(L"recording.screenCastFailed")));
        }

        node = stream.node;
        registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
        static pw_registry_events const registryEvents = [] {
            pw_registry_events value{};
            value.version = PW_VERSION_REGISTRY_EVENTS;
            value.global_remove = [](void* data, uint32_t id) {
                auto self = static_cast<ScreenCast*>(data);
                if (id != self->node) return;
                logInfo(L"The screen cast ended");
                self->handleEnded();
            };
            return value;
        }();
        pw_registry_add_listener(registry, &registryListener, &registryEvents, this);

        this->stream = pw_stream_new(core, "Fileship",
                                     pw_properties_new(PW_KEY_MEDIA_TYPE, "Video", PW_KEY_MEDIA_CATEGORY, "Capture",
                                                       PW_KEY_MEDIA_ROLE, "Screen", nullptr));

        static pw_stream_events const events = [] {
            pw_stream_events value{};
            value.version = PW_VERSION_STREAM_EVENTS;
            value.state_changed = [](void* data, pw_stream_state, pw_stream_state state, char const* error) {
                auto self = static_cast<ScreenCast*>(data);
                if (state != PW_STREAM_STATE_ERROR && state != PW_STREAM_STATE_UNCONNECTED) return;
                if (error) logWarn(L"Screen cast stream stopped: " + toWide(error));
                self->handleEnded();
            };
            value.param_changed = [](void* data, uint32_t id, spa_pod const* param) {
                static_cast<ScreenCast*>(data)->handleParam(id, param);
            };
            value.process = [](void* data) { static_cast<ScreenCast*>(data)->handleFrame(); };
            return value;
        }();
        pw_stream_add_listener(this->stream, &listener, &events, this);

        uint8_t buffer[1024];
        spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
        spa_rectangle defaultSize{1920, 1080};
        spa_rectangle minSize{1, 1};
        spa_rectangle maxSize{16384, 16384};
        spa_fraction defaultRate{60, 1};
        spa_fraction minRate{0, 1};
        spa_fraction maxRate{360, 1};
        spa_pod const* params[] = {reinterpret_cast<spa_pod const*>(spa_pod_builder_add_object(
            &builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat, SPA_FORMAT_mediaType,
            SPA_POD_Id(SPA_MEDIA_TYPE_video), SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
            SPA_FORMAT_VIDEO_format,
            SPA_POD_CHOICE_ENUM_Id(5, SPA_VIDEO_FORMAT_BGRx, SPA_VIDEO_FORMAT_BGRx, SPA_VIDEO_FORMAT_BGRA,
                                   SPA_VIDEO_FORMAT_RGBx, SPA_VIDEO_FORMAT_RGBA),
            SPA_FORMAT_VIDEO_size, SPA_POD_CHOICE_RANGE_Rectangle(&defaultSize, &minSize, &maxSize),
            SPA_FORMAT_VIDEO_framerate, SPA_POD_CHOICE_RANGE_Fraction(&defaultRate, &minRate, &maxRate)))};

        auto flags = static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT);
        int result = pw_stream_connect(this->stream, PW_DIRECTION_INPUT, stream.node, flags, params, 1);
        pw_thread_loop_unlock(loop);

        if (result < 0) throw std::runtime_error(toUtf8(t(L"recording.screenCastFailed")));
    }

    void handleParam(uint32_t id, spa_pod const* param) {
        if (!param || id != SPA_PARAM_Format) return;

        spa_video_info_raw info{};
        if (spa_format_video_raw_parse(param, &info) < 0) return;

        {
            std::lock_guard lock(mutex);
            width = static_cast<int>(info.size.width);
            height = static_cast<int>(info.size.height);
            format = info.format;
            pixelFormat = format == SPA_VIDEO_FORMAT_RGBx   ? L"rgb0"
                          : format == SPA_VIDEO_FORMAT_RGBA ? L"rgba"
                          : format == SPA_VIDEO_FORMAT_BGRA ? L"bgra"
                                                            : L"bgr0";
        }

        uint8_t buffer[256];
        spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
        spa_pod const* params[] = {reinterpret_cast<spa_pod const*>(spa_pod_builder_add_object(
            &builder, SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers, SPA_PARAM_BUFFERS_dataType,
            SPA_POD_CHOICE_FLAGS_Int((1 << SPA_DATA_MemPtr) | (1 << SPA_DATA_MemFd))))};
        pw_stream_update_params(stream, params, 1);
    }

    void handleFrame() {
        auto buffer = pw_stream_dequeue_buffer(stream);
        if (!buffer) return;

        auto const& data = buffer->buffer->datas[0];
        void* mapped = MAP_FAILED;
        size_t length = static_cast<size_t>(data.maxsize) + data.mapoffset;
        uint8_t const* base = nullptr;

        if (data.type == SPA_DATA_MemFd && data.fd >= 0) {
            mapped = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, static_cast<int>(data.fd), 0);
            if (mapped != MAP_FAILED) base = static_cast<uint8_t const*>(mapped) + data.mapoffset;
        } else if (data.type == SPA_DATA_MemPtr) {
            base = static_cast<uint8_t const*>(data.data);
        }

        if (base && data.chunk && data.chunk->size) {
            std::lock_guard lock(mutex);
            auto rowBytes = static_cast<size_t>(width) * 4;
            auto stride = data.chunk->stride > 0 ? static_cast<size_t>(data.chunk->stride) : rowBytes;
            auto source = base + data.chunk->offset;

            if (width > 0 && height > 0 && stride >= rowBytes &&
                data.chunk->offset + stride * (height - 1) + rowBytes <= data.maxsize) {
                frame.resize(rowBytes * height);
                for (int row = 0; row < height; row++) {
                    std::memcpy(frame.data() + row * rowBytes, source + row * stride, rowBytes);
                }
                frameReady.notify_all();
            }
        }

        if (mapped != MAP_FAILED) munmap(mapped, length);
        pw_stream_queue_buffer(stream, buffer);
    }

    void handleEnded() {
        bool wasStreaming = false;
        {
            std::lock_guard lock(mutex);
            wasStreaming = !frame.empty() && !isFailed;
            isFailed = true;
            frameReady.notify_all();
        }
        if (wasStreaming && onEnded) onEnded();
    }

    void disconnect() {
        if (loop) pw_thread_loop_lock(loop);
        if (registry) {
            spa_hook_remove(&registryListener);
            pw_proxy_destroy(reinterpret_cast<pw_proxy*>(registry));
            registry = nullptr;
        }
        if (stream) {
            spa_hook_remove(&listener);
            pw_stream_disconnect(stream);
            pw_stream_destroy(stream);
            stream = nullptr;
        }
        if (core) {
            pw_core_disconnect(core);
            core = nullptr;
        }
        if (loop) pw_thread_loop_unlock(loop);
        if (loop) pw_thread_loop_stop(loop);
        if (context) {
            pw_context_destroy(context);
            context = nullptr;
        }
        if (loop) {
            pw_thread_loop_destroy(loop);
            loop = nullptr;
        }
        portal.reset();
    }

    std::unique_ptr<Portal> portal;
    pw_thread_loop* loop = nullptr;
    pw_context* context = nullptr;
    pw_core* core = nullptr;
    pw_stream* stream = nullptr;
    spa_hook listener{};
    pw_registry* registry = nullptr;
    spa_hook registryListener{};
    uint32_t node = 0;
    mutable std::mutex mutex;
    std::condition_variable frameReady;
    std::vector<uint8_t> frame;
    int width = 0;
    int height = 0;
    spa_video_format format = SPA_VIDEO_FORMAT_BGRx;
    std::wstring pixelFormat = L"bgr0";
    bool isFailed = false;
    std::function<void()> onEnded;
    std::unique_ptr<FrameWriter> writer;
};

}

std::unique_ptr<VideoCapture> openVideoCapture(VideoSource source, RecordTarget const& target, bool cursor,
                                               std::function<void()> onEnded) {
    if (source != VideoSource::Pipewire) return nullptr;

    try {
        return std::make_unique<ScreenCast>(target, cursor, std::move(onEnded));
    } catch (std::exception const& error) {
        logError(L"Could not start the screen cast: " + toWide(error.what()));
        throw;
    }
}

}

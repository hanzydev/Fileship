#include <host/recording/audio.h>

#include <shared/log.h>
#include <shared/utf8.h>

#include <pulse/pulseaudio.h>

#include <chrono>
#include <condition_variable>
#include <stdexcept>

namespace fileship::host::recording {

namespace {

constexpr char const* SYSTEM_AUDIO = "@DEFAULT_MONITOR@";

void signalLoop(pa_threaded_mainloop* loop) {
    pa_threaded_mainloop_signal(loop, 0);
}

class PulseSource : public CaptureSource {
public:
    explicit PulseSource(CaptureOptions const& options) : loop(pa_threaded_mainloop_new()) {
        if (!loop) throw std::runtime_error("Could not start the PulseAudio loop");

        context = pa_context_new(pa_threaded_mainloop_get_api(loop), "Fileship");
        pa_context_set_state_callback(
            context, [](pa_context*, void* data) { signalLoop(static_cast<pa_threaded_mainloop*>(data)); }, loop);

        pa_threaded_mainloop_lock(loop);
        bool isReady = pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) >= 0 &&
                       pa_threaded_mainloop_start(loop) >= 0 && waitForContext();

        auto device = options.loopback ? std::string(SYSTEM_AUDIO) : toUtf8(options.deviceId);
        if (isReady) {
            isReady = connect(device, options.loopback);
            if (!isReady && !options.loopback && !device.empty()) {
                logWarn(L"Could not open the chosen microphone, using the default one");
                isReady = connect({}, false);
            }
        }
        pa_threaded_mainloop_unlock(loop);

        if (!isReady) {
            auto error = toWide(pa_strerror(pa_context_errno(context)));
            close();
            throw std::runtime_error(toUtf8(L"Could not open the audio source: " + error));
        }
    }

    ~PulseSource() override {
        close();
    }

private:
    bool waitForContext() {
        for (;;) {
            auto state = pa_context_get_state(context);
            if (state == PA_CONTEXT_READY) return true;
            if (!PA_CONTEXT_IS_GOOD(state)) return false;
            pa_threaded_mainloop_wait(loop);
        }
    }

    bool connect(std::string const& device, bool isLoopback) {
        if (stream) {
            pa_stream_disconnect(stream);
            pa_stream_unref(stream);
            stream = nullptr;
        }

        pa_sample_spec spec{PA_SAMPLE_FLOAT32LE, SAMPLE_RATE, CHANNELS};
        stream = pa_stream_new(context, isLoopback ? "System audio" : "Microphone", &spec, nullptr);
        if (!stream) return false;

        pa_stream_set_state_callback(
            stream, [](pa_stream*, void* data) { signalLoop(static_cast<pa_threaded_mainloop*>(data)); }, loop);
        pa_stream_set_read_callback(stream, handleRead, this);

        pa_buffer_attr attributes{};
        attributes.maxlength = static_cast<uint32_t>(-1);
        attributes.fragsize = static_cast<uint32_t>(pa_usec_to_bytes(10'000, &spec));

        if (pa_stream_connect_record(stream, device.empty() ? nullptr : device.c_str(), &attributes,
                                     PA_STREAM_ADJUST_LATENCY) < 0)
            return false;

        for (;;) {
            auto state = pa_stream_get_state(stream);
            if (state == PA_STREAM_READY) return true;
            if (!PA_STREAM_IS_GOOD(state)) return false;
            pa_threaded_mainloop_wait(loop);
        }
    }

    static void handleRead(pa_stream* stream, size_t, void* data) {
        auto self = static_cast<PulseSource*>(data);

        while (pa_stream_readable_size(stream) > 0) {
            void const* bytes = nullptr;
            size_t size = 0;
            if (pa_stream_peek(stream, &bytes, &size) < 0 || !size) return;

            self->push(static_cast<float const*>(bytes), size / sizeof(float));
            pa_stream_drop(stream);
        }
    }

    void close() {
        if (!loop) return;

        pa_threaded_mainloop_lock(loop);
        if (stream) {
            pa_stream_set_read_callback(stream, nullptr, nullptr);
            pa_stream_disconnect(stream);
            pa_stream_unref(stream);
            stream = nullptr;
        }
        if (context) {
            pa_context_disconnect(context);
            pa_context_unref(context);
            context = nullptr;
        }
        pa_threaded_mainloop_unlock(loop);

        pa_threaded_mainloop_stop(loop);
        pa_threaded_mainloop_free(loop);
        loop = nullptr;
    }

    pa_threaded_mainloop* loop = nullptr;
    pa_context* context = nullptr;
    pa_stream* stream = nullptr;
};

}

std::unique_ptr<CaptureSource> openCaptureSource(CaptureOptions const& options) {
    try {
        return std::make_unique<PulseSource>(options);
    } catch (std::exception const& error) {
        logError(std::wstring(L"Could not open the ") + (options.loopback ? L"system audio" : L"microphone") +
                 L": " + toWide(error.what()));
        throw;
    }
}

struct MixClock::State {
    std::mutex mutex;
    std::condition_variable signal;
    bool isStopped = false;
};

MixClock::MixClock() : state(std::make_unique<State>()) {}

MixClock::~MixClock() = default;

bool MixClock::wait() {
    std::unique_lock lock(state->mutex);
    state->signal.wait_for(lock, std::chrono::milliseconds(10), [&] { return state->isStopped; });
    return !state->isStopped;
}

void MixClock::stop() {
    {
        std::lock_guard lock(state->mutex);
        state->isStopped = true;
    }
    state->signal.notify_all();
}

JsonArray listMicrophones() {
    JsonArray result;
    auto loop = pa_mainloop_new();
    auto context = pa_context_new(pa_mainloop_get_api(loop), "Fileship");

    struct Listing {
        JsonArray* result;
        bool isDone = false;
    } listing{&result};

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    bool isRequested = false;

    if (pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) >= 0) {
        while (!listing.isDone && std::chrono::steady_clock::now() < deadline) {
            auto state = pa_context_get_state(context);
            if (!PA_CONTEXT_IS_GOOD(state)) break;

            if (state == PA_CONTEXT_READY && !isRequested) {
                isRequested = true;
                pa_operation_unref(pa_context_get_source_info_list(
                    context,
                    [](pa_context*, pa_source_info const* info, int isLast, void* data) {
                        auto& listing = *static_cast<Listing*>(data);
                        if (isLast) {
                            listing.isDone = true;
                            return;
                        }
                        if (info->monitor_of_sink != PA_INVALID_INDEX) return;

                        JsonObject item;
                        item.SetNamedValue(L"id", JsonValue::CreateStringValue(toWide(info->name)));
                        item.SetNamedValue(L"name", JsonValue::CreateStringValue(
                                                        toWide(info->description ? info->description : info->name)));
                        listing.result->Append(item);
                    },
                    &listing));
            }

            if (pa_mainloop_prepare(loop, 50'000) < 0 || pa_mainloop_poll(loop) < 0 || pa_mainloop_dispatch(loop) < 0)
                break;
        }
    }

    if (!listing.isDone) logWarn(L"Could not list the microphones");

    pa_context_disconnect(context);
    pa_context_unref(context);
    pa_mainloop_free(loop);
    return result;
}

}

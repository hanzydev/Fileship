#include <host/recording/audio.h>
#include <shared/i18n.h>
#include <shared/log.h>
#include <shared/utf8.h>

#include <pulse/pulseaudio.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace fileship::host::recording {

namespace {

constexpr int CHANNELS = 2;
constexpr int64_t CUSHION_FRAMES = SAMPLE_RATE * 30 / 1000;
constexpr int64_t MAX_BACKLOG_FRAMES = SAMPLE_RATE * 100 / 1000;
constexpr int64_t MIN_CHUNK_FRAMES = SAMPLE_RATE * 10 / 1000;
constexpr size_t FRAGMENT_FRAMES = SAMPLE_RATE * 10 / 1000;

double now() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

struct Lock {
    explicit Lock(pa_threaded_mainloop* loop) : loop(loop) {
        pa_threaded_mainloop_lock(loop);
    }

    ~Lock() {
        pa_threaded_mainloop_unlock(loop);
    }

    pa_threaded_mainloop* loop;
};

void wake(pa_threaded_mainloop* loop) {
    pa_threaded_mainloop_signal(loop, 0);
}

class Pulse {
public:
    Pulse() {
        loop = pa_threaded_mainloop_new();
        context = pa_context_new(pa_threaded_mainloop_get_api(loop), "Fileship");
        pa_context_set_state_callback(
            context, [](pa_context*, void* data) { wake(static_cast<pa_threaded_mainloop*>(data)); }, loop);
        pa_threaded_mainloop_start(loop);

        Lock lock(loop);
        pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr);

        for (auto state = pa_context_get_state(context); state != PA_CONTEXT_READY;
             state = pa_context_get_state(context)) {
            if (!PA_CONTEXT_IS_GOOD(state)) throw std::runtime_error(pa_strerror(pa_context_errno(context)));
            pa_threaded_mainloop_wait(loop);
        }
    }

    ~Pulse() {
        {
            Lock lock(loop);
            pa_context_disconnect(context);
            pa_context_unref(context);
        }

        pa_threaded_mainloop_stop(loop);
        pa_threaded_mainloop_free(loop);
    }

    Pulse(Pulse const&) = delete;
    Pulse& operator=(Pulse const&) = delete;

    pa_threaded_mainloop* loop = nullptr;
    pa_context* context = nullptr;
};

class CaptureSource {
public:
    CaptureSource(Pulse& pulse, char const* device, char const* description) : pulse(pulse) {
        pa_sample_spec format{PA_SAMPLE_FLOAT32LE, SAMPLE_RATE, CHANNELS};
        pa_buffer_attr buffer{};
        buffer.maxlength = static_cast<uint32_t>(-1);
        buffer.fragsize = static_cast<uint32_t>(FRAGMENT_FRAMES * CHANNELS * sizeof(float));

        Lock lock(pulse.loop);
        stream = pa_stream_new(pulse.context, description, &format, nullptr);
        pa_stream_set_state_callback(
            stream, [](pa_stream*, void* data) { wake(static_cast<pa_threaded_mainloop*>(data)); }, pulse.loop);
        pa_stream_set_read_callback(stream, handleRead, this);
        pa_stream_connect_record(stream, device, &buffer, PA_STREAM_ADJUST_LATENCY);

        for (auto state = pa_stream_get_state(stream); state != PA_STREAM_READY; state = pa_stream_get_state(stream)) {
            if (!PA_STREAM_IS_GOOD(state)) {
                std::string message = pa_strerror(pa_context_errno(pulse.context));
                pa_stream_unref(stream);
                throw std::runtime_error(message);
            }

            pa_threaded_mainloop_wait(pulse.loop);
        }
    }

    ~CaptureSource() {
        Lock lock(pulse.loop);
        pa_stream_set_read_callback(stream, nullptr, nullptr);
        pa_stream_set_state_callback(stream, nullptr, nullptr);
        pa_stream_disconnect(stream);
        pa_stream_unref(stream);
    }

    CaptureSource(CaptureSource const&) = delete;
    CaptureSource& operator=(CaptureSource const&) = delete;

    void take(float* target, int64_t frames) {
        std::lock_guard lock(mutex);

        auto available = static_cast<int64_t>(fifo.size() / CHANNELS);
        if (available > frames + MAX_BACKLOG_FRAMES) {
            fifo.erase(fifo.begin(), fifo.begin() + (available - frames - CUSHION_FRAMES) * CHANNELS);
            available = static_cast<int64_t>(fifo.size() / CHANNELS);
        }

        auto count = std::min(available, frames) * CHANNELS;
        std::copy(fifo.begin(), fifo.begin() + count, target);
        std::fill(target + count, target + frames * CHANNELS, 0.0f);
        fifo.erase(fifo.begin(), fifo.begin() + count);
    }

private:
    static void handleRead(pa_stream* stream, size_t, void* data) {
        auto& self = *static_cast<CaptureSource*>(data);
        void const* chunk = nullptr;
        size_t bytes = 0;

        while (pa_stream_peek(stream, &chunk, &bytes) == 0 && bytes) {
            {
                std::lock_guard lock(self.mutex);
                auto samples = static_cast<float const*>(chunk);
                auto count = bytes / sizeof(float);

                if (samples) {
                    self.fifo.insert(self.fifo.end(), samples, samples + count);
                } else {
                    self.fifo.insert(self.fifo.end(), count, 0.0f);
                }

                auto limit = static_cast<size_t>(SAMPLE_RATE) * CHANNELS;
                if (self.fifo.size() > limit)
                    self.fifo.erase(self.fifo.begin(), self.fifo.begin() + (self.fifo.size() - limit));
            }

            pa_stream_drop(stream);
        }
    }

    Pulse& pulse;
    pa_stream* stream = nullptr;
    std::mutex mutex;
    std::deque<float> fifo;
};

class PulseEngine : public AudioEngine {
public:
    PulseEngine(JsonObject const& audio, std::vector<AudioTrack> const& tracks, ChunkHandler handler)
        : onChunk(std::move(handler)) {
        for (auto const& track : tracks) {
            std::vector<size_t> indexes;

            for (auto const& source : track.sources) {
                size_t index = 0;
                while (index < inputs.size() && inputs[index]->id != source) index++;

                if (index == inputs.size()) {
                    auto input = std::make_unique<Input>();
                    input->id = source;
                    inputs.push_back(std::move(input));
                }

                indexes.push_back(index);
            }

            trackInputs.push_back(indexes);
        }

        for (auto& input : inputs) {
            bool isMicrophone = input->id == L"microphone";
            input->volume = number(audio, isMicrophone ? L"microphoneVolume" : L"systemVolume", 100) / 100.0f;

            try {
                input->capture =
                    isMicrophone ? openMicrophone(toUtf8(text(audio, L"microphoneId")))
                                 : std::make_unique<CaptureSource>(pulse, "@DEFAULT_MONITOR@", "System audio");
            } catch (std::exception const& error) {
                logError(std::wstring(L"Could not open the ") + (isMicrophone ? L"microphone" : L"system audio") +
                         L": " + toWide(error.what()));
                throw std::runtime_error(
                    toUtf8(t(isMicrophone ? L"recorder.microphoneUnavailable" : L"recorder.systemAudioUnavailable")));
            }
        }

        mixer = std::thread([this] { mix(); });
    }

    ~PulseEngine() override {
        isStopping = true;
        mixer.join();
    }

    void setMuted(std::wstring const& source, bool muted) override {
        for (auto& input : inputs) {
            if (input->id == source) input->muted = muted;
        }
    }

private:
    struct Input {
        std::wstring id;
        std::unique_ptr<CaptureSource> capture;
        float volume = 1;
        std::atomic<bool> muted = false;
        std::vector<float> buffer;
    };

    std::unique_ptr<CaptureSource> openMicrophone(std::string const& chosen) {
        if (chosen.empty()) return std::make_unique<CaptureSource>(pulse, nullptr, "Microphone");

        try {
            return std::make_unique<CaptureSource>(pulse, chosen.c_str(), "Microphone");
        } catch (std::exception const&) {
            logWarn(L"Could not open the chosen microphone, using the default one");
            return std::make_unique<CaptureSource>(pulse, nullptr, "Microphone");
        }
    }

    void mix() {
        auto startedAt = now();
        int64_t produced = 0;
        std::vector<float> output;

        while (!isStopping) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            auto target = static_cast<int64_t>((now() - startedAt) * SAMPLE_RATE) - CUSHION_FRAMES;
            auto frames = target - produced;
            if (frames < MIN_CHUNK_FRAMES) continue;

            produced += frames;
            auto samples = static_cast<size_t>(frames) * CHANNELS;

            for (auto& input : inputs) {
                input->buffer.resize(samples);
                input->capture->take(input->buffer.data(), frames);

                float gain = input->muted ? 0.0f : input->volume;
                for (auto& sample : input->buffer) sample *= gain;
            }

            for (size_t track = 0; track < trackInputs.size(); track++) {
                output.assign(samples, 0.0f);

                for (auto index : trackInputs[track]) {
                    auto const& buffer = inputs[index]->buffer;
                    for (size_t sample = 0; sample < samples; sample++) output[sample] += buffer[sample];
                }

                onChunk(track, output);
            }
        }
    }

    Pulse pulse;
    std::vector<std::unique_ptr<Input>> inputs;
    std::vector<std::vector<size_t>> trackInputs;
    ChunkHandler onChunk;
    std::atomic<bool> isStopping = false;
    std::thread mixer;
};

struct Listing {
    JsonArray sources;
    bool isDone = false;
};

void handleSource(pa_context*, pa_source_info const* source, int isLast, void* data) {
    auto& listing = *static_cast<Listing*>(data);

    if (isLast) {
        listing.isDone = true;
        return;
    }

    if (source->monitor_of_sink != PA_INVALID_INDEX) return;

    JsonObject item;
    item.SetNamedValue(L"id", JsonValue::CreateStringValue(toWide(source->name)));
    item.SetNamedValue(L"name", JsonValue::CreateStringValue(toWide(source->description)));
    listing.sources.Append(item);
}

}

std::unique_ptr<AudioEngine> AudioEngine::create(JsonObject const& audio, std::vector<AudioTrack> const& tracks,
                                                 ChunkHandler onChunk) {
    return std::make_unique<PulseEngine>(audio, tracks, std::move(onChunk));
}

JsonArray listMicrophones() {
    Listing listing;
    auto loop = pa_mainloop_new();
    auto context = pa_context_new(pa_mainloop_get_api(loop), "Fileship");
    pa_operation* operation = nullptr;

    pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr);

    while (!listing.isDone && pa_mainloop_iterate(loop, 1, nullptr) >= 0) {
        auto state = pa_context_get_state(context);
        if (!PA_CONTEXT_IS_GOOD(state)) break;
        if (state == PA_CONTEXT_READY && !operation)
            operation = pa_context_get_source_info_list(context, handleSource, &listing);
    }

    if (!listing.isDone) logWarn(L"Could not list the microphones");
    if (operation) pa_operation_unref(operation);
    pa_context_disconnect(context);
    pa_context_unref(context);
    pa_mainloop_free(loop);

    return listing.sources;
}

}

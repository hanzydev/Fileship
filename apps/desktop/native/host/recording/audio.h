#pragma once

#include "command.h"

#include <shared/json.h>

#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace fileship::host::recording {

constexpr int CHANNELS = 2;

struct CaptureOptions {
    bool loopback = false;
    std::wstring deviceId;
    bool noiseSuppression = false;
    bool echoCancellation = false;
    bool autoGainControl = false;
};

class CaptureSource {
public:
    CaptureSource() = default;
    virtual ~CaptureSource() = default;

    CaptureSource(CaptureSource const&) = delete;
    CaptureSource& operator=(CaptureSource const&) = delete;

    void take(float* target, int64_t frames);

protected:
    void push(float const* samples, size_t count);

private:
    std::mutex mutex;
    std::deque<float> fifo;
};

std::unique_ptr<CaptureSource> openCaptureSource(CaptureOptions const& options);
JsonArray listMicrophones();

class MixClock {
public:
    MixClock();
    ~MixClock();

    MixClock(MixClock const&) = delete;
    MixClock& operator=(MixClock const&) = delete;

    bool wait();
    void stop();

private:
    struct State;

    std::unique_ptr<State> state;
};

class AudioEngine {
public:
    using ChunkHandler = std::function<void(size_t track, std::vector<float> const& samples)>;

    AudioEngine(JsonObject const& audio, std::vector<AudioTrack> const& tracks, ChunkHandler onChunk);
    ~AudioEngine();

    AudioEngine(AudioEngine const&) = delete;
    AudioEngine& operator=(AudioEngine const&) = delete;

    void setMuted(std::wstring const& source, bool muted);

private:
    struct Input {
        std::wstring id;
        std::unique_ptr<CaptureSource> capture;
        float volume = 1;
        std::atomic<bool> muted = false;
        std::vector<float> buffer;
    };

    void mix();

    std::vector<std::unique_ptr<Input>> inputs;
    std::vector<std::vector<size_t>> trackInputs;
    ChunkHandler onChunk;
    std::unique_ptr<MixClock> clock;
    std::thread mixer;
};

}

#pragma once

#include "command.h"

#include <shared/json.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace fileship::host::recording {

class CaptureSource;

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
    HANDLE stopEvent = nullptr;
    std::thread mixer;
};

JsonArray listMicrophones();

}

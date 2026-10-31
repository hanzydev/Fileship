#include "audio.h"

#include <shared/i18n.h>
#include <shared/utf8.h>

#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace fileship::host::recording {

namespace {

constexpr int64_t CUSHION_FRAMES = SAMPLE_RATE * 30 / 1000;
constexpr int64_t MAX_BACKLOG_FRAMES = SAMPLE_RATE * 100 / 1000;
constexpr int64_t MIN_CHUNK_FRAMES = SAMPLE_RATE * 10 / 1000;

double now() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

}

void CaptureSource::take(float* target, int64_t frames) {
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

void CaptureSource::push(float const* samples, size_t count) {
    std::lock_guard lock(mutex);

    if (samples) {
        fifo.insert(fifo.end(), samples, samples + count);
    } else {
        fifo.insert(fifo.end(), count, 0.0f);
    }

    auto limit = static_cast<size_t>(SAMPLE_RATE) * CHANNELS;
    if (fifo.size() > limit) fifo.erase(fifo.begin(), fifo.begin() + (fifo.size() - limit));
}

AudioEngine::AudioEngine(JsonObject const& audio, std::vector<AudioTrack> const& tracks, ChunkHandler handler)
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

    if (inputs.empty()) return;

    for (auto& input : inputs) {
        bool isMicrophone = input->id == L"microphone";
        CaptureOptions options;
        options.loopback = !isMicrophone;

        if (isMicrophone) {
            options.deviceId = text(audio, L"microphoneId");
            options.noiseSuppression = flag(audio, L"noiseSuppression", true);
            options.echoCancellation = flag(audio, L"echoCancellation", false);
            options.autoGainControl = flag(audio, L"autoGainControl", false);
        }

        input->volume = number(audio, isMicrophone ? L"microphoneVolume" : L"systemVolume", 100) / 100.0f;

        try {
            input->capture = openCaptureSource(options);
        } catch (...) {
            throw std::runtime_error(
                toUtf8(t(isMicrophone ? L"recorder.microphoneUnavailable" : L"recorder.systemAudioUnavailable")));
        }
    }

    clock = std::make_unique<MixClock>();
    mixer = std::thread([this] { mix(); });
}

AudioEngine::~AudioEngine() {
    if (clock) clock->stop();
    if (mixer.joinable()) mixer.join();
}

void AudioEngine::setMuted(std::wstring const& source, bool muted) {
    for (auto& input : inputs) {
        if (input->id == source) input->muted = muted;
    }
}

void AudioEngine::mix() {
    auto startedAt = now();
    int64_t produced = 0;
    std::vector<float> output;

    while (clock->wait()) {
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

}

#include "audio.h"

#include <shared/i18n.h>
#include <shared/log.h>
#include <shared/utf8.h>

#include <audioclient.h>
#include <audiopolicy.h>
#include <avrt.h>
#include <ks.h>
#include <ksmedia.h>
#include <mmdeviceapi.h>
#include <winrt/base.h>

#include <deque>
#include <future>
#include <mutex>
#include <stdexcept>

namespace fileship::host::recording {

namespace {

using winrt::com_ptr;

constexpr int CHANNELS = 2;
constexpr int64_t CUSHION_FRAMES = SAMPLE_RATE * 30 / 1000;
constexpr int64_t MAX_BACKLOG_FRAMES = SAMPLE_RATE * 100 / 1000;
constexpr int64_t MIN_CHUNK_FRAMES = SAMPLE_RATE * 10 / 1000;
constexpr REFERENCE_TIME BUFFER_DURATION = 2'000'000;

GUID const ECHO_CANCELLATION{STATIC_AUDIO_EFFECT_TYPE_ACOUSTIC_ECHO_CANCELLATION};
GUID const NOISE_SUPPRESSION{STATIC_AUDIO_EFFECT_TYPE_NOISE_SUPPRESSION};
GUID const DEEP_NOISE_SUPPRESSION{STATIC_AUDIO_EFFECT_TYPE_DEEP_NOISE_SUPPRESSION};
GUID const GAIN_CONTROL{STATIC_AUDIO_EFFECT_TYPE_AUTOMATIC_GAIN_CONTROL};
PROPERTYKEY const FRIENDLY_NAME{{0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}}, 14};

struct ComScope {
    HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    ComScope() = default;
    ~ComScope() {
        if (SUCCEEDED(result)) CoUninitialize();
    }

    ComScope(ComScope const&) = delete;
    ComScope& operator=(ComScope const&) = delete;
};

struct MmcssScope {
    HANDLE task = nullptr;

    MmcssScope() {
        DWORD index = 0;
        task = AvSetMmThreadCharacteristicsW(L"Audio", &index);
    }

    ~MmcssScope() {
        if (task) AvRevertMmThreadCharacteristics(task);
    }

    MmcssScope(MmcssScope const&) = delete;
    MmcssScope& operator=(MmcssScope const&) = delete;
};

com_ptr<IMMDeviceEnumerator> deviceEnumerator() {
    return winrt::create_instance<IMMDeviceEnumerator>(__uuidof(MMDeviceEnumerator));
}

WAVEFORMATEXTENSIBLE floatFormat() {
    WAVEFORMATEXTENSIBLE format{};
    format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    format.Format.nChannels = CHANNELS;
    format.Format.nSamplesPerSec = SAMPLE_RATE;
    format.Format.wBitsPerSample = 32;
    format.Format.nBlockAlign = CHANNELS * 4;
    format.Format.nAvgBytesPerSec = SAMPLE_RATE * CHANNELS * 4;
    format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    format.Samples.wValidBitsPerSample = 32;
    format.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    format.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    return format;
}

double now() {
    static LARGE_INTEGER const frequency = [] {
        LARGE_INTEGER value{};
        QueryPerformanceFrequency(&value);
        return value;
    }();

    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    return static_cast<double>(counter.QuadPart) / static_cast<double>(frequency.QuadPart);
}

}

struct CaptureOptions {
    bool loopback = false;
    std::wstring deviceId;
    bool noiseSuppression = false;
    bool echoCancellation = false;
    bool autoGainControl = false;
};

class CaptureSource {
public:
    explicit CaptureSource(CaptureOptions options) : options(std::move(options)) {
        stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

        std::promise<void> started;
        auto ready = started.get_future();
        thread = std::thread([this, started = std::move(started)]() mutable { run(started); });

        try {
            ready.get();
        } catch (...) {
            thread.join();
            CloseHandle(stopEvent);
            throw;
        }
    }

    ~CaptureSource() {
        SetEvent(stopEvent);
        if (thread.joinable()) thread.join();
        CloseHandle(stopEvent);
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
    com_ptr<IAudioClient3> open(IMMDeviceEnumerator* enumerator, bool useDefault) {
        com_ptr<IMMDevice> device;

        if (options.loopback) {
            winrt::check_hresult(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, device.put()));
        } else if (useDefault || options.deviceId.empty()) {
            winrt::check_hresult(enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, device.put()));
        } else {
            winrt::check_hresult(enumerator->GetDevice(options.deviceId.c_str(), device.put()));
        }

        com_ptr<IAudioClient3> client;
        winrt::check_hresult(device->Activate(__uuidof(IAudioClient3), CLSCTX_ALL, nullptr, client.put_void()));

        bool processing = options.noiseSuppression || options.echoCancellation || options.autoGainControl;
        AudioClientProperties properties{};
        properties.cbSize = sizeof(properties);
        properties.eCategory = !options.loopback && processing ? AudioCategory_Communications : AudioCategory_Other;
        winrt::check_hresult(client->SetClientProperties(&properties));

        auto format = floatFormat();
        DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
                      AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
        if (options.loopback) flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;

        winrt::check_hresult(client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, BUFFER_DURATION, 0,
                                                reinterpret_cast<WAVEFORMATEX*>(&format), nullptr));
        return client;
    }

    void configure(IAudioClient3* client) {
        com_ptr<IAudioSessionControl> control;
        if (SUCCEEDED(client->GetService(__uuidof(IAudioSessionControl), control.put_void()))) {
            if (auto session = control.try_as<IAudioSessionControl2>()) session->SetDuckingPreference(TRUE);
        }

        if (options.loopback) return;

        com_ptr<IAudioEffectsManager> effects;
        if (FAILED(client->GetService(__uuidof(IAudioEffectsManager), effects.put_void()))) return;

        AUDIO_EFFECT* list = nullptr;
        UINT32 count = 0;
        if (FAILED(effects->GetAudioEffects(&list, &count))) return;

        for (UINT32 index = 0; index < count; index++) {
            auto const& effect = list[index];
            std::optional<bool> wanted;

            if (effect.id == ECHO_CANCELLATION) wanted = options.echoCancellation;
            if (effect.id == NOISE_SUPPRESSION || effect.id == DEEP_NOISE_SUPPRESSION) {
                wanted = options.noiseSuppression;
            }
            if (effect.id == GAIN_CONTROL) wanted = options.autoGainControl;
            if (!wanted || !effect.canSetState) continue;

            auto state = *wanted ? AUDIO_EFFECT_STATE_ON : AUDIO_EFFECT_STATE_OFF;
            if (effect.state != state) effects->SetAudioEffectState(effect.id, state);
        }

        CoTaskMemFree(list);
    }

    void run(std::promise<void>& started) {
        ComScope com;
        com_ptr<IAudioClient3> client;
        com_ptr<IAudioCaptureClient> capture;
        HANDLE sampleEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

        try {
            auto enumerator = deviceEnumerator();

            try {
                client = open(enumerator.get(), false);
            } catch (...) {
                if (options.loopback || options.deviceId.empty()) throw;
                logWarn(L"Could not open the chosen microphone, using the default one");
                client = open(enumerator.get(), true);
            }

            configure(client.get());
            winrt::check_hresult(client->SetEventHandle(sampleEvent));
            winrt::check_hresult(client->GetService(__uuidof(IAudioCaptureClient), capture.put_void()));
            winrt::check_hresult(client->Start());
        } catch (winrt::hresult_error const& error) {
            logError(std::wstring(L"Could not open the ") + (options.loopback ? L"system audio" : L"microphone") +
                     L": " + std::wstring(error.message()));
            CloseHandle(sampleEvent);
            started.set_exception(std::make_exception_ptr(std::runtime_error("open")));
            return;
        }

        started.set_value();

        MmcssScope mmcss;
        HANDLE events[] = {stopEvent, sampleEvent};

        while (WaitForMultipleObjects(2, events, FALSE, 100) != WAIT_OBJECT_0) {
            UINT32 packet = 0;
            HRESULT result = S_OK;

            while (SUCCEEDED(result = capture->GetNextPacketSize(&packet)) && packet) {
                BYTE* data = nullptr;
                UINT32 frames = 0;
                DWORD flags = 0;

                result = capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
                if (FAILED(result)) break;

                {
                    std::lock_guard lock(mutex);
                    auto samples = reinterpret_cast<float const*>(data);
                    auto count = static_cast<size_t>(frames) * CHANNELS;

                    if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                        fifo.insert(fifo.end(), count, 0.0f);
                    } else {
                        fifo.insert(fifo.end(), samples, samples + count);
                    }

                    auto limit = static_cast<size_t>(SAMPLE_RATE) * CHANNELS;
                    if (fifo.size() > limit) fifo.erase(fifo.begin(), fifo.begin() + (fifo.size() - limit));
                }

                capture->ReleaseBuffer(frames);
            }

            if (FAILED(result)) {
                logWarn(std::wstring(L"Audio capture stopped: ") + (options.loopback ? L"system" : L"microphone") +
                        L" " + std::to_wstring(result));
                break;
            }
        }

        client->Stop();
        CloseHandle(sampleEvent);
    }

    CaptureOptions options;
    HANDLE stopEvent = nullptr;
    std::thread thread;
    std::mutex mutex;
    std::deque<float> fifo;
};

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
            input->capture = std::make_unique<CaptureSource>(options);
        } catch (...) {
            throw std::runtime_error(
                toUtf8(t(isMicrophone ? L"recorder.microphoneUnavailable" : L"recorder.systemAudioUnavailable")));
        }
    }

    stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    mixer = std::thread([this] { mix(); });
}

AudioEngine::~AudioEngine() {
    if (stopEvent) SetEvent(stopEvent);
    if (mixer.joinable()) mixer.join();
    if (stopEvent) CloseHandle(stopEvent);
}

void AudioEngine::setMuted(std::wstring const& source, bool muted) {
    for (auto& input : inputs) {
        if (input->id == source) input->muted = muted;
    }
}

void AudioEngine::mix() {
    MmcssScope mmcss;
    HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    LARGE_INTEGER due{};
    due.QuadPart = -100'000;
    SetWaitableTimer(timer, &due, 10, nullptr, nullptr, FALSE);

    HANDLE events[] = {stopEvent, timer};
    auto startedAt = now();
    int64_t produced = 0;
    std::vector<float> output;

    while (WaitForMultipleObjects(2, events, FALSE, 50) != WAIT_OBJECT_0) {
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

    CancelWaitableTimer(timer);
    CloseHandle(timer);
}

JsonArray listMicrophones() {
    ComScope com;
    JsonArray result;

    try {
        auto enumerator = deviceEnumerator();
        com_ptr<IMMDeviceCollection> devices;
        winrt::check_hresult(enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, devices.put()));

        UINT count = 0;
        devices->GetCount(&count);

        for (UINT index = 0; index < count; index++) {
            com_ptr<IMMDevice> device;
            if (FAILED(devices->Item(index, device.put()))) continue;

            LPWSTR id = nullptr;
            if (FAILED(device->GetId(&id))) continue;

            std::wstring label;
            com_ptr<IPropertyStore> properties;
            if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, properties.put()))) {
                PROPVARIANT name;
                PropVariantInit(&name);
                if (SUCCEEDED(properties->GetValue(FRIENDLY_NAME, &name)) && name.vt == VT_LPWSTR) {
                    label = name.pwszVal;
                }
                PropVariantClear(&name);
            }

            JsonObject item;
            item.SetNamedValue(L"id", JsonValue::CreateStringValue(id));
            item.SetNamedValue(L"label", JsonValue::CreateStringValue(label));
            result.Append(item);
            CoTaskMemFree(id);
        }
    } catch (winrt::hresult_error const& error) {
        logWarn(L"Could not list the microphones: " + std::wstring(error.message()));
    }

    return result;
}

}

#include <host/recording/audio.h>

#include <shared/log.h>
#include <shared/utf8.h>

#include <windows.h>

#include <audioclient.h>
#include <audiopolicy.h>
#include <avrt.h>
#include <ks.h>
#include <ksmedia.h>
#include <mmdeviceapi.h>
#include <winrt/base.h>

#include <future>
#include <optional>
#include <stdexcept>

namespace fileship::host::recording {

namespace {

using winrt::com_ptr;

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

class WasapiSource : public CaptureSource {
public:
    explicit WasapiSource(CaptureOptions options) : options(std::move(options)) {
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

    ~WasapiSource() override {
        SetEvent(stopEvent);
        if (thread.joinable()) thread.join();
        CloseHandle(stopEvent);
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

                auto count = static_cast<size_t>(frames) * CHANNELS;
                push(flags & AUDCLNT_BUFFERFLAGS_SILENT ? nullptr : reinterpret_cast<float const*>(data), count);
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
};

}

std::unique_ptr<CaptureSource> openCaptureSource(CaptureOptions const& options) {
    return std::make_unique<WasapiSource>(options);
}

struct MixClock::State {
    HANDLE stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    std::unique_ptr<MmcssScope> mmcss;
};

MixClock::MixClock() : state(std::make_unique<State>()) {
    LARGE_INTEGER due{};
    due.QuadPart = -100'000;
    SetWaitableTimer(state->timer, &due, 10, nullptr, nullptr, FALSE);
}

MixClock::~MixClock() {
    CancelWaitableTimer(state->timer);
    CloseHandle(state->timer);
    CloseHandle(state->stopEvent);
}

bool MixClock::wait() {
    if (!state->mmcss) state->mmcss = std::make_unique<MmcssScope>();

    HANDLE events[] = {state->stopEvent, state->timer};
    if (WaitForMultipleObjects(2, events, FALSE, 50) != WAIT_OBJECT_0) return true;

    state->mmcss.reset();
    return false;
}

void MixClock::stop() {
    SetEvent(state->stopEvent);
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

            std::wstring friendlyName;
            com_ptr<IPropertyStore> properties;
            if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, properties.put()))) {
                PROPVARIANT name;
                PropVariantInit(&name);
                if (SUCCEEDED(properties->GetValue(FRIENDLY_NAME, &name)) && name.vt == VT_LPWSTR) {
                    friendlyName = name.pwszVal;
                }
                PropVariantClear(&name);
            }

            JsonObject item;
            item.SetNamedValue(L"id", JsonValue::CreateStringValue(id));
            item.SetNamedValue(L"name", JsonValue::CreateStringValue(friendlyName));
            result.Append(item);
            CoTaskMemFree(id);
        }
    } catch (winrt::hresult_error const& error) {
        logWarn(L"Could not list the microphones: " + std::wstring(error.message()));
    }

    return result;
}

}

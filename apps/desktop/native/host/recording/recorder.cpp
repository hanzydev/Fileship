#include "audio.h"
#include "command.h"
#include "ffmpeg.h"
#include "pipe.h"
#include "process.h"

#include <host/recording.h>
#include <host/rpc.h>
#include <host/services.h>
#include <shared/log.h>
#include <ui/recorderUi.h>

#include <dxgi1_2.h>
#include <shlobj.h>
#include <winrt/base.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <thread>

namespace fs = std::filesystem;

namespace fileship::host::recording {

namespace {

constexpr int64_t CAPTURE_LATENCY_MS = 100;

struct State {
    std::wstring phase = L"idle";
    std::wstring kind = L"video";
    int64_t elapsed = 0;
    std::optional<int64_t> since;
    int countdown = 0;
    std::vector<AudioTrack> tracks;
    std::vector<std::wstring> muted;
    int64_t maxDuration = 0;
};

struct PendingChunk {
    double at = 0;
    std::vector<char> data;
};

struct Segment {
    std::wstring output;
    std::optional<double> videoStartedAt;
    std::vector<std::deque<PendingChunk>> pending;
    std::vector<bool> aligned;
    std::unique_ptr<ChildProcess> process;
    std::vector<std::unique_ptr<AudioPipe>> pipes;
    std::string stderrText;
    bool isStopping = false;
};

struct Recording {
    std::wstring kind;
    JsonObject settings{nullptr};
    std::wstring ffmpeg;
    std::wstring codec;
    std::vector<AudioTrack> tracks;
    bool useDda = false;
    RecordTarget target;
    fs::path folder;
    std::vector<std::wstring> segments;
    std::shared_ptr<Segment> segment;
    std::unique_ptr<AudioEngine> audio;
    bool isTiming = false;
    bool isSwitching = false;
};

std::recursive_mutex mutex;
State state;
std::shared_ptr<Recording> current;
AreaPicker areaPicker;

std::mutex publishMutex;
std::condition_variable publishSignal;
std::optional<State> pendingPublish;
bool isPublisherRunning = false;

int64_t epochMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

double steadyMs() {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

template <typename Work>
void async(char const* name, Work work) {
    std::thread([name, work = std::move(work)]() mutable {
        try {
            work();
        } catch (std::exception const& error) {
            logError(toWide(name) + L" failed: " + toWide(error.what()));
        } catch (winrt::hresult_error const& error) {
            logError(toWide(name) + L" failed: " + std::wstring(error.message()));
        }
    }).detach();
}

State idleState(std::wstring const& kind = L"video") {
    State result;
    result.kind = kind;
    return result;
}

std::wstring message(std::exception const& error) {
    return toWide(error.what());
}

ui::AudioSource audioSource(std::wstring const& id) {
    return id == L"microphone" ? ui::AudioSource::Microphone : ui::AudioSource::System;
}

ui::RecorderPhase recorderPhase(std::wstring const& phase) {
    if (phase == L"preparing") return ui::RecorderPhase::Preparing;
    if (phase == L"countdown") return ui::RecorderPhase::Countdown;
    if (phase == L"recording") return ui::RecorderPhase::Recording;
    if (phase == L"paused") return ui::RecorderPhase::Paused;
    if (phase == L"processing") return ui::RecorderPhase::Processing;
    return ui::RecorderPhase::Idle;
}

std::vector<std::wstring> sourcesOf(std::vector<AudioTrack> const& tracks) {
    std::vector<std::wstring> sources;
    for (auto const& track : tracks) {
        for (auto const& source : track.sources) {
            if (std::find(sources.begin(), sources.end(), source) == sources.end()) sources.push_back(source);
        }
    }
    return sources;
}

ui::RecorderViewState viewOf(State const& value) {
    ui::RecorderViewState view;
    view.phase = recorderPhase(value.phase);
    view.countdown = value.countdown;
    view.elapsedMs = value.elapsed;
    view.sinceMs = value.since.value_or(0);
    view.maxDurationMs = value.maxDuration;
    for (auto const& source : sourcesOf(value.tracks)) view.sources.push_back(audioSource(source));
    for (auto const& source : value.muted) view.muted.push_back(audioSource(source));
    return view;
}

JsonObject stateJson(State const& value) {
    JsonObject result;
    JsonArray muted;
    for (auto const& source : value.muted) muted.Append(JsonValue::CreateStringValue(source));

    result.SetNamedValue(L"phase", JsonValue::CreateStringValue(value.phase));
    result.SetNamedValue(L"kind", JsonValue::CreateStringValue(value.kind));
    result.SetNamedValue(L"elapsed", JsonValue::CreateNumberValue(static_cast<double>(value.elapsed)));
    result.SetNamedValue(L"since", value.since ? JsonValue::CreateNumberValue(static_cast<double>(*value.since))
                                               : JsonValue::CreateNullValue());
    result.SetNamedValue(L"countdown", JsonValue::CreateNumberValue(value.countdown));
    result.SetNamedValue(L"tracks", tracksJson(value.tracks));
    result.SetNamedValue(L"muted", muted);
    result.SetNamedValue(L"maxDuration", JsonValue::CreateNumberValue(static_cast<double>(value.maxDuration)));
    return result;
}

void runPublisher() {
    for (;;) {
        State value;

        {
            std::unique_lock lock(publishMutex);
            publishSignal.wait(lock, [] { return pendingPublish.has_value(); });
            value = std::move(*pendingPublish);
            pendingPublish.reset();
        }

        try {
            refreshTray();
            ui::updateRecorderUi(viewOf(value));
        } catch (...) {
        }
    }
}

template <typename Patch>
void setState(Patch patch) {
    State snapshot;

    {
        std::lock_guard lock(mutex);
        patch(state);
        snapshot = state;
    }

    {
        std::lock_guard lock(publishMutex);
        pendingPublish = std::move(snapshot);
        if (!isPublisherRunning) {
            isPublisherRunning = true;
            std::thread(runPublisher).detach();
        }
    }

    publishSignal.notify_one();
}

bool contains(ui::ScreenRect const& outer, ui::ScreenRect const& inner) {
    return inner.x >= outer.x && inner.y >= outer.y && inner.x + inner.width <= outer.x + outer.width &&
           inner.y + inner.height <= outer.y + outer.height;
}

std::optional<OutputTarget> findOutput(ui::ScreenRect const& rect) {
    winrt::com_ptr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(factory.put())))) {
        logWarn(L"Could not list the displays");
        return std::nullopt;
    }

    winrt::com_ptr<IDXGIAdapter1> adapter;
    for (UINT adapterIndex = 0; factory->EnumAdapters1(adapterIndex, adapter.put()) != DXGI_ERROR_NOT_FOUND;
         adapterIndex++) {
        winrt::com_ptr<IDXGIOutput> output;

        for (UINT outputIndex = 0; adapter->EnumOutputs(outputIndex, output.put()) != DXGI_ERROR_NOT_FOUND;
             outputIndex++) {
            DXGI_OUTPUT_DESC desc{};

            if (SUCCEEDED(output->GetDesc(&desc))) {
                auto const& bounds = desc.DesktopCoordinates;
                ui::ScreenRect outputRect{bounds.left, bounds.top, bounds.right - bounds.left,
                                          bounds.bottom - bounds.top};
                if (contains(outputRect, rect)) {
                    return OutputTarget{static_cast<int>(adapterIndex), static_cast<int>(outputIndex), outputRect};
                }
            }

            output = nullptr;
        }

        adapter = nullptr;
    }

    return std::nullopt;
}

ui::ScreenRect primaryBounds() {
    MONITORINFO info{sizeof(info)};
    GetMonitorInfoW(MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY), &info);
    auto const& rect = info.rcMonitor;
    return {rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top};
}

ui::ThemeMode themeMode(JsonObject const& value) {
    auto theme = text(value, L"theme", L"system");
    if (theme == L"light") return ui::ThemeMode::Light;
    if (theme == L"dark") return ui::ThemeMode::Dark;

    DWORD light = 1;
    DWORD size = sizeof(light);
    RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                 L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &size);
    return light ? ui::ThemeMode::Light : ui::ThemeMode::Dark;
}

std::wstring pad(int value, size_t width = 2) {
    auto text = std::to_wstring(value);
    return std::wstring(width > text.size() ? width - text.size() : 0, L'0') + text;
}

void replaceAll(std::wstring& value, std::wstring const& from, std::wstring const& to) {
    for (size_t index = 0; (index = value.find(from, index)) != std::wstring::npos; index += to.size()) {
        value.replace(index, from.size(), to);
    }
}

std::wstring randomName() {
    static std::mt19937_64 random{std::random_device{}()};
    static wchar_t const digits[] = L"0123456789abcdefghijklmnopqrstuvwxyz";
    std::wstring result;
    for (int index = 0; index < 10; index++) result += digits[random() % 36];
    return result;
}

std::wstring formatFileName(std::wstring pattern) {
    SYSTEMTIME now{};
    GetLocalTime(&now);

    replaceAll(pattern, L"%yyyy", std::to_wstring(now.wYear));
    replaceAll(pattern, L"%mo", pad(now.wMonth));
    replaceAll(pattern, L"%d", pad(now.wDay));
    replaceAll(pattern, L"%h", pad(now.wHour));
    replaceAll(pattern, L"%mi", pad(now.wMinute));
    replaceAll(pattern, L"%ms", pad(now.wMilliseconds, 3));
    replaceAll(pattern, L"%s", pad(now.wSecond));
    replaceAll(pattern, L"%pn", L"");
    replaceAll(pattern, L"%ra", randomName());

    std::wstring_view const invalid = L"\\/:*?\"<>|";
    std::erase_if(pattern, [&](wchar_t c) { return invalid.find(c) != std::wstring_view::npos; });

    auto start = pattern.find_first_not_of(L" \t\r\n");
    if (start == std::wstring::npos) return L"";
    auto end = pattern.find_last_not_of(L" \t\r\n");
    return pattern.substr(start, end - start + 1);
}

fs::path saveFolder(JsonObject const& value) {
    auto folder = text(section(value, L"save"), L"folder");
    if (!folder.empty()) return folder;

    PWSTR pictures = nullptr;
    SHGetKnownFolderPath(FOLDERID_Pictures, 0, nullptr, &pictures);
    fs::path result = fs::path(pictures) / L"Fileship";
    CoTaskMemFree(pictures);
    return result;
}

fs::path uniquePath(fs::path const& folder, std::wstring const& name) {
    auto extension = fs::path(name).extension().wstring();
    auto base = name.substr(0, name.size() - extension.size());
    auto path = folder / name;

    std::error_code error;
    for (int index = 2; fs::exists(path, error); index++) {
        path = folder / (base + L" (" + std::to_wstring(index) + L")" + extension);
    }

    return path;
}

fs::path tempFolder() {
    wchar_t buffer[MAX_PATH + 1]{};
    GetTempPathW(MAX_PATH + 1, buffer);
    return fs::path(buffer);
}

void playShutter() {
    if (flag(section(settings(), L"capture"), L"playSound", true)) playSoundFile(L"shutter.wav");
}

void stopRecording();
void abortRecording();

void writeAudio(Segment& segment, size_t track, char const* data, size_t size) {
    if (track < segment.pipes.size() && segment.pipes[track]) segment.pipes[track]->write(data, size);
}

void writeAligned(Segment& segment, size_t track, double at, std::vector<char> const& data) {
    if (segment.aligned[track]) return writeAudio(segment, track, data.data(), data.size());

    auto startedAt = *segment.videoStartedAt;
    auto endedAt = at - CAPTURE_LATENCY_MS;
    auto beganAt = endedAt - static_cast<double>(data.size()) / BYTES_PER_SECOND * 1000;
    if (endedAt <= startedAt) return;

    segment.aligned[track] = true;

    auto frames = std::max<int64_t>(0, std::llround((startedAt - beganAt) / 1000 * SAMPLE_RATE));
    auto skip = std::min(static_cast<size_t>(frames) * 8, data.size());
    writeAudio(segment, track, data.data() + skip, data.size() - skip);
}

void markVideoStarted(Segment& segment) {
    std::lock_guard lock(mutex);
    if (segment.videoStartedAt) return;

    segment.videoStartedAt = steadyMs();

    for (size_t track = 0; track < segment.pending.size(); track++) {
        for (auto const& chunk : segment.pending[track]) writeAligned(segment, track, chunk.at, chunk.data);
        segment.pending[track].clear();
    }
}

void writeRecordingAudio(size_t track, std::vector<float> const& samples) {
    std::lock_guard lock(mutex);
    auto segment = current ? current->segment : nullptr;
    if (state.phase != L"recording" || !segment || segment->isStopping) return;

    auto bytes = reinterpret_cast<char const*>(samples.data());
    std::vector<char> data(bytes, bytes + samples.size() * sizeof(float));
    auto at = steadyMs();

    if (segment->videoStartedAt) return writeAligned(*segment, track, at, data);
    if (track >= segment->pending.size()) return;

    auto& pending = segment->pending[track];
    pending.push_back({at, std::move(data)});
    if (pending.size() > 100) pending.pop_front();
}

void startSegment(std::shared_ptr<Recording> const& recording) {
    auto index = recording->segments.size();
    auto output = (recording->folder / (L"segment-" + std::to_wstring(index) + L".mkv")).wstring();
    auto const& tracks = recording->tracks;
    auto stamp = std::to_wstring(epochMs());

    auto segment = std::make_shared<Segment>();
    segment->output = output;
    segment->pending.resize(tracks.size());
    segment->aligned.resize(tracks.size(), false);

    std::vector<std::wstring> pipes;
    for (size_t track = 0; track < tracks.size(); track++) {
        auto name = L"\\\\.\\pipe\\fileship-" + std::to_wstring(GetCurrentProcessId()) + L"-" + stamp + L"-" +
                    std::to_wstring(index) + L"-" + std::to_wstring(track);
        segment->pipes.push_back(
            std::make_unique<AudioPipe>(name, static_cast<size_t>(std::llround(BYTES_PER_SECOND * PRIMING_SECONDS))));
        pipes.push_back(name);
    }

    RecordCommand command{recording->kind, recording->codec, recording->useDda, recording->target,
                          pipes,           tracks,           output};
    auto args = recordArgs(recording->settings, command);

    logInfo(L"Recording " + formatCommand(recording->ffmpeg, args));

    std::weak_ptr<Segment> weak = segment;
    segment->process = std::make_unique<ChildProcess>(recording->ffmpeg, args, [weak](std::string const& line) {
        auto segment = weak.lock();
        if (!segment) return;

        if (line.find(FIRST_FRAME_MARKER) != std::string::npos) {
            bool started = false;
            {
                std::lock_guard lock(mutex);
                started = segment->videoStartedAt.has_value();
            }
            if (!started) markVideoStarted(*segment);
        }

        if (line.starts_with("[error]") || line.starts_with("[fatal]") || line.starts_with("[panic]")) {
            auto text = line.substr(line.find(']') + 1);
            text.erase(0, text.find_first_not_of(" \t"));

            std::lock_guard lock(mutex);
            segment->stderrText += text + "\n";
            if (segment->stderrText.size() > 4000) {
                segment->stderrText.erase(0, segment->stderrText.size() - 4000);
            }
        }
    });

    {
        std::lock_guard lock(mutex);
        if (current != recording) {
            segment->isStopping = true;
            segment->process->kill();
            return;
        }

        recording->segment = segment;
        recording->segments.push_back(output);
    }

    setState([](State& value) {
        value.phase = L"recording";
        value.since = epochMs();
    });

    std::weak_ptr<Recording> weakRecording = recording;
    std::thread([segment, weakRecording] {
        segment->process->wait(INFINITE);
        auto code = segment->process->exitCode();
        std::string stderrText;

        {
            std::lock_guard lock(mutex);
            auto recording = weakRecording.lock();
            if (segment->isStopping || !recording || current != recording) return;
            stderrText = segment->stderrText;
        }

        logError(L"ffmpeg stopped while recording " + std::to_wstring(code) + L" " + toWide(stderrText));
        auto last = toWide(lastLine(stderrText));
        notify(t(L"recording.stopped"),
               last.empty() ? t(L"recording.ffmpegExited", {{L"code", std::to_wstring(static_cast<int>(code))}})
                            : last);
        stopRecording();
    }).detach();
}

void closeSegment(std::shared_ptr<Recording> const& recording, std::shared_ptr<Segment> const& segment,
                  bool isAbort = false) {
    std::vector<std::unique_ptr<AudioPipe>> pipes;

    {
        std::lock_guard lock(mutex);
        segment->isStopping = true;
        pipes = std::move(segment->pipes);
        segment->pipes.clear();
    }

    setState([](State& value) {
        if (!value.since) return;
        value.elapsed += epochMs() - *value.since;
        value.since.reset();
    });

    if (isAbort) {
        segment->process->kill();
    } else {
        segment->process->write("q");
        for (auto& pipe : pipes) pipe->end();
    }

    if (!segment->process->wait(15'000)) {
        std::lock_guard lock(mutex);
        logWarn(L"ffmpeg did not stop in time " + toWide(segment->stderrText));
        segment->process->kill();
    }

    segment->process->wait(INFINITE);
    if (auto code = segment->process->exitCode()) {
        std::lock_guard lock(mutex);
        logWarn(L"ffmpeg exited with " + std::to_wstring(code) + L" " + toWide(segment->stderrText));
    }

    pipes.clear();

    std::lock_guard lock(mutex);
    if (recording->segment == segment) recording->segment = nullptr;
}

void finish(std::shared_ptr<Recording> const& recording) {
    std::unique_ptr<AudioEngine> audio;

    {
        std::lock_guard lock(mutex);
        recording->isTiming = false;
        audio = std::move(recording->audio);
        if (current == recording) current = nullptr;
    }

    audio.reset();
    ui::hideRecorderUi();
    setState([&](State& value) { value = idleState(recording->kind); });
}

struct Produced {
    fs::path output;
    std::vector<uint8_t> thumbnail;
};

Produced produce(Recording const& recording) {
    auto recordingSettings = section(recording.settings, L"recording");
    std::vector<std::wstring> segments;

    for (auto const& segment : recording.segments) {
        std::error_code error;
        auto size = fs::file_size(segment, error);
        if (!error && size > 1024) segments.push_back(segment);
    }

    if (segments.empty()) throw std::runtime_error(toUtf8(t(L"recording.nothingRecorded")));

    auto list = recording.folder / L"segments.txt";
    {
        std::ofstream file(list, std::ios::binary);
        for (size_t index = 0; index < segments.size(); index++) {
            auto path = segments[index];
            std::replace(path.begin(), path.end(), L'\\', L'/');

            std::wstring escaped;
            for (auto c : path) escaped += c == L'\'' ? std::wstring(L"'\\''") : std::wstring(1, c);

            file << (index ? "\n" : "") << "file '" << toUtf8(escaped) << "'";
        }
    }

    auto extension = recording.kind == L"gif" ? std::wstring(L"gif") : text(recordingSettings, L"container", L"mp4");
    auto afterCapture = section(recording.settings, L"afterCapture");
    bool isSaved = flag(afterCapture, L"saveToFile", false) || !flag(afterCapture, L"upload", true);
    auto folder = isSaved ? saveFolder(recording.settings) : recording.folder;
    fs::create_directories(folder);

    auto name = formatFileName(text(section(recording.settings, L"save"), L"pattern", L"%yyyy-%mo-%d_%h-%mi-%s"));
    auto output = uniquePath(folder, (name.empty() ? std::wstring(L"recording") : name) + L"." + extension);
    auto input = segmentInput(segments, list.wstring());

    runProcess(recording.ffmpeg, recording.kind == L"gif"
                                     ? gifArgs(input, output.wstring(), section(recording.settings, L"gif"))
                                     : joinArgs(input, output.wstring(), extension, recording.tracks));

    std::vector<uint8_t> thumbnail;
    try {
        auto result = runProcess(recording.ffmpeg, thumbnailArgs(output.wstring()));
        thumbnail.assign(result.out.begin(), result.out.end());
    } catch (...) {
    }

    return {output, thumbnail};
}

void removeFolder(fs::path const& folder) {
    std::error_code error;
    fs::remove_all(folder, error);
}

void stopRecording() {
    std::shared_ptr<Recording> recording;
    std::shared_ptr<Segment> segment;
    bool isStarting = false;

    {
        std::lock_guard lock(mutex);
        recording = current;
        if (!recording || state.phase == L"processing") return;

        isStarting = state.phase == L"preparing" || state.phase == L"countdown";
        if (!isStarting) {
            recording->isTiming = false;
            segment = recording->segment;
        }
    }

    if (isStarting) return abortRecording();

    setState([](State& value) { value.phase = L"processing"; });
    if (segment) closeSegment(recording, segment);

    std::optional<Produced> result;

    try {
        result = produce(*recording);
        playShutter();
    } catch (std::exception const& error) {
        logError(L"Could not finish the recording: " + message(error));
        notify(t(L"recording.finishFailed"), message(error));
    }

    finish(recording);

    if (result) {
        try {
            handleRecording({result->output, recording->kind == L"gif" ? L"gif" : L"recording", result->thumbnail});
        } catch (std::exception const& error) {
            logError(L"Could not handle the recording: " + message(error));
            notify(t(L"app.somethingWentWrong"), message(error));
        }
    }

    notifyIdle();
    removeFolder(recording->folder);
}

void abortRecording() {
    std::shared_ptr<Recording> recording;
    std::shared_ptr<Segment> segment;

    {
        std::lock_guard lock(mutex);
        recording = current;
        if (!recording || state.phase == L"processing") return;

        current = nullptr;
        segment = recording->segment;
    }

    if (segment) closeSegment(recording, segment, true);

    finish(recording);
    notifyIdle();
    removeFolder(recording->folder);
}

void pauseRecording() {
    std::shared_ptr<Recording> recording;
    std::shared_ptr<Segment> segment;

    {
        std::lock_guard lock(mutex);
        recording = current;
        if (!recording || !recording->segment || state.phase != L"recording" || recording->isSwitching) return;

        recording->isSwitching = true;
        segment = recording->segment;
    }

    closeSegment(recording, segment);

    std::lock_guard lock(mutex);
    recording->isSwitching = false;
    if (current == recording) setState([](State& value) { value.phase = L"paused"; });
}

void resumeRecording() {
    std::shared_ptr<Recording> recording;

    {
        std::lock_guard lock(mutex);
        recording = current;
        if (!recording || state.phase != L"paused" || recording->isSwitching) return;

        recording->isSwitching = true;
    }

    try {
        startSegment(recording);
    } catch (std::exception const& error) {
        {
            std::lock_guard lock(mutex);
            recording->isSwitching = false;
        }

        notify(t(L"recording.resumeFailed"), message(error));
        return stopRecording();
    }

    std::lock_guard lock(mutex);
    recording->isSwitching = false;
}

void runTimer(std::shared_ptr<Recording> const& recording) {
    std::thread([recording] {
        for (;;) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));

            {
                std::lock_guard lock(mutex);
                if (current != recording || !recording->isTiming) return;

                auto elapsed = state.elapsed + (state.since ? epochMs() - *state.since : 0);
                if (!state.maxDuration || elapsed < state.maxDuration) continue;
            }

            stopRecording();
            return;
        }
    }).detach();
}

void begin(std::wstring const& kind, ui::ScreenRect const& rect) {
    auto ffmpeg = getFfmpeg();

    if (!ffmpeg) {
        notify(t(L"recording.ffmpegFailed"), t(L"recording.checkFfmpeg"));
        showMainWindow(L"/settings/recording?ffmpeg=missing");
        return;
    }

    auto values = settings();
    auto recordingSettings = section(values, L"recording");
    auto codec = text(recordingSettings, L"codec", L"auto");
    if (kind == L"gif") {
        codec = L"libx264";
    } else if (codec == L"auto") {
        codec = autoCodec(ffmpeg->encoders);
    }

    if (std::find(ffmpeg->encoders.begin(), ffmpeg->encoders.end(), codec) == ffmpeg->encoders.end()) {
        notify(t(L"recording.encoderUnavailable"), t(L"recording.encoderUnsupported", {{L"codec", codec}}));
        showMainWindow(L"/settings/recording");
        return;
    }

    auto problem = kind == L"video" ? containerProblem(text(recordingSettings, L"container", L"mp4"), codec)
                                    : std::nullopt;

    if (problem) {
        notify(t(L"recording.cannotRecord"), *problem);
        showMainWindow(L"/settings/recording");
        return;
    }

    auto output = findOutput(rect);
    auto recording = std::make_shared<Recording>();
    recording->kind = kind;
    recording->settings = values;
    recording->ffmpeg = ffmpeg->path;
    recording->codec = codec;
    recording->tracks = kind == L"gif" ? std::vector<AudioTrack>{} : audioTracks(section(values, L"audio"));
    recording->useDda = text(recordingSettings, L"source", L"auto") != L"gdigrab" && ffmpeg->ddagrab && output;
    recording->target = {rect, output};
    recording->folder = tempFolder() / (L"fileship-recording-" + std::to_wstring(epochMs()));

    {
        std::lock_guard lock(mutex);
        current = recording;
    }

    fs::create_directories(recording->folder);

    setState([&](State& value) {
        value = idleState(kind);
        value.phase = L"preparing";
        value.tracks = recording->tracks;
        value.maxDuration = static_cast<int64_t>(number(recordingSettings, L"maxDuration", 0)) * 1000;
    });

    ui::RecorderUiCallbacks callbacks;
    callbacks.onControl = [](ui::RecorderAction action) {
        controlRecording(action == ui::RecorderAction::Stop    ? L"stop"
                         : action == ui::RecorderAction::Pause ? L"pause"
                         : action == ui::RecorderAction::Abort ? L"abort"
                                                               : L"resume");
    };
    callbacks.onMute = [](ui::AudioSource source, bool muted) {
        setRecordingMute(source == ui::AudioSource::Microphone ? L"microphone" : L"system", muted);
    };

    ui::showRecorderUi(rect, themeMode(values), callbacks, flag(recordingSettings, L"showBorder", true));

    std::optional<ui::RecorderViewState> view;
    {
        std::lock_guard lock(mutex);
        if (current == recording) view = viewOf(state);
    }
    if (view) ui::updateRecorderUi(*view);

    std::optional<std::wstring> error;
    std::unique_ptr<AudioEngine> audio;

    try {
        if (!recording->tracks.empty()) {
            audio = std::make_unique<AudioEngine>(section(values, L"audio"), recording->tracks,
                                                  [](size_t track, std::vector<float> const& samples) {
                                                      writeRecordingAudio(track, samples);
                                                  });
        }
    } catch (std::exception const& audioError) {
        error = message(audioError);
    } catch (...) {
        error = t(L"recorder.audioDevicesFailed");
    }

    {
        std::lock_guard lock(mutex);
        if (current != recording) return;

        recording->audio = std::move(audio);
        for (auto const& source : state.muted) {
            if (recording->audio) recording->audio->setMuted(source, true);
        }
    }

    if (error) {
        notify(t(L"recording.startFailed"), *error);
        return abortRecording();
    }

    for (int seconds = number(recordingSettings, L"startDelay", 0); seconds > 0; seconds--) {
        setState([&](State& value) {
            value.phase = L"countdown";
            value.countdown = seconds;
        });
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::lock_guard lock(mutex);
        if (current != recording) return;
    }

    try {
        startSegment(recording);
    } catch (std::exception const& startError) {
        logError(L"Could not start ffmpeg: " + message(startError));
        notify(t(L"recording.startFailed"), message(startError));
        return abortRecording();
    }

    {
        std::lock_guard lock(mutex);
        if (current != recording) return;
        recording->isTiming = true;
    }

    runTimer(recording);
}

}

void setAreaPicker(AreaPicker picker) {
    std::lock_guard lock(mutex);
    areaPicker = std::move(picker);
}

void toggleRecording(std::wstring const& kind) {
    AreaPicker picker;

    {
        std::lock_guard lock(mutex);

        if (state.phase != L"idle" && state.kind != kind) {
            notify(t(L"recording.alreadyRecording"), t(L"recording.alreadyRecordingDescription"));
            return;
        }

        if (state.phase == L"recording" || state.phase == L"paused") return async("Recorder", stopRecording);
        if (state.phase == L"preparing" || state.phase == L"countdown") return async("Recorder", abortRecording);
        if (state.phase != L"idle") return;

        picker = areaPicker;
    }

    if (!ffmpegPath()) {
        notify(t(L"recording.ffmpegFailed"), t(L"recording.checkFfmpeg"));
        showMainWindow(L"/settings/recording?ffmpeg=missing");
        return;
    }

    async("Recorder", [] { getFfmpeg(); });

    if (!picker) {
        logWarn(L"No area picker is registered for recordings");
        return;
    }

    picker(kind == L"gif" ? ui::OverlayPurpose::Gif : ui::OverlayPurpose::Video, [kind](ui::ScreenRect area) {
        async("Recorder", [kind, area] {
            try {
                begin(kind, area);
            } catch (std::exception const& error) {
                logError(L"Could not start recording: " + message(error));
                notify(t(L"recording.startFailed"), message(error));
                abortRecording();
            }
        });
    });
}

void controlRecording(std::wstring const& action) {
    async("Recorder", [action] {
        if (action == L"stop") return stopRecording();
        if (action == L"abort") return abortRecording();
        if (action == L"pause") return pauseRecording();
        resumeRecording();
    });
}

void setRecordingMute(std::wstring const& source, bool muted) {
    {
        std::lock_guard lock(mutex);
        if (current && current->audio) current->audio->setMuted(source, muted);
    }

    setState([&](State& value) {
        std::erase(value.muted, source);
        if (muted) value.muted.push_back(source);
    });
}

bool isRecording() {
    std::lock_guard lock(mutex);
    return state.phase != L"idle";
}

JsonObject recorderState() {
    std::lock_guard lock(mutex);
    return stateJson(state);
}

void resetFfmpegProbe() {
    clearFfmpegProbe();
    async("FFmpeg", publishFfmpegStatus);
}

void quitRecorder() {
    std::lock_guard lock(mutex);
    if (current && current->segment && current->segment->process) current->segment->process->write("q");
}

JsonValue recordingPreview() {
    auto ffmpeg = getFfmpeg();
    auto values = settings();
    auto recordingSettings = section(values, L"recording");
    auto codec = text(recordingSettings, L"codec", L"auto");
    if (codec == L"auto") codec = autoCodec(ffmpeg ? ffmpeg->encoders : std::vector<std::wstring>{});

    auto bounds = primaryBounds();
    auto tracks = audioTracks(section(values, L"audio"));
    auto output = findOutput(bounds);

    std::vector<std::wstring> pipes;
    for (auto const& track : tracks) pipes.push_back(L"\\\\.\\pipe\\fileship-" + track.id);

    RecordCommand command{L"video",
                          codec,
                          text(recordingSettings, L"source", L"auto") != L"gdigrab" && (!ffmpeg || ffmpeg->ddagrab),
                          {bounds, output ? output : OutputTarget{0, 0, bounds}},
                          pipes,
                          tracks,
                          L"recording.mkv"};

    return JsonValue::CreateStringValue(
        formatCommand(ffmpeg ? ffmpeg->path : L"ffmpeg.exe", recordArgs(values, command)));
}

void registerRecordingHandlers() {
    registerHandler(L"ffmpeg:status", [](JsonArray const&) { return jsonValue(ffmpegStatus()); });
    registerHandler(L"ffmpeg:download", [](JsonArray const&) {
        downloadFfmpeg();
        return JsonValue::CreateNullValue();
    });
    registerHandler(L"recording:preview", [](JsonArray const&) { return recordingPreview(); });
    registerHandler(L"recording:microphones", [](JsonArray const&) { return jsonValue(listMicrophones()); });
}

}

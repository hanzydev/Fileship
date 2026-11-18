#include <host/app.h>
#include <host/linux/host.h>
#include <host/recording/platform.h>

#include <glib.h>

#include <filesystem>
#include <mutex>

namespace fileship::host::recording {

namespace {

std::mutex mutex;
ui::RecorderUiCallbacks callbacks;
std::optional<ui::RecorderPhase> shown;
uint32_t notice = 0;

void handleAction(std::string const& action) {
    ui::RecorderUiCallbacks current;

    {
        std::lock_guard lock(mutex);
        current = callbacks;
    }

    if (!current.onControl) return;
    if (action == "stop") current.onControl(ui::RecorderAction::Stop);
    if (action == "pause") current.onControl(ui::RecorderAction::Pause);
    if (action == "resume") current.onControl(ui::RecorderAction::Resume);
    if (action == "discard") current.onControl(ui::RecorderAction::Abort);
}

}

std::optional<std::wstring> ffmpegPath() {
    auto custom = text(section(settings(), L"recording"), L"ffmpegPath");
    if (!custom.empty()) return std::filesystem::is_regular_file(custom) ? std::optional(custom) : std::nullopt;

    gchar* found = g_find_program_in_path("ffmpeg");
    if (!found) return std::nullopt;

    auto path = toWide(found);
    g_free(found);
    return path;
}

std::wstring ffmpegFileName() {
    return L"ffmpeg";
}

void describeFfmpeg(JsonObject& status) {
    status.SetNamedValue(L"managedVersion", JsonValue::CreateStringValue(L""));
    status.SetNamedValue(L"download", JsonValue::CreateNullValue());
}

void showRecorderControls(ui::ScreenRect const&, ui::ThemeMode, ui::RecorderUiCallbacks value, bool) {
    std::lock_guard lock(mutex);
    callbacks = std::move(value);
    shown.reset();
}

void updateRecorderControls(ui::RecorderViewState const& state) {
    std::lock_guard lock(mutex);
    if (state.phase == ui::RecorderPhase::Idle || state.phase == shown) return;
    shown = state.phase;

    Notice value;
    value.title = t(L"tray.screenRecording");

    switch (state.phase) {
    case ui::RecorderPhase::Recording:
        value.body = t(L"recorder.recording");
        value.actions = {{"stop", t(L"recorder.stop")}, {"pause", t(L"tray.pause")}, {"discard", t(L"tray.discardRecording")}};
        break;
    case ui::RecorderPhase::Paused:
        value.body = t(L"recorder.paused");
        value.actions = {{"stop", t(L"recorder.stop")}, {"resume", t(L"tray.resume")}, {"discard", t(L"tray.discardRecording")}};
        break;
    case ui::RecorderPhase::Processing:
        value.body = t(L"recorder.processing");
        break;
    default:
        value.body = t(L"recorder.starting");
        value.actions = {{"discard", t(L"tray.discardRecording")}};
        break;
    }

    notice = showNotice(notice, value, handleAction);
}

void hideRecorderControls() {
    std::lock_guard lock(mutex);
    if (notice) closeNotice(notice);
    notice = 0;
    shown.reset();
    callbacks = {};
}

}

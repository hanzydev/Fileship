#pragma once

#include "overlay.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace fileship::ui {

enum class RecorderPhase { Idle, Preparing, Countdown, Recording, Paused, Processing };

enum class RecorderAction { Stop, Pause, Resume, Abort };

enum class AudioSource { Microphone, System };

struct RecorderViewState {
    RecorderPhase phase = RecorderPhase::Idle;
    int countdown = 0;
    int64_t elapsedMs = 0;
    int64_t sinceMs = 0;
    int64_t maxDurationMs = 0;
    std::vector<AudioSource> sources;
    std::vector<AudioSource> muted;
};

struct RecorderUiCallbacks {
    std::function<void(RecorderAction)> onControl;
    std::function<void(AudioSource, bool muted)> onMute;
    std::function<void()> onVisible;
};

void showRecorderUi(ScreenRect area, ThemeMode theme, RecorderUiCallbacks callbacks, bool showBorder = true);
void updateRecorderUi(RecorderViewState const& state);
void hideRecorderUi();

}

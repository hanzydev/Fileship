#pragma once

#include "command.h"
#include "ffmpeg.h"

#include <ui/recorderUi.h>

#include <memory>
#include <optional>
#include <string>

namespace fileship::host::recording {

class VideoCapture {
public:
    static std::unique_ptr<VideoCapture> start(JsonObject const& settings, VideoRequest const& request,
                                               Ffmpeg const& ffmpeg);

    virtual ~VideoCapture() = default;

    virtual VideoInput const& input() const = 0;
    virtual void stop() = 0;
};

VideoInput previewVideoInput(JsonObject const& settings, std::wstring const& codec,
                             std::optional<Ffmpeg> const& ffmpeg);

std::wstring ffmpegFileName();
void describeFfmpeg(JsonObject& status);

void showRecorderControls(ui::ScreenRect const& area, ui::ThemeMode theme, ui::RecorderUiCallbacks callbacks,
                          bool showBorder);
void updateRecorderControls(ui::RecorderViewState const& state);
void hideRecorderControls();

}

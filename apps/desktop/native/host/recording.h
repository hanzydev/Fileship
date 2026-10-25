#pragma once

#include <shared/json.h>
#include <ui/overlay.h>

#include <functional>
#include <string>

namespace fileship::host::recording {

using AreaCallback = std::function<void(ui::ScreenRect area)>;
using AreaPicker = std::function<bool(ui::OverlayPurpose purpose, AreaCallback onArea)>;

void setAreaPicker(AreaPicker picker);
void toggleRecording(std::wstring const& kind);
void controlRecording(std::wstring const& action);
void setRecordingMute(std::wstring const& source, bool muted);
bool isRecording();
JsonObject recorderState();
void resetFfmpegProbe();
void quitRecorder();
void registerRecordingHandlers();

}

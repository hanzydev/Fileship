#pragma once

#include <shared/json.h>

#include <string>

namespace fileship::host::recording {

void toggleRecording(std::wstring const& kind);
void controlRecording(std::wstring const& action);
bool isRecording();
JsonObject recorderState();
void quitRecorder();
void registerRecordingHandlers();

}

#include <host/recording.h>

namespace fileship::host::recording {

void toggleRecording(std::wstring const&) {}
void controlRecording(std::wstring const&) {}

bool isRecording() {
    return false;
}

JsonObject recorderState() {
    return JsonObject();
}

void quitRecorder() {}
void registerRecordingHandlers() {}

}

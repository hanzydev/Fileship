#include <host/app.h>
#include <host/harbor.h>
#include <host/recording.h>

namespace fileship::host::recording {

void setAreaPicker(AreaPicker) {}

void toggleRecording(std::wstring const&) {}

void controlRecording(std::wstring const&) {}

void setRecordingMute(std::wstring const&, bool) {}

bool isRecording() {
    return false;
}

JsonObject recorderState() {
    JsonObject state;
    state.SetNamedValue(L"phase", JsonValue::CreateStringValue(L"idle"));
    return state;
}

void resetFfmpegProbe() {}

void quitRecorder() {}

void registerRecordingHandlers() {}

}

namespace fileship::host::harbor {

void startHarbor() {}

void stopHarbor(StopOptions) {}

JsonObject harborStatus() {
    JsonObject status;
    status.SetNamedValue(L"state", JsonValue::CreateStringValue(L"off"));
    return status;
}

std::optional<std::wstring> harborStorageLabel() {
    return std::nullopt;
}

JsonArray harborActivity() {
    return JsonArray();
}

void pauseHarbor(bool) {}

void openHarbor() {}

void runHarborAction(std::wstring const&, std::vector<std::wstring> const&) {}

std::optional<std::filesystem::path> harborPathFor(std::wstring const&) {
    return std::nullopt;
}

void registerHarborHandlers() {}

}

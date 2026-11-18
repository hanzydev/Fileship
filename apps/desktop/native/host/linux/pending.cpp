#include <host/app.h>
#include <host/harbor.h>
#include <host/platform.h>
#include <host/recording.h>
#include <shared/log.h>

namespace fileship::core {

bool captureRects(std::vector<Rect> const&, bool, std::vector<uint8_t*> const&) {
    return false;
}

std::optional<Bitmap> captureRect(Rect const&, bool) {
    return std::nullopt;
}

std::vector<WindowInfo> listWindows(std::vector<uint64_t> const&, bool) {
    return {};
}

std::optional<WindowInfo> foregroundWindow(std::vector<uint64_t> const&) {
    return std::nullopt;
}

std::vector<uint64_t> processWindows(std::vector<uint32_t> const&) {
    return {};
}

std::vector<DisplayInfo> listDisplays() {
    return {};
}

Rect virtualScreen() {
    return {};
}

DisplayInfo displayNearest(Point) {
    return {};
}

Point cursorPosition() {
    return {};
}

bool playSound(uint8_t const*, size_t) {
    return false;
}

}

namespace fileship::ui {

bool openOverlay(OverlayRequest, OverlayCallback) {
    return false;
}

}

namespace fileship::host {

JsonObject registerHotkeys() {
    return JsonObject();
}

JsonObject hotkeyStatus() {
    return JsonObject();
}

void pauseHotkeys() {}

void unregisterHotkeys() {}

void writeClipboardText(std::wstring const&) {}

bool writeClipboardImage(std::vector<uint8_t> const&, core::Bitmap const&) {
    return false;
}

std::wstring readClipboardText() {
    return {};
}

std::optional<core::Bitmap> readClipboardImage() {
    return std::nullopt;
}

std::optional<std::filesystem::path> readClipboardFilePath() {
    return std::nullopt;
}

void startUpdater() {}

JsonObject updateStatus() {
    JsonObject status;
    status.SetNamedValue(L"state", JsonValue::CreateStringValue(L"unsupported"));
    return status;
}

void checkForUpdates() {}

void installUpdate() {}

void notifyIdle() {}

std::vector<std::filesystem::path> pickFiles() {
    return {};
}

void applyDesktopIntegration(JsonObject const&, bool) {}

void registerPlatformHandlers() {}

void setShellHarborRoot(std::optional<std::filesystem::path> const&) {}

}

namespace fileship::host::recording {

void toggleRecording(std::wstring const&) {}

void controlRecording(std::wstring const&) {}

bool isRecording() {
    return false;
}

JsonObject recorderState() {
    return JsonObject();
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
    status.SetNamedValue(L"root", JsonValue::CreateStringValue(L""));
    status.SetNamedValue(L"files", JsonValue::CreateNumberValue(0));
    status.SetNamedValue(L"folders", JsonValue::CreateNumberValue(0));
    status.SetNamedValue(L"error", JsonValue::CreateNullValue());
    status.SetNamedValue(L"storage", JsonValue::CreateNullValue());
    status.SetNamedValue(L"localSize", JsonValue::CreateNullValue());
    status.SetNamedValue(L"lastSyncedAt", JsonValue::CreateNullValue());
    return status;
}

void pauseHarbor(bool) {}

void runHarborAction(std::wstring const&, std::vector<std::wstring> const&) {}

std::optional<std::filesystem::path> harborPathFor(std::wstring const&) {
    return std::nullopt;
}

void registerHarborHandlers() {
    registerHandler(L"harbor:status", [](JsonArray const&) -> IJsonValue { return harborStatus(); });
}

}

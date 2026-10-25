#include <host/rpc.h>
#include <host/services.h>

#include <shlobj.h>

namespace fileship::host {

namespace {

[[noreturn]] void missing(char const* name) {
    throw std::runtime_error(std::string(name) + " is not implemented yet");
}

std::filesystem::path executableFolder() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
}

}

void registerHandler(std::wstring const&, RpcHandler) {}
void broadcast(std::wstring const&, JsonValue const&) {}

std::filesystem::path userDataPath() {
    PWSTR folder = nullptr;
    SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &folder);
    std::filesystem::path result = std::filesystem::path(folder) / L"Fileship";
    CoTaskMemFree(folder);
    return result;
}

std::filesystem::path resourcesPath() {
    return executableFolder();
}

std::wstring appVersion() {
    return L"0.0.0";
}

JsonObject settings() {
    return JsonObject();
}

JsonObject updateSettings(JsonObject const&) {
    missing("updateSettings");
}

JsonValue storeValue(std::wstring const&) {
    return JsonValue::CreateNullValue();
}

void setStoreValue(std::wstring const&, JsonValue const&) {}

std::wstring instanceUrl() {
    return {};
}

std::wstring token() {
    return {};
}

std::optional<JsonObject> currentUser() {
    return std::nullopt;
}

bool hasPermission(std::wstring const&) {
    return false;
}

std::filesystem::path harborRoot() {
    return {};
}

void setHarborRoot(std::filesystem::path const&) {}

HttpEndpoint apiEndpoint() {
    return {};
}

JsonValue api(std::wstring const&, std::wstring const&, std::optional<JsonValue> const&, int) {
    missing("api");
}

void handleUnauthorized() {}
void resetUploadSettings() {}

UploadedFile uploadFile(UploadSource const&, UploadOptions const&, UploadProgress const&) {
    missing("uploadFile");
}

bool isUploading() {
    return false;
}

void handleRecording(RecordingOutput const&) {}

BusyScope::BusyScope() {}
BusyScope::~BusyScope() {}

bool isWorking() {
    return false;
}

void notifyIdle() {}
void notify(std::wstring const&, std::wstring const&, std::function<void()>) {}
void writeClipboardText(std::wstring const&) {}
void refreshTray() {}
void playSoundFile(std::wstring const&) {}
void showMainWindow(std::wstring const&) {}
void openDashboard(std::wstring const&) {}
void openWebUrl(std::wstring const&) {}
void runAction(std::wstring const&) {}

void postToMainThread(std::function<void()> work) {
    work();
}

}

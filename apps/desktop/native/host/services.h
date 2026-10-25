#pragma once

#include <shared/http.h>
#include <shared/i18n.h>
#include <shared/json.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace fileship::host {

std::filesystem::path userDataPath();
std::filesystem::path resourcesPath();
std::wstring appVersion();

JsonObject settings();
JsonObject updateSettings(JsonObject const& patch);
JsonValue storeValue(std::wstring const& key);
void setStoreValue(std::wstring const& key, JsonValue const& value);

std::wstring instanceUrl();
std::wstring token();
std::optional<JsonObject> currentUser();
bool hasPermission(std::wstring const& permission);
std::filesystem::path harborRoot();
void setHarborRoot(std::filesystem::path const& root);

struct ApiError : std::runtime_error {
    DWORD status;
    JsonObject data;

    ApiError(std::string const& message, DWORD status, JsonObject data)
        : std::runtime_error(message), status(status), data(std::move(data)) {}
};

HttpEndpoint apiEndpoint();
JsonValue api(std::wstring const& method, std::wstring const& path,
              std::optional<JsonValue> const& body = std::nullopt, int timeoutMs = 30000);
void handleUnauthorized();
void resetUploadSettings();

struct UploadSource {
    std::filesystem::path path;
    std::vector<char> data;
    std::wstring name;
    std::wstring mimeType;
};

struct UploadOptions {
    std::wstring fileNameType = L"Random";
    std::optional<std::wstring> expiration;
    std::optional<int> maxViews;
    std::optional<int> compression;
    std::optional<std::wstring> password;
    std::optional<std::wstring> folderId;
};

struct UploadedFile {
    std::wstring id;
    std::wstring name;
    std::wstring fileName;
    std::wstring url;
    std::wstring directUrl;
};

using UploadProgress = std::function<void(int64_t sent, int64_t total)>;

UploadedFile uploadFile(UploadSource const& source, UploadOptions const& options,
                        UploadProgress const& progress = {});
bool isUploading();

struct RecordingOutput {
    std::filesystem::path path;
    std::wstring kind;
    std::vector<uint8_t> thumbnailJpeg;
};

void handleRecording(RecordingOutput const& output);

class BusyScope {
public:
    BusyScope();
    ~BusyScope();

    BusyScope(BusyScope const&) = delete;
    BusyScope& operator=(BusyScope const&) = delete;
};

bool isWorking();
void notifyIdle();

void notify(std::wstring const& title, std::wstring const& body, std::function<void()> onClick = {});
void writeClipboardText(std::wstring const& text);
void refreshTray();
void playSoundFile(std::wstring const& name);

void showMainWindow(std::wstring const& route = L"/");
void openDashboard(std::wstring const& route = L"/");
void openWebUrl(std::wstring const& url);
void runAction(std::wstring const& action);
void postToMainThread(std::function<void()> work);

}

#pragma once

#include <core/core.h>
#include <shared/http.h>
#include <shared/i18n.h>
#include <shared/json.h>
#include <ui/overlay.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace fileship::host {

using winrt::Windows::Data::Json::IJsonValue;

std::filesystem::path userDataPath();
std::filesystem::path resourcesPath();
std::filesystem::path hostExecutable();
std::wstring appVersion();
bool isPackaged();
std::wstring desktopUserAgent();

JsonObject settings();
JsonObject updateSettings(JsonObject const& patch);
JsonValue storeValue(std::wstring const& key);
void setStoreValue(std::wstring const& key, JsonValue const& value);

std::wstring instanceUrl();
std::wstring token();
std::wstring siteName();
std::optional<JsonObject> currentUser();
bool isConnected();
bool hasPermission(std::wstring const& permission);
std::filesystem::path harborRoot();
void setHarborRoot(std::filesystem::path const& root);

struct ApiError : std::runtime_error {
    DWORD status;
    JsonObject data;

    ApiError(std::string const& message, DWORD status, JsonObject data)
        : std::runtime_error(message), status(status), data(std::move(data)) {}
};

std::runtime_error translatedError(std::wstring const& message);
bool isQuotaExceeded(std::exception const& error);
bool isUnauthorized(std::exception const& error);
std::wstring errorMessage(std::exception const& error);

HttpEndpoint apiEndpoint();
JsonValue api(std::wstring const& method, std::wstring const& path,
              std::optional<IJsonValue> const& body = std::nullopt, int timeoutMs = 30000);
JsonValue apiForm(std::wstring const& path, Multipart& form, int timeoutMs = 30000);
void handleUnauthorized();
void resetUploadSettings();

struct UploadSource {
    std::filesystem::path path;
    std::vector<char> data;
    std::wstring name;
    std::wstring mimeType;
    std::wstring id;
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

UploadOptions uploadOptions();
UploadedFile uploadFile(UploadSource const& source, UploadOptions const& options,
                        UploadProgress const& progress = {});
bool isUploading();
std::wstring shortenUrl(std::wstring const& url);

struct RecordingOutput {
    std::filesystem::path path;
    std::wstring kind;
    std::vector<uint8_t> thumbnailJpeg;
    std::wstring name;
    bool isSaved = false;
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
bool isBusy();
void notifyIdle();

std::wstring mimeTypeOf(std::filesystem::path const& path);
std::wstring formatFileName(std::wstring const& pattern, std::wstring const& title = {});
std::filesystem::path saveFolder();
std::filesystem::path uniquePath(std::filesystem::path const& folder, std::wstring const& name);
std::wstring formatStorageSize(int64_t bytes);
int64_t nowMs();

using AreaCallback = std::function<void(core::Rect)>;

void pickArea(ui::OverlayPurpose purpose, AreaCallback onArea);
bool isCapturing();
void playShutter();
void playUploaded();
ui::ThemeMode themeMode();

void notify(std::wstring const& title, std::wstring const& body, std::function<void()> onClick = {});
void writeClipboardText(std::wstring const& text);
void refreshTray();
void playSoundFile(std::wstring const& name);
void showItemInFolder(std::filesystem::path const& path);
void setShellHarborRoot(std::optional<std::filesystem::path> const& root);

void showMainWindow(std::wstring const& route = L"/");
void openDashboard(std::wstring const& route = L"/dashboard");
void openWebUrl(std::wstring const& url);
void runAction(std::wstring const& action);
void postToMainThread(std::function<void()> work);
void runAsync(std::function<void()> work);
void runAfter(int delayMs, std::function<void()> work);

}

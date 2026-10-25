#pragma once

#include "rpc.h"
#include "services.h"

#include <future>
#include <map>

#undef GetObject

namespace fileship::host {

struct LaunchOptions {
    std::filesystem::path userData;
    bool hidden = false;
    bool dev = false;
    std::wstring rendererUrl;
    std::wstring electronCommand;
    std::wstring updateFeed;
    std::wstring pipe;
    std::vector<std::wstring> arguments;
};

LaunchOptions& launchOptions();
std::wstring pipeName();
std::wstring mutexName();
std::wstring appUserModelId();
HWND mainWindow();
void runMainQueue();
bool isMainThread();
void quitApp();

template <typename Result>
Result runOnMainThread(std::function<Result()> work) {
    if (isMainThread()) return work();

    auto task = std::make_shared<std::packaged_task<Result()>>(std::move(work));
    auto future = task->get_future();
    postToMainThread([task] { (*task)(); });
    return future.get();
}

std::wstring jsonString(JsonObject const& object, wchar_t const* key, std::wstring const& fallback = {});
bool jsonBool(JsonObject const& object, wchar_t const* key, bool fallback = false);
double jsonNumber(JsonObject const& object, wchar_t const* key, double fallback = 0);
JsonObject jsonObject(JsonObject const& object, wchar_t const* key);
JsonValue stringOrNull(std::wstring const& value);
JsonValue stringOrNull(std::optional<std::wstring> const& value);
JsonObject cloneObject(JsonObject const& object);
std::string prettyJson(IJsonValue const& value);
std::wstring randomHex(size_t bytes);
std::wstring randomUuid();
std::wstring base64(void const* data, size_t size);
std::vector<uint8_t> fromBase64(std::wstring const& text);
std::vector<char> readFileBytes(std::filesystem::path const& path);
bool writeFileAtomic(std::filesystem::path const& path, std::string const& content);
std::wstring argumentOf(std::vector<std::wstring> const& arguments, std::wstring const& name);

void loadStore();
JsonObject storeData();
void setSession(std::wstring const& instanceUrl, std::wstring const& siteName, std::wstring const& token,
                JsonObject const& user);
void clearSession();
std::optional<std::string> unprotectElectronValue(std::wstring const& value);

struct HttpResult {
    DWORD status = 0;
    JsonValue body{nullptr};
};

JsonValue request(std::wstring const& baseUrl, std::wstring const& method, std::wstring const& path,
                  std::vector<char> body = {}, std::wstring const& contentType = {},
                  std::wstring const& bearer = {}, int timeoutMs = 30000);
std::wstring normalizeInstanceUrl(std::wstring const& input);
JsonObject checkInstance(std::wstring const& input);
IJsonValue getAccount();
IJsonValue accountJson();
IJsonValue startSession(std::wstring const& url, std::wstring const& sessionToken);
void endSession();

struct UploadSummary {
    size_t count = 0;
    int progress = 0;
};

std::optional<UploadSummary> uploadSummary();
JsonArray uploadProgress();

struct HistoryEntry {
    std::wstring kind;
    std::wstring name;
    int64_t size = 0;
    std::optional<std::wstring> localPath;
    std::optional<std::wstring> thumbnail;
};

std::wstring addHistory(HistoryEntry const& entry);
void updateHistory(std::wstring const& id, std::map<std::wstring, std::optional<std::wstring>> const& patch);
JsonObject listHistory(int64_t offset, int64_t limit);
std::optional<JsonObject> getHistory(std::wstring const& id);
void removeHistory(std::wstring const& id);
void clearHistory();

std::wstring thumbnailOf(core::Bitmap const& bitmap);
void handleImage(core::Bitmap const& image, std::wstring const& title = {}, std::wstring const& kind = L"screenshot");
void handleFile(UploadSource const& source);
std::wstring finishUpload(std::wstring const& historyId, UploadedFile const& file);

bool writeClipboardImage(std::vector<uint8_t> const& png, core::Bitmap const& bitmap);
std::wstring readClipboardText();
std::optional<core::Bitmap> readClipboardImage();
std::optional<std::filesystem::path> readClipboardFilePath();

struct Shot {
    core::Bitmap image;
    std::wstring title;
};

std::vector<uint64_t> ignoredWindows();
void addWindowProcess(DWORD pid);
void removeWindowProcess(DWORD pid);
std::optional<Shot> captureRect(core::Rect const& rect, bool sound = true);
std::optional<Shot> captureFullscreen();
std::optional<Shot> captureActiveMonitor();
std::optional<Shot> captureActiveWindow();
void openRegionCapture(ui::OverlayPurpose purpose = ui::OverlayPurpose::Capture, AreaCallback onArea = {},
                       std::function<void(core::Bitmap)> onImage = {});
void captureLastRegion();
void captureText();

void uploadPaths(std::vector<std::filesystem::path> const& paths);

void createTray();
void removeTray();
bool handleTrayMessage(UINT message, WPARAM wParam, LPARAM lParam);

JsonObject registerHotkeys();
JsonObject hotkeyStatus();
void pauseHotkeys();
void unregisterHotkeys();
void handleHotkey(int id);
bool isSnippingToolKeyEnabled();
JsonObject setSnippingToolKey(bool enabled);

void initNotifications();

void registerShellMenu();
void applyStartAtLogin(bool enabled);
bool handleLaunch(std::vector<std::wstring> const& arguments, std::filesystem::path const& cwd);

void startUpdater();
JsonObject updateStatus();
void checkForUpdates();
void installUpdate();
void installOnQuit();

void registerHandlers();
void applySettings();
void launchElectron(std::vector<std::wstring> const& arguments);
void handleWindowClient(uint64_t client, DWORD pid);
void flushWindowEvents(uint64_t client);
IJsonValue invokeHandler(std::wstring const& channel, JsonArray const& args);
void startRpcServer();
void stopRpcServer();
bool forwardToRunningHost(std::vector<std::wstring> const& arguments);

}

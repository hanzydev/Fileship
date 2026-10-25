#pragma once

#include <shared/json.h>

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace fileship::host::harbor {

struct ScanItem {
    std::wstring path;
    bool isFolder = false;
    std::optional<std::wstring> identity;
    bool inSync = false;
    bool modified = false;
    bool hydrated = false;
    bool isUnreadable = false;
    int64_t size = 0;
};

struct HelperEvent {
    std::wstring type;
    std::wstring identity;
    std::wstring path;
    std::wstring from;
    std::optional<std::wstring> to;
    bool isFolder = false;
    int64_t size = 0;
    uint64_t requestId = 0;
    std::wstring action;
    std::vector<std::wstring> paths;
};

std::filesystem::path helperPath();
bool isHelperAvailable();
std::filesystem::path harborSessionPath();

constexpr int DEFAULT_TIMEOUT_MS = 120000;

class HarborHelper : public std::enable_shared_from_this<HarborHelper> {
public:
    using Callback = std::function<void(std::optional<JsonObject> const& result, std::wstring const& error)>;

    ~HarborHelper();

    void start();
    bool isRunning() const;
    JsonObject request(std::wstring const& type, JsonObject payload = JsonObject(),
                       int timeoutMs = DEFAULT_TIMEOUT_MS);
    void requestAsync(std::wstring const& type, JsonObject payload, Callback callback,
                      int timeoutMs = DEFAULT_TIMEOUT_MS);
    void ack(uint64_t requestId, bool ok);
    void stop();
    std::vector<ScanItem> scan();
    std::optional<std::wstring> identityOf(std::wstring const& path);

    std::function<void(HelperEvent const&)> onEvent;
    std::function<void(std::optional<DWORD>)> onExit;

private:
    struct Pending {
        Callback callback;
        uint64_t timer = 0;
        std::wstring type;
    };

    void write(std::string const& line);
    void readOutput(HANDLE output);
    void readErrors(HANDLE errors);
    void handleLine(std::string const& line);
    void finish();

    std::mutex mutex;
    std::mutex writeMutex;
    HANDLE process = nullptr;
    HANDLE input = nullptr;
    std::atomic<bool> running = false;
    uint64_t nextId = 1;
    std::map<uint64_t, Pending> pending;
};

}

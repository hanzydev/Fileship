#pragma once

#include <shared/json.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace fileship::host {

struct LocalTime {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int millisecond = 0;
};

class RpcChannel {
public:
    virtual ~RpcChannel() = default;

    virtual size_t read(char* buffer, size_t size) = 0;
    virtual bool write(std::string const& data) = 0;
    virtual void close() = 0;
};

std::filesystem::path executablePath();
std::filesystem::path defaultUserDataPath();
std::filesystem::path picturesPath();
std::wstring platformName();
std::wstring electronExecutable();
LocalTime localTime();

void wakeMainThread();
bool replaceFile(std::filesystem::path const& from, std::filesystem::path const& to);
void openExternal(std::wstring const& url);
bool isSystemDark();
bool isNameBefore(std::wstring const& a, std::wstring const& b);
std::vector<std::filesystem::path> pickFiles();

std::wstring storeSecret(std::wstring const& name, std::wstring const& value);
std::wstring loadSecret(std::wstring const& name, std::wstring const& stored);

void applyDesktopIntegration(JsonObject const& settings, bool isLanguageChanged);
void registerPlatformHandlers();

void serveRpcChannel(std::shared_ptr<RpcChannel> channel);
bool sendToRunningHost(std::string const& line);

}

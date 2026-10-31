#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fileship::host {

std::filesystem::path executablePath();
std::filesystem::path defaultUserDataPath();
std::filesystem::path picturesFolder();
std::filesystem::path updatesFolder();
std::wstring platformName();
bool hasPackagedLayout();
std::wstring defaultPipeName(std::wstring const& suffix);

void wakeMainThread();
void stopMainLoop();

std::wstring protectSecret(std::wstring const& name, std::wstring const& value);
std::optional<std::wstring> unprotectSecret(std::wstring const& name, std::wstring const& stored);

bool replaceFile(std::filesystem::path const& from, std::filesystem::path const& to);
std::vector<std::filesystem::path> pickFiles();
bool systemUsesLightTheme();

std::wstring updateManifestName();
void startInstaller(std::filesystem::path const& path, bool forceRun);

class RpcConnection {
public:
    virtual ~RpcConnection() = default;

    virtual size_t read(char* buffer, size_t size) = 0;
    virtual bool write(char const* data, size_t size) = 0;
    virtual void close() = 0;
};

void startRpcListener(std::wstring const& name, std::function<void(std::shared_ptr<RpcConnection>)> onConnection);
void stopRpcListener();
std::shared_ptr<RpcConnection> connectRpc(std::wstring const& name, int timeoutMs);

}

#pragma once

#define WIN32_NO_STATUS
#include <windows.h>
#include <winternl.h>
#undef WIN32_NO_STATUS

#include <unknwn.h>

#include <ntstatus.h>
#include <cfapi.h>

#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>

#define CF_SIZE_OF_OP_PARAM(field)                                                                      (FIELD_OFFSET(CF_OPERATION_PARAMETERS, field) + RTL_FIELD_SIZE(CF_OPERATION_PARAMETERS, field))

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace harbor {

using winrt::Windows::Data::Json::JsonArray;
using winrt::Windows::Data::Json::JsonObject;
using winrt::Windows::Data::Json::JsonValue;

struct Config {
    std::wstring root;
    std::wstring syncRootId;
    std::wstring displayName;
    std::wstring iconPath;
    std::wstring version;
    std::wstring instanceUrl;
    std::wstring userAgent;
    std::wstring appPath;
    JsonObject labels;
    std::vector<std::pair<int32_t, std::wstring>> properties;
};

struct IdentityProbe {
    bool isKnown = false;
    std::optional<std::wstring> identity;
};

struct PlaceholderItem {
    std::wstring identity;
    std::wstring path;
    bool isFolder = false;
    int64_t size = 0;
    int64_t created = 0;
};

Config& config();
std::wstring token();
void setToken(std::wstring const& value);

void send(JsonObject const& message);
void sendEvent(std::wstring const& type, JsonObject message);
void log(std::wstring const& text);

std::wstring fullPath(std::wstring const& relative);
std::optional<std::wstring> relativePath(std::wstring const& absolute);
std::wstring parentOf(std::wstring const& relative);
int depthOf(std::wstring const& relative);
std::wstring identityOf(CF_CALLBACK_INFO const* info);
bool isOwnProcess(CF_CALLBACK_INFO const* info);
bool isIgnoredName(std::wstring const& name);

std::wstring syncRootId();
void registerSyncRoot();
void unregisterSyncRoot(std::wstring const& id);
void unregisterAll();
void connectSyncRoot();
void disconnectSyncRoot();
void reportSyncStatus(std::optional<std::wstring> const& error);
std::wstring label(std::wstring const& key);
JsonArray setStates(JsonArray const& items);
CF_CONNECTION_KEY connectionKey();

void createPlaceholders(std::vector<PlaceholderItem> const& items);
void movePlaceholder(std::wstring const& from, std::wstring const& to);
void updatePlaceholder(std::wstring const& path, std::optional<std::wstring> const& identity,
                       bool resetBaseline = false);
bool isModified(std::wstring const& absolute);
bool refreshPlaceholder(std::wstring const& path, int64_t size);
bool markInSync(std::wstring const& path);
void removePath(std::wstring const& path);
void adoptPath(std::wstring const& path, std::wstring const& identity);
std::wstring preparePath(std::wstring const& path, std::wstring const& identity);
void reportProgress(std::wstring const& path, int64_t total, int64_t completed);
JsonArray scanRoot();
int64_t localSize();
int64_t freeUpSpace();
std::optional<std::wstring> placeholderIdentity(std::wstring const& absolute, CF_PIN_STATE* pin,
                                                CF_IN_SYNC_STATE* sync);
IdentityProbe probeIdentity(std::wstring const& absolute, CF_PIN_STATE* pin, CF_IN_SYNC_STATE* sync);
void hydratePath(std::wstring const& absolute);
void setPinState(std::wstring const& absolute, CF_PIN_STATE pin);
void dehydratePath(std::wstring const& absolute);

void fetchData(CF_CALLBACK_INFO const* info, CF_CALLBACK_PARAMETERS const* parameters);
void cancelFetch(CF_CALLBACK_INFO const* info);

void startWatcher();
void stopWatcher();
void settle(std::wstring const& folder);
void settleItem(std::wstring const& path);

void reportDeleted(CF_CALLBACK_INFO const* info);
void askRename(CF_CALLBACK_INFO const* info, CF_CALLBACK_PARAMETERS const* parameters);
void renamed(CF_CALLBACK_INFO const* info, CF_CALLBACK_PARAMETERS const* parameters);
void resolveAck(uint64_t requestId, bool ok);

std::wstring toWide(std::string const& text);
std::string toUtf8(std::wstring const& text);

}

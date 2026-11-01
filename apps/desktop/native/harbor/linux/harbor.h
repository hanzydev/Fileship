#pragma once

#include <shared/json.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace harbor {

using fileship::JsonArray;
using fileship::JsonObject;
using fileship::JsonValue;

struct Config {
    std::filesystem::path root;
    std::filesystem::path cache;
    std::wstring instanceUrl;
    std::wstring userAgent;
};

struct Node {
    std::string name;
    bool isFolder = false;
    std::wstring identity;
    bool inSync = false;
    bool isHydrated = false;
    bool isPinned = false;
    int64_t size = 0;
    int64_t created = 0;
    int64_t baseline = 0;
    uint64_t blob = 0;
    int openCount = 0;
    Node* parent = nullptr;
    std::map<std::string, std::unique_ptr<Node>> children;
};

struct PlaceholderItem {
    std::wstring identity;
    std::string path;
    bool isFolder = false;
    int64_t size = 0;
    int64_t created = 0;
};

Config& config();
std::wstring token();
void setToken(std::wstring const& value);

void send(JsonObject const& message);
void sendEvent(std::wstring const& type, JsonObject message);
void log(std::string const& text);

std::recursive_mutex& treeMutex();
Node* rootNode();
Node* find(std::string const& relative);
std::string relativeOf(Node const* node);
std::string parentOf(std::string const& relative);
std::string nameOf(std::string const& relative);
std::filesystem::path blobPath(uint64_t blob);
uint64_t newBlob();
int64_t blobMtime(uint64_t blob);
int64_t blobSize(uint64_t blob);
bool isModified(Node const* node);
bool isOwned(Node const* node);
bool isIgnoredName(std::string const& name);
bool childrenInSync(Node const* node);
std::wstring stampOf(Node const* node);
Node* attach(Node* parent, std::unique_ptr<Node> node);
std::unique_ptr<Node> detach(Node* node);
void dropContent(Node* node);
void markDirty();
void loadState();
void saveState();
void forEachNode(Node* node, std::function<void(Node*)> const& visit);

void createPlaceholders(std::vector<PlaceholderItem> const& items);
void movePlaceholder(std::string const& from, std::string const& to);
void updatePlaceholder(std::string const& path, std::optional<std::wstring> const& identity);
bool refreshPlaceholder(std::string const& path, int64_t size);
bool markInSync(std::string const& path);
bool removePath(std::string const& path);
void adoptPath(std::string const& path, std::wstring const& identity, bool isChanged);
JsonArray unsyncedChildren(std::string const& path);
std::wstring preparePath(std::string const& path, std::wstring const& identity);
JsonArray scanRoot();
int64_t localSize();
int64_t freeUpSpace(Node* folder = nullptr);
void materialize();

void mountRoot();
void unmountRoot();
void startDownload(std::wstring const& identity);
void hydrateNow(std::string const& relative);
void resolveAck(uint64_t requestId, bool ok);

void startWatcher();
void stopWatcher();
void touch(std::string const& relative);
void forget(std::string const& relative);
void recheck(std::string const& relative);

}

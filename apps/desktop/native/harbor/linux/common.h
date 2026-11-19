#pragma once

#include <shared/http.h>
#include <shared/json.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace harbor {

using fileship::JsonArray;
using fileship::JsonObject;
using fileship::JsonValue;

struct Config {
    std::string root;
    std::string backing;
    std::wstring instanceUrl;
    std::wstring userAgent;
    std::wstring token;
};

struct Entry {
    bool exists = false;
    bool isFolder = false;
    int64_t size = 0;
    std::optional<std::string> identity;
    bool inSync = false;
    std::optional<int64_t> remote;
    std::string base;

    bool isOwned() const {
        return identity && !identity->empty() && !identity->starts_with("p:");
    }
};

struct Item {
    std::string identity;
    std::string path;
    bool isFolder = false;
    int64_t size = 0;
    int64_t created = 0;
};

Config& config();
void send(JsonObject const& message);
void sendEvent(std::string const& type, JsonObject message);
void log(std::string const& text);
bool askHost(std::string const& type, JsonObject message);
JsonValue text(std::string const& value);
JsonValue number(int64_t value);

std::string backingOf(std::string const& relative);
std::string parentOf(std::string const& relative);
bool isIgnored(std::string const& relative);
Entry entryOf(std::string const& relative);
std::string stampOf(std::string const& absolute);
bool isModified(std::string const& relative, Entry const& entry);
void setAttribute(std::string const& relative, char const* name, std::string const& value);
void clearAttribute(std::string const& relative, char const* name);
void forgetEntry(std::string const& relative);
void copyIdentity(std::string const& from, std::string const& to);

void createPlaceholders(std::vector<Item> const& items);
void movePlaceholder(std::string const& from, std::string const& to);
void updatePlaceholder(std::string const& path, std::optional<std::string> const& identity);
bool refreshPlaceholder(std::string const& path, int64_t size);
bool removePath(std::string const& path);
void adoptPath(std::string const& path, std::string const& identity, bool isChanged);
JsonArray unsyncedChildren(std::string const& path);
std::string preparePath(std::string const& path, std::string const& identity);
bool markInSync(std::string const& path);
JsonArray scanRoot();
int64_t localSize();
int64_t freeUpSpace();
void finishHydration(std::string const& relative);
void releaseLocal();

void fetchRange(std::string const& relative, int64_t offset, int64_t length);
void fetchAll(std::string const& relative);
void forgetBlocks(std::string const& relative);
fileship::HttpEndpoint endpoint();

void queueThumbnail(std::string const& relative);
void stopThumbnails();

void mountRoot();
void unmountRoot();
bool isOpenForWriting(std::string const& relative);
void scheduleChange(std::string const& relative);
void recheck(std::string const& relative);

constexpr char const* IDENTITY = "user.fileship.id";
constexpr char const* IN_SYNC = "user.fileship.sync";
constexpr char const* REMOTE = "user.fileship.remote";
constexpr char const* BASE = "user.fileship.base";

}

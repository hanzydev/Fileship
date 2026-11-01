#include "harbor.h"

#include <shared/utf8.h>

#include <glib.h>

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <clocale>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>
#include <thread>

namespace harbor {

namespace {

Config currentConfig;
std::wstring currentToken;
std::mutex tokenMutex;
std::mutex outputMutex;

std::string utf8(JsonObject const& object, wchar_t const* key) {
    return fileship::toUtf8(object.GetNamedString(key));
}

std::string folderName(std::wstring const& value) {
    std::string result;
    for (auto character : fileship::toUtf8(value)) {
        result.push_back(std::isalnum(static_cast<unsigned char>(character)) || character == '-' ? character : '_');
    }
    return result;
}

std::vector<std::string> mountsOf(std::string const& source) {
    std::vector<std::string> mounts;
    std::ifstream file("/proc/self/mountinfo");

    for (std::string line; std::getline(file, line);) {
        auto separator = line.find(" - ");
        if (separator == std::string::npos) continue;

        std::istringstream before(line.substr(0, separator));
        std::istringstream after(line.substr(separator + 3));
        std::string id, parent, device, root, point, type, name;
        before >> id >> parent >> device >> root >> point;
        after >> type >> name;
        if (type == "fuse.harbor" && name == source) mounts.push_back(point);
    }

    return mounts;
}

void unmountStale(std::filesystem::path const& folder) {
    for (auto const& point : mountsOf("fileship")) {
        std::error_code error;
        if (!folder.empty() && !std::filesystem::equivalent(point, folder, error) && point != folder.string()) continue;

        pid_t pid = fork();
        if (pid == 0) {
            execlp("fusermount3", "fusermount3", "-u", "-z", point.c_str(), static_cast<char*>(nullptr));
            _exit(127);
        }
        if (pid > 0) waitpid(pid, nullptr, 0);
    }
}

JsonObject reply(JsonObject const& request, bool ok) {
    JsonObject message;
    message.SetNamedValue(L"type", JsonValue::CreateStringValue(L"result"));
    message.SetNamedValue(L"id", JsonValue::CreateNumberValue(request.GetNamedNumber(L"id", 0)));
    message.SetNamedValue(L"ok", JsonValue::CreateBooleanValue(ok));
    return message;
}

std::vector<PlaceholderItem> readItems(JsonObject const& request) {
    std::vector<PlaceholderItem> items;

    for (auto const& value : request.GetNamedArray(L"items")) {
        auto object = value.GetObject();
        items.push_back(PlaceholderItem{
            object.GetNamedString(L"identity"),
            utf8(object, L"path"),
            object.GetNamedBoolean(L"isFolder", false),
            static_cast<int64_t>(object.GetNamedNumber(L"size", 0)),
            static_cast<int64_t>(object.GetNamedNumber(L"created", 0)),
        });
    }

    return items;
}

void handle(JsonObject const& request) {
    std::wstring type = request.GetNamedString(L"type");
    auto response = reply(request, true);

    try {
        if (type == L"init") {
            currentConfig.root = utf8(request, L"root");
            currentConfig.instanceUrl = request.GetNamedString(L"instanceUrl");
            currentConfig.userAgent = request.GetNamedString(L"userAgent");
            auto cacheHome = std::getenv("XDG_CACHE_HOME");
            auto cache = cacheHome && *cacheHome == '/' ? std::filesystem::path(cacheHome)
                                                        : std::filesystem::path(g_get_home_dir()) / ".cache";
            currentConfig.cache = cache / "Fileship" / "harbor" / folderName(request.GetNamedString(L"syncRootId"));
            setToken(request.GetNamedString(L"token"));

            std::filesystem::create_directories(currentConfig.cache / "blobs");
            unmountStale(currentConfig.root);
            if (rootNode()->children.empty()) loadState();
            mountRoot();
            startWatcher();
        } else if (type == L"create") {
            createPlaceholders(readItems(request));
        } else if (type == L"move") {
            movePlaceholder(utf8(request, L"from"), utf8(request, L"to"));
        } else if (type == L"update") {
            std::optional<std::wstring> identity;
            if (request.HasKey(L"identity")) identity = request.GetNamedString(L"identity");
            updatePlaceholder(utf8(request, L"path"), identity);
        } else if (type == L"remove") {
            response.SetNamedValue(L"kept", JsonValue::CreateBooleanValue(removePath(utf8(request, L"path"))));
        } else if (type == L"adopt") {
            auto path = utf8(request, L"path");
            std::wstring stamp = request.GetNamedString(L"stamp", L"");
            bool isChanged = false;

            {
                std::lock_guard lock(treeMutex());
                auto node = find(path);
                if (!node) throw std::runtime_error("The path was not found");
                isChanged = !stamp.empty() && stampOf(node) != stamp;
            }

            adoptPath(path, request.GetNamedString(L"identity"), isChanged);
            if (isChanged) recheck(path);
            response.SetNamedValue(L"items", unsyncedChildren(path));
        } else if (type == L"markInSync") {
            std::vector<std::string> paths;
            for (auto const& value : request.GetNamedArray(L"paths")) paths.push_back(fileship::toUtf8(value.GetString()));
            std::ranges::sort(paths, std::greater{}, [](auto const& path) { return std::ranges::count(path, '/'); });

            JsonArray marked;
            for (auto const& path : paths) {
                if (markInSync(path)) marked.Append(JsonValue::CreateStringValue(fileship::toWide(path)));
            }
            response.SetNamedValue(L"marked", marked);
        } else if (type == L"prepare") {
            auto identity = preparePath(utf8(request, L"path"), request.GetNamedString(L"identity"));
            response.SetNamedValue(L"identity", JsonValue::CreateStringValue(identity));
        } else if (type == L"progress" || type == L"syncStatus") {
        } else if (type == L"setStates") {
            response.SetNamedValue(L"failed", JsonArray());
        } else if (type == L"scan") {
            response.SetNamedValue(L"items", scanRoot());
        } else if (type == L"identity") {
            std::lock_guard lock(treeMutex());
            auto node = find(utf8(request, L"path"));
            response.SetNamedValue(L"exists", JsonValue::CreateBooleanValue(node != nullptr));
            response.SetNamedValue(L"identity", node && !node->identity.empty()
                                                    ? JsonValue::CreateStringValue(node->identity)
                                                    : JsonValue::CreateNullValue());
        } else if (type == L"ack") {
            resolveAck(static_cast<uint64_t>(request.GetNamedNumber(L"requestId")), request.GetNamedBoolean(L"ok"));
            return;
        } else if (type == L"refresh") {
            bool refreshed = refreshPlaceholder(utf8(request, L"path"),
                                                static_cast<int64_t>(request.GetNamedNumber(L"size")));
            response.SetNamedValue(L"refreshed", JsonValue::CreateBooleanValue(refreshed));
        } else if (type == L"usage") {
            response.SetNamedValue(L"local", JsonValue::CreateNumberValue(static_cast<double>(localSize())));
        } else if (type == L"freeUpSpace") {
            response.SetNamedValue(L"freed", JsonValue::CreateNumberValue(static_cast<double>(freeUpSpace())));
        } else if (type == L"unregister") {
            stopWatcher();
            unmountRoot();
            materialize();
            {
                std::lock_guard lock(treeMutex());
                rootNode()->children.clear();
            }
            std::error_code error;
            std::filesystem::remove_all(currentConfig.cache, error);
        } else if (type == L"stop") {
            stopWatcher();
            unmountRoot();
            if (!currentConfig.cache.empty() && std::filesystem::exists(currentConfig.cache)) saveState();
            send(response);
            std::fflush(stdout);
            _exit(0);
        } else {
            throw std::runtime_error("Unknown command");
        }
    } catch (std::exception const& error) {
        response = reply(request, false);
        response.SetNamedValue(L"error", JsonValue::CreateStringValue(fileship::toWide(error.what())));
    }

    if (request.HasKey(L"id")) send(response);
}

}

Config& config() {
    return currentConfig;
}

std::wstring token() {
    std::lock_guard lock(tokenMutex);
    return currentToken;
}

void setToken(std::wstring const& value) {
    std::lock_guard lock(tokenMutex);
    currentToken = value;
}

void send(JsonObject const& message) {
    auto line = fileship::toUtf8(message.Stringify()) + "\n";
    std::lock_guard lock(outputMutex);
    std::fwrite(line.data(), 1, line.size(), stdout);
    std::fflush(stdout);
}

void sendEvent(std::wstring const& type, JsonObject message) {
    message.SetNamedValue(L"type", JsonValue::CreateStringValue(type));
    send(message);
}

void log(std::string const& text) {
    auto line = text + "\n";
    std::lock_guard lock(outputMutex);
    std::fwrite(line.data(), 1, line.size(), stderr);
    std::fflush(stderr);
}

}

int main(int argc, char** argv) {
    std::setlocale(LC_ALL, "");
    try {
        std::locale::global(std::locale("C.UTF-8"));
    } catch (std::runtime_error const&) {
    }
    signal(SIGPIPE, SIG_IGN);

    std::string command = argc > 1 ? argv[1] : "";
    if (command == "--unregister-all") {
        harbor::unmountStale({});
        return 0;
    }

    for (std::string line; std::getline(std::cin, line);) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (line.empty()) continue;

        harbor::JsonObject request;
        if (!harbor::JsonObject::TryParse(fileship::toWide(line), request)) {
            harbor::log("Invalid message: " + line);
            continue;
        }

        std::thread([request] { harbor::handle(request); }).detach();
    }

    harbor::stopWatcher();
    harbor::unmountRoot();
    if (!harbor::config().cache.empty()) harbor::saveState();
    std::fflush(stdout);
    _exit(0);
}

#include "common.h"

#include <shared/utf8.h>

#include <glib.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <map>
#include <mutex>

namespace harbor {

namespace {

using fileship::toUtf8;
using fileship::toWide;

constexpr auto ANSWER_TIME = std::chrono::seconds(60);

Config currentConfig;
std::mutex outputMutex;
std::mutex answerMutex;
std::condition_variable answerSignal;
std::map<uint64_t, std::optional<bool>> answers;
uint64_t nextRequest = 1;

std::string field(JsonObject const& request, wchar_t const* key) {
    return toUtf8(request.GetNamedString(key));
}

JsonObject reply(JsonObject const& request, bool ok) {
    JsonObject message;
    message.SetNamedValue(L"type", JsonValue::CreateStringValue(L"result"));
    message.SetNamedValue(L"id", JsonValue::CreateNumberValue(request.GetNamedNumber(L"id", 0)));
    message.SetNamedValue(L"ok", JsonValue::CreateBooleanValue(ok));
    return message;
}

std::vector<Item> readItems(JsonObject const& request) {
    std::vector<Item> items;

    for (auto const& value : request.GetNamedArray(L"items")) {
        auto object = value.GetObject();
        items.push_back({field(object, L"identity"), field(object, L"path"), object.GetNamedBoolean(L"isFolder", false),
                         static_cast<int64_t>(object.GetNamedNumber(L"size", 0)),
                         static_cast<int64_t>(object.GetNamedNumber(L"created", 0))});
    }

    return items;
}

void resolve(uint64_t id, bool ok) {
    std::lock_guard lock(answerMutex);
    if (auto found = answers.find(id); found != answers.end()) found->second = ok;
    answerSignal.notify_all();
}

void handle(JsonObject const& request) {
    auto type = request.GetNamedString(L"type");
    auto response = reply(request, true);

    try {
        if (type == L"init") {
            currentConfig.root = field(request, L"root");
            currentConfig.backing = std::string(g_get_user_data_dir()) + "/fileship/harbor/" + field(request, L"syncRootId");
            currentConfig.instanceUrl = request.GetNamedString(L"instanceUrl");
            currentConfig.userAgent = request.GetNamedString(L"userAgent");
            currentConfig.token = request.GetNamedString(L"token");

            std::filesystem::create_directories(currentConfig.root);
            std::filesystem::create_directories(currentConfig.backing);
            mountRoot();
        } else if (type == L"create") {
            createPlaceholders(readItems(request));
        } else if (type == L"move") {
            movePlaceholder(field(request, L"from"), field(request, L"to"));
        } else if (type == L"update") {
            updatePlaceholder(field(request, L"path"), request.HasKey(L"identity")
                                                           ? std::optional(field(request, L"identity"))
                                                           : std::nullopt);
        } else if (type == L"remove") {
            response.SetNamedValue(L"kept", JsonValue::CreateBooleanValue(removePath(field(request, L"path"))));
        } else if (type == L"adopt") {
            auto path = field(request, L"path");
            auto stamp = toUtf8(request.GetNamedString(L"stamp", L""));
            bool isChanged = !stamp.empty() && stampOf(backingOf(path)) != stamp;

            adoptPath(path, field(request, L"identity"), isChanged);
            if (isChanged) recheck(path);
            response.SetNamedValue(L"items", unsyncedChildren(path));
        } else if (type == L"markInSync") {
            std::vector<std::string> paths;
            for (auto const& value : request.GetNamedArray(L"paths")) paths.push_back(toUtf8(value.GetString()));
            std::ranges::sort(paths, std::greater{}, [](std::string const& path) { return std::ranges::count(path, '/'); });

            JsonArray marked;
            for (auto const& path : paths) {
                if (markInSync(path)) marked.Append(text(path));
            }

            response.SetNamedValue(L"marked", marked);
        } else if (type == L"prepare") {
            response.SetNamedValue(L"identity", text(preparePath(field(request, L"path"), field(request, L"identity"))));
        } else if (type == L"progress" || type == L"syncStatus") {
        } else if (type == L"setStates") {
            response.SetNamedValue(L"failed", JsonArray());
        } else if (type == L"scan") {
            response.SetNamedValue(L"items", scanRoot());
        } else if (type == L"identity") {
            auto entry = entryOf(field(request, L"path"));
            response.SetNamedValue(L"exists", JsonValue::CreateBooleanValue(entry.exists));
            response.SetNamedValue(L"identity", entry.identity && !entry.identity->empty()
                                                    ? text(*entry.identity)
                                                    : JsonValue::CreateNullValue());
        } else if (type == L"ack") {
            resolve(static_cast<uint64_t>(request.GetNamedNumber(L"requestId")), request.GetNamedBoolean(L"ok"));
            return;
        } else if (type == L"refresh") {
            bool isRefreshed = refreshPlaceholder(field(request, L"path"),
                                                  static_cast<int64_t>(request.GetNamedNumber(L"size")));
            response.SetNamedValue(L"refreshed", JsonValue::CreateBooleanValue(isRefreshed));
        } else if (type == L"usage") {
            response.SetNamedValue(L"local", number(localSize()));
        } else if (type == L"freeUpSpace") {
            response.SetNamedValue(L"freed", number(freeUpSpace()));
        } else if (type == L"unregister") {
            unmountRoot();
            releaseLocal();
        } else if (type == L"stop") {
            unmountRoot();
            send(response);
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown command");
        }
    } catch (std::exception const& error) {
        response = reply(request, false);
        response.SetNamedValue(L"error", text(error.what()));
    }

    if (request.HasKey(L"id")) send(response);
}

}

Config& config() {
    return currentConfig;
}

void send(JsonObject const& message) {
    auto line = toUtf8(message.Stringify()) + "\n";
    std::lock_guard lock(outputMutex);
    std::cout << line << std::flush;
}

void sendEvent(std::string const& type, JsonObject message) {
    message.SetNamedValue(L"type", text(type));
    send(message);
}

void log(std::string const& text) {
    std::lock_guard lock(outputMutex);
    std::cerr << text << std::endl;
}

bool askHost(std::string const& type, JsonObject message) {
    uint64_t id = 0;

    {
        std::lock_guard lock(answerMutex);
        id = nextRequest++;
        answers[id] = std::nullopt;
    }

    message.SetNamedValue(L"requestId", number(static_cast<int64_t>(id)));
    sendEvent(type, message);

    std::unique_lock lock(answerMutex);
    answerSignal.wait_for(lock, ANSWER_TIME, [&] { return answers[id].has_value(); });

    bool isAllowed = answers[id].value_or(false);
    answers.erase(id);
    return isAllowed;
}

JsonValue text(std::string const& value) {
    return JsonValue::CreateStringValue(toWide(value));
}

JsonValue number(int64_t value) {
    return JsonValue::CreateNumberValue(static_cast<double>(value));
}

}

int main() {
    signal(SIGPIPE, SIG_IGN);

    std::string line;

    while (std::getline(std::cin, line)) {
        fileship::JsonObject request;

        if (fileship::JsonObject::TryParse(fileship::toWide(line), request)) {
            harbor::handle(request);
        } else if (!line.empty()) {
            harbor::log("Invalid command: " + line);
        }
    }

    harbor::unmountRoot();
    return 0;
}

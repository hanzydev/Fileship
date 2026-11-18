#include "ffmpeg.h"

#include "command.h"
#include "platform.h"
#include "process.h"

#include <host/app.h>
#include <host/rpc.h>
#include <shared/log.h>

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <regex>
#include <sstream>

namespace fs = std::filesystem;

namespace fileship::host::recording {

namespace {

std::mutex probeMutex;
std::optional<Ffmpeg> probed;

std::wstring probeKey(std::wstring const& path) {
    return path + L"|" + std::to_wstring(fs::file_size(path)) + L"|" +
           std::to_wstring(fs::last_write_time(path).time_since_epoch().count());
}

Ffmpeg runProbe(std::wstring const& path) {
    auto key = probeKey(path);
    auto cached = storeValue(L"ffmpegProbe");

    if (cached.ValueType() == JsonValueType::Object) {
        auto object = JsonObject::Parse(cached.Stringify());

        if (object.GetNamedString(L"key", L"") == key) {
            Ffmpeg result{path, std::wstring(object.GetNamedString(L"version", L"unknown")),
                          object.GetNamedBoolean(L"ddagrab", false), {}};
            for (auto const& item : object.GetNamedArray(L"encoders", JsonArray())) {
                if (item.ValueType() == JsonValueType::String) result.encoders.emplace_back(item.GetString());
            }
            return result;
        }
    }

    auto version = runProcess(path, {L"-hide_banner", L"-version"}).out;
    auto filters = runProcess(path, {L"-hide_banner", L"-filters"}).out;
    auto encoders = runProcess(path, {L"-hide_banner", L"-encoders"}).out;

    std::vector<std::wstring> available;
    std::istringstream lines(encoders);

    for (std::string line; std::getline(lines, line);) {
        std::istringstream words(line);
        std::string flags;
        std::string name;

        if (words >> flags >> name && flags[0] == 'V') available.push_back(toWide(name));
    }

    std::vector<std::wstring> working;
    for (auto const& name : available) {
        if (name.find(L'_') == std::wstring::npos) working.push_back(name);
    }

    static std::wregex const hardwarePattern(L"_(nvenc|amf|qsv)$");

    for (auto const& encoder : available) {
        if (std::find(VIDEO_CODECS.begin(), VIDEO_CODECS.end(), encoder) == VIDEO_CODECS.end()) continue;
        if (!std::regex_search(encoder, hardwarePattern)) continue;

        try {
            runProcess(path, {L"-hide_banner", L"-loglevel", L"error", L"-f", L"lavfi", L"-i",
                              L"color=c=black:s=256x256:r=30", L"-frames:v", L"3", L"-c:v", encoder, L"-f",
                              L"null", L"-"});
            working.push_back(encoder);
        } catch (...) {
        }
    }

    std::smatch match;
    Ffmpeg result{path,
                  std::regex_search(version, match, std::regex("ffmpeg version (\\S+)")) ? toWide(match[1].str())
                                                                                        : L"unknown",
                  std::regex_search(filters, std::regex("\\bddagrab\\b")), working};

    JsonObject cache;
    JsonArray list;
    for (auto const& encoder : result.encoders) list.Append(JsonValue::CreateStringValue(encoder));

    cache.SetNamedValue(L"key", JsonValue::CreateStringValue(key));
    cache.SetNamedValue(L"version", JsonValue::CreateStringValue(result.version));
    cache.SetNamedValue(L"ddagrab", JsonValue::CreateBooleanValue(result.ddagrab));
    cache.SetNamedValue(L"encoders", list);
    setStoreValue(L"ffmpegProbe", jsonValue(cache));

    return result;
}

}

std::optional<Ffmpeg> getFfmpeg() {
    auto path = ffmpegPath();
    if (!path) return std::nullopt;

    std::lock_guard lock(probeMutex);
    if (probed && probed->path == *path) return probed;

    try {
        probed = runProbe(*path);
        return probed;
    } catch (std::exception const& error) {
        logWarn(L"Could not run ffmpeg: " + toWide(error.what()));
        return std::nullopt;
    }
}

std::wstring autoCodec(std::vector<std::wstring> const& encoders) {
    for (auto const& codec : HARDWARE_CODECS) {
        if (std::find(encoders.begin(), encoders.end(), codec) != encoders.end()) return codec;
    }
    return L"libx264";
}

JsonObject ffmpegStatus() {
    auto info = getFfmpeg();
    auto custom = text(section(settings(), L"recording"), L"ffmpegPath");
    auto string = [](std::optional<std::wstring> const& value) {
        return value ? JsonValue::CreateStringValue(*value) : JsonValue::CreateNullValue();
    };

    JsonObject status;
    JsonArray encoders;
    if (info) {
        for (auto const& encoder : info->encoders) encoders.Append(JsonValue::CreateStringValue(encoder));
    }

    status.SetNamedValue(L"path", string(info ? std::optional(info->path)
                                              : custom.empty() ? std::nullopt : std::optional(custom)));
    status.SetNamedValue(L"version", string(info ? std::optional(info->version) : std::nullopt));
    status.SetNamedValue(L"isManaged", JsonValue::CreateBooleanValue(custom.empty()));
    status.SetNamedValue(L"isInstalled", JsonValue::CreateBooleanValue(info.has_value()));
    status.SetNamedValue(L"ddagrab", JsonValue::CreateBooleanValue(info && info->ddagrab));
    status.SetNamedValue(L"encoders", encoders);
    describeFfmpeg(status);

    return status;
}

void publishFfmpegStatus() {
    broadcast(L"ffmpeg:status", jsonValue(ffmpegStatus()));
}

void clearFfmpegProbe() {
    std::lock_guard lock(probeMutex);
    probed.reset();
}

}

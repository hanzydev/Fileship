#include "ffmpeg.h"

#include "command.h"
#include "process.h"

#include <host/rpc.h>
#include <host/services.h>
#include <shared/hash.h>
#include <shared/log.h>


#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <regex>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;

namespace fileship::host::recording {

namespace {

struct Build {
    wchar_t const* version;
    wchar_t const* url;
    char const* sha256;
};

constexpr Build BUILD = {
    L"9.0.2",
    L"https://github.com/GyanD/codexffmpeg/releases/download/9.0.2/ffmpeg-9.0.2-essentials_build.zip",
    "60f467265b1e312373dbcd92200c2618a74850f98d3d078e94296bb3fa2047ba",
};

struct Download {
    int progress = 0;
    std::optional<std::wstring> error;
};

std::mutex probeMutex;
std::optional<Ffmpeg> probed;

std::mutex downloadMutex;
std::optional<Download> download;

fs::path managedDir() {
    return userDataPath() / L"ffmpeg";
}

fs::path managedPath() {
    return managedDir() / L"ffmpeg.exe";
}

fs::path tempFolder() {
    wchar_t buffer[MAX_PATH + 1]{};
    GetTempPathW(MAX_PATH + 1, buffer);
    return fs::path(buffer);
}

bool fileExists(fs::path const& path) {
    std::error_code error;
    return fs::is_regular_file(path, error);
}

std::wstring probeKey(std::wstring const& path) {
    WIN32_FILE_ATTRIBUTE_DATA data{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data)) {
        throw std::runtime_error("Could not read " + toUtf8(path));
    }

    auto size = (static_cast<uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
    auto time = (static_cast<uint64_t>(data.ftLastWriteTime.dwHighDateTime) << 32) |
                data.ftLastWriteTime.dwLowDateTime;
    auto mtimeMs = (time - 116444736000000000ULL) / 10000;

    return path + L"|" + std::to_wstring(size) + L"|" + std::to_wstring(mtimeMs);
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

void setDownload(std::optional<Download> value) {
    std::lock_guard lock(downloadMutex);
    download = std::move(value);
}

void fetchArchive(fs::path const& archive) {
    std::wstring url = BUILD.url;
    auto slash = url.find(L'/', url.find(L"://") + 3);

    std::optional<HttpStream> stream;
    try {
        stream.emplace(HttpEndpoint{url.substr(0, slash), {}, desktopUserAgent(), {}},
                       HttpRequest{L"GET", url.substr(slash), {}, {}, 60000});
    } catch (std::exception const& error) {
        throw std::runtime_error(std::string("Could not connect (") + error.what() + ")");
    }

    if (stream->status < 200 || stream->status >= 300) {
        throw std::runtime_error(
            toUtf8(t(L"recording.downloadFailed", {{L"status", std::to_wstring(stream->status)}})));
    }

    uint64_t total = stream->contentLength > 0 ? static_cast<uint64_t>(stream->contentLength) : 0;

    std::ofstream file(archive, std::ios::binary);
    if (!file) throw std::runtime_error("Could not write " + archive.string());

    Hasher hash(HashKind::Sha256);
    std::vector<char> buffer(256 * 1024);
    uint64_t received = 0;
    auto lastPublish = std::chrono::steady_clock::now();

    for (;;) {
        size_t read = 0;
        try {
            read = stream->read(buffer.data(), buffer.size());
        } catch (std::exception const& error) {
            throw std::runtime_error(std::string("Download interrupted (") + error.what() + ")");
        }
        if (!read) break;

        hash.update(buffer.data(), read);
        file.write(buffer.data(), read);
        if (!file) throw std::runtime_error("Could not write " + archive.string());
        received += read;

        auto now = std::chrono::steady_clock::now();
        if (total && now - lastPublish > std::chrono::milliseconds(250)) {
            lastPublish = now;
            setDownload(Download{static_cast<int>((received * 200 / total + 1) / 2), {}});
            publishFfmpegStatus();
        }
    }

    file.close();
    if (!file) throw std::runtime_error("Could not write " + archive.string());

    if (toHex(hash.finish()) != BUILD.sha256) throw std::runtime_error(toUtf8(t(L"recording.downloadDamaged")));
}

void runDownload() {
    auto stamp = std::to_wstring(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
    auto archive = tempFolder() / (L"fileship-ffmpeg-" + stamp + L".zip");
    auto extracted = tempFolder() / (L"fileship-ffmpeg-" + stamp);
    std::error_code ignored;

    try {
        fetchArchive(archive);

        fs::create_directories(extracted);

        wchar_t system[MAX_PATH]{};
        GetSystemDirectoryW(system, MAX_PATH);
        runProcess((fs::path(system) / L"tar.exe").wstring(),
                   {L"-xf", archive.wstring(), L"-C", extracted.wstring(), L"--strip-components", L"2",
                    L"*/bin/ffmpeg.exe"});

        auto binary = extracted / L"ffmpeg.exe";
        if (!fileExists(binary)) throw std::runtime_error(toUtf8(t(L"recording.ffmpegNotInDownload")));

        fs::create_directories(managedDir());
        fs::remove(managedPath(), ignored);
        fs::rename(binary, managedPath());

        setDownload(std::nullopt);
        clearFfmpegProbe();
    } catch (std::exception const& error) {
        logError(L"Could not download ffmpeg: " + toWide(error.what()));
        setDownload(Download{0, toWide(error.what())});
    }

    fs::remove(archive, ignored);
    fs::remove_all(extracted, ignored);
    publishFfmpegStatus();
}

}

std::optional<std::wstring> ffmpegPath() {
    auto custom = text(section(settings(), L"recording"), L"ffmpegPath");
    if (!custom.empty()) return fileExists(custom) ? std::optional(custom) : std::nullopt;
    return fileExists(managedPath()) ? std::optional(managedPath().wstring()) : std::nullopt;
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
    status.SetNamedValue(L"managedVersion", JsonValue::CreateStringValue(BUILD.version));

    std::lock_guard lock(downloadMutex);
    if (download) {
        JsonObject progress;
        progress.SetNamedValue(L"progress", JsonValue::CreateNumberValue(download->progress));
        progress.SetNamedValue(L"error", string(download->error));
        status.SetNamedValue(L"download", progress);
    } else {
        status.SetNamedValue(L"download", JsonValue::CreateNullValue());
    }

    return status;
}

void publishFfmpegStatus() {
    broadcast(L"ffmpeg:status", jsonValue(ffmpegStatus()));
}

void clearFfmpegProbe() {
    std::lock_guard lock(probeMutex);
    probed.reset();
}

void downloadFfmpeg() {
    {
        std::lock_guard lock(downloadMutex);
        if (download && !download->error) return;
        download = Download{};
    }

    publishFfmpegStatus();
    std::thread(runDownload).detach();
}

}

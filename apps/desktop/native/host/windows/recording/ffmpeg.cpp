#include <host/recording/command.h>
#include <host/recording/ffmpeg.h>
#include <host/recording/platform.h>
#include <host/recording/process.h>
#include <host/windows/host.h>
#include <shared/log.h>
#include <shared/windows/winhttp.h>

#include <bcrypt.h>

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

std::string hex(std::vector<UCHAR> const& bytes) {
    std::string result;
    char buffer[3]{};
    for (auto byte : bytes) {
        snprintf(buffer, sizeof(buffer), "%02x", byte);
        result += buffer;
    }
    return result;
}

class Sha256 {
public:
    Sha256() {
        BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
        BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0);
    }

    ~Sha256() {
        if (hash) BCryptDestroyHash(hash);
        if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    }

    Sha256(Sha256 const&) = delete;
    Sha256& operator=(Sha256 const&) = delete;

    void update(char const* data, DWORD size) {
        BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<char*>(data)), size, 0);
    }

    std::string digest() {
        std::vector<UCHAR> result(32);
        BCryptFinishHash(hash, result.data(), static_cast<ULONG>(result.size()), 0);
        return hex(result);
    }

private:
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
};

void setDownload(std::optional<Download> value) {
    std::lock_guard lock(downloadMutex);
    download = std::move(value);
}

void fetchArchive(fs::path const& archive) {
    URL_COMPONENTS url{};
    url.dwStructSize = sizeof(url);
    wchar_t host[256]{};
    wchar_t path[2048]{};
    url.lpszHostName = host;
    url.dwHostNameLength = static_cast<DWORD>(std::size(host));
    url.lpszUrlPath = path;
    url.dwUrlPathLength = static_cast<DWORD>(std::size(path));
    WinHttpCrackUrl(BUILD.url, 0, 0, &url);

    Internet session(WinHttpOpen((L"FileshipDesktop/" + appVersion() + L" (Windows)").c_str(),
                                 WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                                 WINHTTP_NO_PROXY_BYPASS, 0));
    Internet connection(session.value ? WinHttpConnect(session.value, host, url.nPort, 0) : nullptr);
    Internet request(connection.value ? WinHttpOpenRequest(connection.value, L"GET", path, nullptr,
                                                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                           WINHTTP_FLAG_SECURE)
                                      : nullptr);

    if (!request.value) throw std::runtime_error("Could not connect (" + std::to_string(GetLastError()) + ")");

    WinHttpSetTimeouts(request.value, 60000, 60000, 60000, 60000);

    if (!WinHttpSendRequest(request.value, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request.value, nullptr)) {
        throw std::runtime_error("Could not connect (" + std::to_string(GetLastError()) + ")");
    }

    DWORD status = 0;
    DWORD size = sizeof(status);
    WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
    if (status < 200 || status >= 300) {
        throw std::runtime_error(toUtf8(t(L"recording.downloadFailed", {{L"status", std::to_wstring(status)}})));
    }

    uint64_t total = 0;
    wchar_t lengthText[32]{};
    DWORD lengthSize = sizeof(lengthText);
    if (WinHttpQueryHeaders(request.value, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, lengthText,
                            &lengthSize, WINHTTP_NO_HEADER_INDEX)) {
        total = _wcstoui64(lengthText, nullptr, 10);
    }

    std::ofstream file(archive, std::ios::binary);
    if (!file) throw std::runtime_error("Could not write " + archive.string());

    Sha256 hash;
    std::vector<char> buffer(256 * 1024);
    uint64_t received = 0;
    auto lastPublish = std::chrono::steady_clock::now();

    for (;;) {
        DWORD read = 0;
        if (!WinHttpReadData(request.value, buffer.data(), static_cast<DWORD>(buffer.size()), &read)) {
            throw std::runtime_error("Download interrupted (" + std::to_string(GetLastError()) + ")");
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

    if (hash.digest() != BUILD.sha256) throw std::runtime_error(toUtf8(t(L"recording.downloadDamaged")));
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

std::wstring ffmpegFileName() {
    return L"ffmpeg.exe";
}

void describeFfmpeg(JsonObject& status) {
    status.SetNamedValue(L"managedVersion", JsonValue::CreateStringValue(BUILD.version));

    std::lock_guard lock(downloadMutex);
    if (download) {
        JsonObject progress;
        progress.SetNamedValue(L"progress", JsonValue::CreateNumberValue(download->progress));
        progress.SetNamedValue(L"error", download->error ? JsonValue::CreateStringValue(*download->error)
                                                           : JsonValue::CreateNullValue());
        status.SetNamedValue(L"download", progress);
    } else {
        status.SetNamedValue(L"download", JsonValue::CreateNullValue());
    }
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

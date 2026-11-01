#include "app.h"
#include "rpc.h"

#include <shared/hash.h>
#include <shared/log.h>

#include <fstream>
#include <mutex>
#include <sstream>

namespace fileship::host {

namespace {

constexpr wchar_t const* FEED = L"https://github.com/hanzydev/fileship-desktop/releases/latest/download";
constexpr int CHECK_INTERVAL_MS = 4 * 60 * 60 * 1000;
constexpr int RETRY_DELAY_MS = 15 * 60 * 1000;

struct Release {
    std::wstring version;
    std::wstring url;
    std::wstring sha512;
    int64_t size = 0;
};

struct SemVer {
    int major = 0;
    int minor = 0;
    int patch = 0;
    std::wstring prerelease;
};

std::mutex mutex;
JsonObject status{nullptr};
bool installWhenIdle = false;
uint64_t retryGeneration = 0;
std::filesystem::path installer;

void setStatus(JsonObject next) {
    bool isChanged = false;

    {
        std::lock_guard lock(mutex);
        isChanged = jsonString(next, L"state") != jsonString(status, L"state");
        status = next;
    }

    broadcast(L"updates:status", cloneObject(next));
    if (isChanged) refreshTray();
}

JsonObject makeStatus(std::wstring const& state, std::optional<std::wstring> version = std::nullopt,
                      std::optional<int> progress = std::nullopt, std::optional<std::wstring> error = std::nullopt) {
    JsonObject result;
    result.SetNamedValue(L"state", JsonValue::CreateStringValue(state));
    if (version) result.SetNamedValue(L"version", JsonValue::CreateStringValue(*version));
    if (progress) result.SetNamedValue(L"progress", JsonValue::CreateNumberValue(*progress));
    if (error) result.SetNamedValue(L"error", JsonValue::CreateStringValue(*error));
    return result;
}

std::wstring feed() {
    auto custom = launchOptions().updateFeed;
    auto base = custom.empty() ? std::wstring(FEED) : custom;
    while (!base.empty() && base.back() == L'/') base.pop_back();
    return base;
}

std::pair<std::wstring, std::wstring> splitUrl(std::wstring const& url) {
    auto scheme = url.find(L"://");
    auto slash = url.find(L'/', scheme == std::wstring::npos ? 0 : scheme + 3);
    if (slash == std::wstring::npos) return {url, L"/"};
    return {url.substr(0, slash), url.substr(slash)};
}

std::wstring resolveUrl(std::wstring const& value) {
    if (value.find(L"://") != std::wstring::npos) return value;
    return feed() + L"/" + value;
}

std::optional<SemVer> parseVersion(std::wstring text) {
    if (!text.empty() && (text[0] == L'v' || text[0] == L'V')) text.erase(0, 1);

    SemVer version;
    auto plus = text.find(L'+');
    if (plus != std::wstring::npos) text.resize(plus);

    auto hyphen = text.find(L'-');
    if (hyphen != std::wstring::npos) {
        version.prerelease = text.substr(hyphen + 1);
        text.resize(hyphen);
    }

    std::wistringstream parts(text);
    wchar_t first = 0;
    wchar_t second = 0;
    if (!(parts >> version.major >> first >> version.minor >> second >> version.patch) || first != L'.' ||
        second != L'.' || !parts.eof())
        return std::nullopt;
    return version;
}

bool isNewer(SemVer const& remote, SemVer const& local) {
    if (remote.major != local.major) return remote.major > local.major;
    if (remote.minor != local.minor) return remote.minor > local.minor;
    if (remote.patch != local.patch) return remote.patch > local.patch;
    if (remote.prerelease == local.prerelease) return false;
    if (remote.prerelease.empty()) return true;
    if (local.prerelease.empty()) return false;
    return remote.prerelease > local.prerelease;
}

std::wstring unquote(std::wstring value) {
    auto start = value.find_first_not_of(L" \t");
    if (start == std::wstring::npos) return {};
    value = value.substr(start, value.find_last_not_of(L" \t\r") - start + 1);

    if (value.size() >= 2 && (value.front() == L'\'' || value.front() == L'"') && value.back() == value.front())
        return value.substr(1, value.size() - 2);

    return value;
}

Release parseRelease(std::string const& text) {
    Release release;
    std::wistringstream lines(toWide(text));
    std::wstring line;
    Release fallback;
    bool inFiles = false;
    bool isChosen = false;

    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == L'\r') line.pop_back();

        auto first = line.find_first_not_of(L' ');
        if (first == std::wstring::npos || line[first] == L'#') continue;

        bool isIndented = first > 0 || line[first] == L'-';
        bool isEntry = line[first] == L'-';
        auto trimmed = line.substr(line.find_first_not_of(L" -"));
        auto colon = trimmed.find(L':');
        if (colon == std::wstring::npos) continue;

        auto key = trimmed.substr(0, colon);
        auto value = unquote(trimmed.substr(colon + 1));

        if (!isIndented) {
            inFiles = key == L"files";
            if (key == L"version") release.version = value;
            if (key == L"path") fallback.url = value;
            if (key == L"sha512") fallback.sha512 = value;
            continue;
        }

        if (!inFiles) continue;
        if (isEntry) isChosen = release.url.empty() && key == L"url" && isUpdateFile(value);
        if (!isChosen) continue;

        if (key == L"url") release.url = value;
        if (key == L"sha512") release.sha512 = value;
        if (key == L"size") release.size = std::wcstoll(value.c_str(), nullptr, 10);
    }

    if (release.url.empty() && isUpdateFile(fallback.url)) {
        release.url = fallback.url;
        release.sha512 = fallback.sha512;
    }

    return release;
}

std::wstring describeError(std::exception const& error) {
    if (auto api = dynamic_cast<ApiError const*>(&error)) {
        if (api->status == 404) return t(L"updater.noReleases");
        if (api->status == 0) return t(L"updater.unreachable");
    }

    auto message = errorMessage(error);
    message = message.substr(0, message.find(L'\n')).substr(0, 200);
    return message.empty() ? t(L"updater.checkFailed") : message;
}

std::filesystem::path downloadFolder() {
    return isPackaged() ? updatesFolder() : userDataPath() / L"updates";
}

std::wstring hashFile(std::filesystem::path const& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};

    Hasher hasher(HashKind::Sha512);
    std::vector<char> buffer(1024 * 1024);
    while (file) {
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        hasher.update(buffer.data(), static_cast<size_t>(file.gcount()));
    }

    auto digest = hasher.finish();
    return base64(digest.data(), digest.size());
}

HttpEndpoint endpointFor(std::wstring const& base) {
    return HttpEndpoint{base, {}, desktopUserAgent(), {}};
}

std::string fetchText(std::wstring const& url) {
    auto [base, path] = splitUrl(url);

    HttpResponse response;
    try {
        response = httpSend(endpointFor(base), HttpRequest{L"GET", path, {}, {}, 30000});
    } catch (std::exception const& error) {
        throw ApiError(error.what(), 0, JsonObject());
    }

    if (response.status < 200 || response.status >= 300)
        throw ApiError(std::to_string(response.status) + " " + toUtf8(url), response.status, JsonObject());

    return response.text();
}

void download(Release const& release, std::filesystem::path const& target) {
    auto [base, path] = splitUrl(resolveUrl(release.url));
    auto partial = std::filesystem::path(target).concat(L".download");

    std::optional<HttpStream> stream;
    try {
        stream.emplace(endpointFor(base), HttpRequest{L"GET", path, {}, {}, 60000});
    } catch (std::exception const& error) {
        throw ApiError(error.what(), 0, JsonObject());
    }

    if (stream->status < 200 || stream->status >= 300)
        throw ApiError(std::to_string(stream->status), stream->status, JsonObject());

    std::ofstream file(partial, std::ios::binary | std::ios::trunc);
    if (!file) throw std::runtime_error("Could not write the update");

    Hasher hasher(HashKind::Sha512);
    std::vector<char> buffer(256 * 1024);
    int64_t received = 0;
    int reported = -1;

    while (size_t read = stream->read(buffer.data(), buffer.size())) {
        file.write(buffer.data(), read);
        hasher.update(buffer.data(), read);
        received += read;

        if (release.size > 0) {
            int progress = static_cast<int>(received * 100 / release.size);
            if (progress != reported) {
                reported = progress;
                setStatus(makeStatus(L"downloading", release.version, progress));
            }
        }
    }

    file.close();

    auto digest = hasher.finish();
    if (base64(digest.data(), digest.size()) != release.sha512) {
        std::error_code error;
        std::filesystem::remove(partial, error);
        throw std::runtime_error("sha512 checksum mismatch");
    }

    std::error_code error;
    std::filesystem::remove(target, error);
    std::filesystem::rename(partial, target);
}

void scheduleRetry() {
    uint64_t generation = 0;

    {
        std::lock_guard lock(mutex);
        generation = ++retryGeneration;
    }

    runAfter(RETRY_DELAY_MS, [generation] {
        {
            std::lock_guard lock(mutex);
            if (generation != retryGeneration) return;
        }

        checkForUpdates();
    });
}

void runInstaller(bool forceRun) {
    std::filesystem::path path;

    {
        std::lock_guard lock(mutex);
        path = installer;
    }

    if (path.empty()) return;
    startInstaller(path, forceRun);
}

void install() {
    logInfo(L"Installing the update");
    runInstaller(true);

    {
        std::lock_guard lock(mutex);
        installer.clear();
    }

    postToMainThread(quitApp);
}

bool isEnabled() {
    return (isPackaged() && canUpdateItself()) || !launchOptions().updateFeed.empty();
}

void scheduleChecks() {
    runAfter(CHECK_INTERVAL_MS, [] {
        checkForUpdates();
        scheduleChecks();
    });
}

}

JsonObject updateStatus() {
    std::lock_guard lock(mutex);
    return status ? cloneObject(status) : makeStatus(L"idle");
}

void checkForUpdates() {
    if (!isEnabled()) {
        setStatus(makeStatus(L"unsupported"));
        return;
    }

    auto checking = makeStatus(L"checking");

    {
        std::lock_guard lock(mutex);
        auto state = jsonString(status, L"state");
        if (state == L"checking" || state == L"downloading" || state == L"ready") return;

        status = checking;
        retryGeneration++;
    }

    broadcast(L"updates:status", cloneObject(checking));
    refreshTray();

    try {
        auto release = parseRelease(fetchText(feed() + L"/" + updateManifestName()));
        auto remote = parseVersion(release.version);
        auto local = parseVersion(appVersion());
        if (!remote || release.url.empty() || release.sha512.empty()) throw std::runtime_error("Invalid latest.yml");

        if (!local || !isNewer(*remote, *local)) {
            setStatus(makeStatus(L"latest"));
            return;
        }

        setStatus(makeStatus(L"downloading", release.version, 0));

        auto folder = downloadFolder();
        std::filesystem::create_directories(folder);
        auto target = folder / std::filesystem::path(release.url).filename();

        if (!std::filesystem::exists(target) || hashFile(target) != release.sha512) download(release, target);

        {
            std::lock_guard lock(mutex);
            installer = target;
        }

        setStatus(makeStatus(L"ready", release.version));
        notify(t(L"updater.readyTitle", {{L"version", release.version}}), t(L"updater.readyBody"),
               [] { installUpdate(); });
    } catch (std::exception const& error) {
        logWarn(L"Update check failed: " + errorMessage(error));
        auto previous = updateStatus();
        auto version = jsonString(previous, L"version");
        setStatus(makeStatus(L"error", version.empty() ? std::nullopt : std::optional(version), std::nullopt,
                             describeError(error)));
        scheduleRetry();
    }
}

void installUpdate() {
    if (jsonString(updateStatus(), L"state") != L"ready") return;

    if (isBusy()) {
        std::lock_guard lock(mutex);
        installWhenIdle = true;
        return;
    }

    runAsync(install);
}

void notifyIdle() {
    bool shouldInstall = false;

    {
        std::lock_guard lock(mutex);
        shouldInstall = installWhenIdle;
    }

    if (shouldInstall && !isBusy()) {
        {
            std::lock_guard lock(mutex);
            installWhenIdle = false;
        }

        runAsync(install);
    }
}

void installOnQuit() {
    if (jsonString(updateStatus(), L"state") == L"ready") runInstaller(false);
}

void startUpdater() {
    if (!isEnabled()) {
        setStatus(makeStatus(L"unsupported"));
        return;
    }

    runAfter(10000, checkForUpdates);
    scheduleChecks();
}

}

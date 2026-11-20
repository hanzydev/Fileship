#include "updater.h"

#include "updateFeed.h"

#include <host/app.h>
#include <host/rpc.h>

#include <shared/log.h>

#include <fstream>
#include <mutex>
#include <sstream>

namespace fileship::host {

namespace {

constexpr wchar_t const* FEED = L"https://github.com/hanzydev/fileship-desktop/releases/latest/download";
constexpr int CHECK_INTERVAL_MS = 4 * 60 * 60 * 1000;
constexpr int RETRY_DELAY_MS = 15 * 60 * 1000;

std::mutex mutex;
JsonObject status{nullptr};
bool installWhenIdle = false;
bool isInstalling = false;
uint64_t retryGeneration = 0;
std::filesystem::path installer;
std::wstring installerHash;

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

std::wstring describeError(std::exception const& error) {
    if (auto api = dynamic_cast<ApiError const*>(&error)) {
        if (api->status == 404) return t(L"updater.noReleases");
        if (api->status == 0) return t(L"updater.unreachable");
    }

    auto message = errorMessage(error);
    message = message.substr(0, message.find(L'\n')).substr(0, 200);
    return message.empty() ? t(L"updater.checkFailed") : message;
}

std::wstring hashFile(std::filesystem::path const& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};

    auto hasher = Sha512::create();
    std::vector<char> buffer(1024 * 1024);
    while (file) {
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        hasher->update(buffer.data(), static_cast<size_t>(file.gcount()));
    }

    return hasher->finish();
}

HttpEndpoint endpointFor(std::wstring const& base) {
    return HttpEndpoint{base, {}, desktopUserAgent(), {}};
}

std::string fetchText(std::wstring const& url) {
    auto [base, path] = splitUrl(url);

    HttpResponse response;
    try {
        response = httpSend(endpointFor(base), HttpRequest{L"GET", path, {}, {}, 30000, {}});
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
        stream.emplace(endpointFor(base), HttpRequest{L"GET", path, {}, {}, 60000, {}});
    } catch (std::exception const& error) {
        throw ApiError(error.what(), 0, JsonObject());
    }

    if (stream->status < 200 || stream->status >= 300)
        throw ApiError(std::to_string(stream->status), stream->status, JsonObject());

    std::ofstream file(partial, std::ios::binary | std::ios::trunc);
    if (!file) throw std::runtime_error("Could not write the update");

    auto hasher = Sha512::create();
    std::vector<char> buffer(256 * 1024);
    int64_t received = 0;
    int reported = -1;

    while (size_t read = stream->read(buffer.data(), buffer.size())) {
        file.write(buffer.data(), read);
        hasher->update(buffer.data(), read);
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

    if (hasher->finish() != release.sha512) {
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

void install() {
    std::filesystem::path file;
    std::wstring hash;

    {
        std::lock_guard lock(mutex);
        if (isInstalling) return;

        isInstalling = true;
        file = installer;
        hash = installerHash;
    }

    logInfo(L"Installing the update");

    auto result = file.empty()              ? InstallResult::Started
                  : hashFile(file) != hash ? InstallResult::Failed
                                           : runUpdateInstaller(file, true);

    {
        std::lock_guard lock(mutex);
        isInstalling = false;
    }

    if (result == InstallResult::Refused) return;

    if (result == InstallResult::Failed) {
        setStatus(makeStatus(L"error", jsonString(updateStatus(), L"version"), std::nullopt, t(L"updater.installFailed")));
        scheduleRetry();
        return;
    }

    {
        std::lock_guard lock(mutex);
        installer.clear();
    }

    postToMainThread(quitApp);
}

bool isEnabled() {
    return isPackaged() || !launchOptions().updateFeed.empty();
}

void scheduleChecks() {
    runAfter(CHECK_INTERVAL_MS, [] {
        checkForUpdates();
        scheduleChecks();
    });
}

}

std::filesystem::path pendingUpdate() {
    std::lock_guard lock(mutex);
    return installer;
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
        auto release = parseRelease(fetchText(feed() + L"/" + updateFeedFile()), updatePackageSuffix());
        auto remote = parseVersion(release.version);
        auto local = parseVersion(appVersion());
        if (!remote || release.url.empty() || release.sha512.empty()) throw std::runtime_error("Invalid update feed");

        if (!local || !isNewer(*remote, *local)) {
            setStatus(makeStatus(L"latest"));
            return;
        }

        setStatus(makeStatus(L"downloading", release.version, 0));

        auto folder = updateDownloadFolder();
        std::filesystem::create_directories(folder);
        auto target = folder / std::filesystem::path(release.url).filename();

        if (!std::filesystem::exists(target) || hashFile(target) != release.sha512) download(release, target);

        {
            std::lock_guard lock(mutex);
            installer = target;
            installerHash = release.sha512;
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

void startUpdater() {
    if (!isEnabled()) {
        setStatus(makeStatus(L"unsupported"));
        return;
    }

    runAfter(10000, checkForUpdates);
    scheduleChecks();
}

}

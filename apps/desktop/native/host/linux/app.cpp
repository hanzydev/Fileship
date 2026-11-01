#include "linux.h"

#include <core/linux/linux.h>
#include <host/app.h>
#include <host/harbor/helper.h>
#include <shared/log.h>
#include <ui/linux/gtk.h>

#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>

namespace fileship::host {

namespace {

GThreadPool* workers() {
    static GThreadPool* pool = g_thread_pool_new(
        [](gpointer data, gpointer) {
            std::unique_ptr<std::function<void()>> work(static_cast<std::function<void()>*>(data));

            try {
                (*work)();
            } catch (std::exception const& error) {
                logError(L"Background task failed: " + errorMessage(error));
            } catch (...) {
                logError(L"Background task failed: " + describeException(std::current_exception()));
            }
        },
        nullptr, -1, FALSE, nullptr);
    return pool;
}

}

GMainLoop* mainLoop() {
    static GMainLoop* loop = g_main_loop_new(nullptr, FALSE);
    return loop;
}

GDBusConnection* sessionBus() {
    static GDBusConnection* connection = [] {
        auto bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
        if (!bus || isFlatpak()) return bus;

        GVariantBuilder options;
        g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);
        auto reply = g_dbus_connection_call_sync(bus, "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
                                                 "org.freedesktop.host.portal.Registry", "Register",
                                                 g_variant_new("(sa{sv})", "dev.hanzy.fileship", &options), nullptr,
                                                 G_DBUS_CALL_FLAGS_NONE, 2000, nullptr, nullptr);
        if (reply) g_variant_unref(reply);
        return bus;
    }();
    return connection;
}

bool ensureGtk() {
    return ui::initGtk();
}

bool isFlatpak() {
    static bool const flatpak = std::filesystem::exists("/.flatpak-info");
    return flatpak;
}

std::filesystem::path xdgFolder(char const* variable, char const* fallback) {
    if (auto value = std::getenv(variable); value && *value == '/') return value;
    return std::filesystem::path(g_get_home_dir()) / fallback;
}

std::filesystem::path runtimeFolder() {
    auto value = std::getenv("XDG_RUNTIME_DIR");
    auto id = std::getenv("FLATPAK_ID");
    if (value && *value == '/' && id && *id && isFlatpak()) return std::filesystem::path(value) / "app" / id;
    if (value && *value == '/') return std::filesystem::path(value);
    return std::filesystem::temp_directory_path() / ("fileship-" + std::to_string(getuid()));
}

std::filesystem::path iconPath() {
    return resourcesPath() / "icon.png";
}

std::string shellQuote(std::string const& argument) {
    std::string result = "'";
    for (auto character : argument) {
        if (character == '\'') result += "'\\''";
        else result += character;
    }
    return result + "'";
}

std::filesystem::path executablePath() {
    std::error_code error;
    return std::filesystem::read_symlink("/proc/self/exe", error);
}

std::filesystem::path defaultUserDataPath() {
    return xdgFolder("XDG_CONFIG_HOME", ".config") / "Fileship";
}

std::filesystem::path picturesFolder() {
    auto pictures = g_get_user_special_dir(G_USER_DIRECTORY_PICTURES);
    return pictures ? std::filesystem::path(pictures) : std::filesystem::path(g_get_home_dir()) / "Pictures";
}

std::filesystem::path updatesFolder() {
    return xdgFolder("XDG_CACHE_HOME", ".cache") / "fileship-updater" / "pending";
}

std::filesystem::path localDataFolder() {
    return xdgFolder("XDG_DATA_HOME", ".local/share");
}

std::filesystem::path homeFolder() {
    return g_get_home_dir();
}

std::wstring executableName(std::wstring const& base) {
    return base;
}

std::wstring platformName() {
    return L"Linux";
}

std::optional<std::filesystem::path> appImagePath() {
    if (auto image = std::getenv("APPIMAGE"); image && *image) return std::filesystem::path(image);

    std::ifstream file(resourcesPath().parent_path().parent_path() / "appimage");
    std::string path;
    std::error_code error;
    if (std::getline(file, path) && !path.empty() && std::filesystem::exists(path, error)) return path;
    return std::nullopt;
}

void runOutsideAppImage(char** argv) {
    auto folder = std::getenv("APPDIR");
    auto image = std::getenv("APPIMAGE");
    if (!folder || !*folder || !image || !*image) return;

    auto executable = executablePath();
    if (!executable.string().starts_with(std::string(folder) + "/")) return;

    auto runtime = localDataFolder() / "Fileship" / "runtime";
    auto version = toUtf8(appVersion());
    auto target = runtime / version;
    auto host = target / "resources" / "host" / "fileship-host";
    std::error_code error;

    if (!std::filesystem::exists(host, error)) {
        auto staging = runtime / (version + ".tmp");
        auto resources = executable.parent_path().parent_path();
        std::filesystem::remove_all(staging, error);
        std::filesystem::create_directories(staging / "resources", error);

        for (auto name : {"host", "harbor"}) {
            if (!error) std::filesystem::copy(resources / name, staging / "resources" / name,
                                              std::filesystem::copy_options::recursive, error);
        }

        if (!error) {
            std::ofstream launcher(staging / "fileship");
            launcher << "#!/bin/sh\nexec \"$(cat \"$(dirname \"$0\")/appimage\")\" \"$@\"\n";
        }
        chmod((staging / "fileship").c_str(), 0755);
        if (!error) std::filesystem::rename(staging, target, error);
        if (error) return logWarn(L"Could not copy Fileship out of the AppImage: " + toWide(error.message()));
    }

    std::ofstream(target / "appimage", std::ios::trunc) << image;

    for (auto const& entry : std::filesystem::directory_iterator(runtime, error)) {
        std::error_code ignored;
        if (entry.path() != target) std::filesystem::remove_all(entry.path(), ignored);
    }

    unsetenv("APPDIR");
    execv(host.c_str(), argv);
}

std::wstring launchCommand() {
    if (auto id = std::getenv("FLATPAK_ID"); id && *id && isFlatpak()) return L"flatpak run " + toWide(id);
    if (auto image = appImagePath()) return toWide(shellQuote(image->string()));
    if (isPackaged()) return toWide(shellQuote((resourcesPath().parent_path().parent_path() / "fileship").string()));
    return toWide(shellQuote(hostExecutable().string()));
}

JsonObject platformCapabilities() {
    bool isWayland = core::isWaylandSession();

    JsonObject voiceProcessing;
    for (auto key : {L"noiseSuppression", L"echoCancellation", L"autoGainControl"})
        voiceProcessing.SetNamedValue(key, JsonValue::CreateBooleanValue(false));

    JsonArray videoSources;
    for (auto source : {L"auto", isWayland ? L"pipewire" : L"x11grab"}) {
        videoSources.Append(JsonValue::CreateStringValue(source));
    }

    std::error_code error;
    bool hasHarbor = !isFlatpak() && std::filesystem::exists("/dev/fuse", error) && harbor::isHelperAvailable();
    auto updates = !isPackaged() ? L"none" : canUpdateItself() ? L"auto" : L"package";

    JsonObject capabilities;
    capabilities.SetNamedValue(L"windowDetection", JsonValue::CreateBooleanValue(!isWayland));
    capabilities.SetNamedValue(L"controlDetection", JsonValue::CreateBooleanValue(false));
    capabilities.SetNamedValue(L"snippingToolKey", JsonValue::CreateBooleanValue(false));
    capabilities.SetNamedValue(L"harbor", JsonValue::CreateBooleanValue(hasHarbor));
    capabilities.SetNamedValue(L"recordingBorder", JsonValue::CreateBooleanValue(!isWayland));
    capabilities.SetNamedValue(L"hotkeys", JsonValue::CreateStringValue(hotkeyBackend()));
    capabilities.SetNamedValue(L"voiceProcessing", voiceProcessing);
    capabilities.SetNamedValue(L"videoSources", videoSources);
    capabilities.SetNamedValue(L"updates", JsonValue::CreateStringValue(updates));
    return capabilities;
}

bool hasPackagedLayout() {
    auto resources = resourcesPath().parent_path();
    return resources.filename() == "resources" && std::filesystem::exists(resources.parent_path() / "fileship");
}

std::wstring defaultPipeName(std::wstring const& suffix) {
    return (runtimeFolder() / "fileship" / (L"host" + suffix + L".sock")).wstring();
}

void wakeMainThread() {
    g_idle_add(
        [](gpointer) {
            runMainQueue();
            return G_SOURCE_REMOVE;
        },
        nullptr);
}

void stopMainLoop() {
    g_main_loop_quit(mainLoop());
}

void runAsync(std::function<void()> work) {
    g_thread_pool_push(workers(), new std::function<void()>(std::move(work)), nullptr);
}

void runAfter(int delayMs, std::function<void()> work) {
    g_timeout_add_full(
        G_PRIORITY_DEFAULT, static_cast<guint>(std::max(delayMs, 0)),
        [](gpointer data) {
            runAsync(std::move(*static_cast<std::function<void()>*>(data)));
            return G_SOURCE_REMOVE;
        },
        new std::function<void()>(std::move(work)), [](gpointer data) { delete static_cast<std::function<void()>*>(data); });
}

bool replaceFile(std::filesystem::path const& from, std::filesystem::path const& to) {
    chmod(from.c_str(), 0600);

    std::error_code error;
    std::filesystem::rename(from, to, error);
    return !error;
}

bool systemUsesLightTheme() {
    auto reply = g_dbus_connection_call_sync(
        sessionBus(), "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.Settings", "ReadOne", g_variant_new("(ss)", "org.freedesktop.appearance", "color-scheme"),
        G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, 1000, nullptr, nullptr);
    if (!reply) return true;

    GVariant* value = nullptr;
    g_variant_get(reply, "(v)", &value);
    bool isDark = g_variant_is_of_type(value, G_VARIANT_TYPE_UINT32) && g_variant_get_uint32(value) == 1;
    g_variant_unref(value);
    g_variant_unref(reply);
    return !isDark;
}

bool textLess(std::wstring const& a, std::wstring const& b) {
    auto first = g_utf8_casefold(toUtf8(a).c_str(), -1);
    auto second = g_utf8_casefold(toUtf8(b).c_str(), -1);
    bool isLess = g_utf8_collate(first, second) < 0;
    g_free(first);
    g_free(second);
    return isLess;
}

void openWebUrl(std::wstring const& url) {
    auto lower = url.substr(0, 8);
    for (auto& character : lower) character = static_cast<wchar_t>(towlower(character));
    if (lower.rfind(L"http://", 0) != 0 && lower.rfind(L"https://", 0) != 0) return;

    postToMainThread([uri = toUtf8(url)] { g_app_info_launch_default_for_uri(uri.c_str(), nullptr, nullptr); });
}

void openFolder(std::filesystem::path const& path) {
    postToMainThread([path] {
        auto uri = g_filename_to_uri(path.c_str(), nullptr, nullptr);
        if (!uri) return;
        g_app_info_launch_default_for_uri(uri, nullptr, nullptr);
        g_free(uri);
    });
}

void showItemInFolder(std::filesystem::path const& path) {
    postToMainThread([path] {
        auto uri = g_filename_to_uri(path.c_str(), nullptr, nullptr);
        if (!uri) return;

        GVariantBuilder items;
        g_variant_builder_init(&items, G_VARIANT_TYPE("as"));
        g_variant_builder_add(&items, "s", uri);

        auto reply = g_dbus_connection_call_sync(sessionBus(), "org.freedesktop.FileManager1",
                                                 "/org/freedesktop/FileManager1", "org.freedesktop.FileManager1",
                                                 "ShowItems", g_variant_new("(ass)", &items, ""), nullptr,
                                                 G_DBUS_CALL_FLAGS_NONE, 2000, nullptr, nullptr);

        if (reply) {
            g_variant_unref(reply);
        } else if (auto folder = g_filename_to_uri(path.parent_path().c_str(), nullptr, nullptr)) {
            g_app_info_launch_default_for_uri(folder, nullptr, nullptr);
            g_free(folder);
        }

        g_free(uri);
    });
}

}

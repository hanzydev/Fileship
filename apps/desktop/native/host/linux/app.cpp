#include "host.h"

#include <host/platform.h>
#include <shared/log.h>
#include <shared/platform.h>

#include <ctime>

namespace fileship::host {

namespace {

using Work = std::function<void()>;

void runWork(gpointer data, gpointer) {
    std::unique_ptr<Work> work(static_cast<Work*>(data));

    try {
        (*work)();
    } catch (...) {
        logError(L"Background task failed: " + exceptionMessage());
    }
}

GThreadPool* workers() {
    static GThreadPool* const pool = g_thread_pool_new(runWork, nullptr, -1, FALSE, nullptr);
    return pool;
}

gboolean runQueue(gpointer) {
    runMainQueue();
    return G_SOURCE_REMOVE;
}

gboolean runTimer(gpointer data) {
    g_thread_pool_push(workers(), data, nullptr);
    return G_SOURCE_REMOVE;
}

std::filesystem::path folder(char const* path) {
    return std::filesystem::path(path);
}

}

std::filesystem::path executablePath() {
    return std::filesystem::read_symlink("/proc/self/exe");
}

std::wstring pipeName() {
    if (!launchOptions().pipe.empty()) return launchOptions().pipe;
    return (folder(g_get_user_runtime_dir()) / (L"fileship-host" + profileSuffix() + L".sock")).wstring();
}

std::filesystem::path defaultUserDataPath() {
    return folder(g_get_user_config_dir()) / "Fileship";
}

std::filesystem::path picturesPath() {
    auto pictures = g_get_user_special_dir(G_USER_DIRECTORY_PICTURES);
    return pictures ? folder(pictures) : folder(g_get_home_dir()) / "Pictures";
}

std::wstring platformName() {
    return L"Linux";
}

std::wstring electronExecutable() {
    return L"fileship";
}

LocalTime localTime() {
    timespec now{};
    clock_gettime(CLOCK_REALTIME, &now);

    tm local{};
    localtime_r(&now.tv_sec, &local);

    return {local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,  local.tm_hour,
            local.tm_min,         local.tm_sec,     static_cast<int>(now.tv_nsec / 1000000)};
}

std::wstring appVersion() {
    return toWide(FILESHIP_VERSION);
}

void wakeMainThread() {
    g_idle_add(runQueue, nullptr);
}

void runAsync(std::function<void()> work) {
    g_thread_pool_push(workers(), new Work(std::move(work)), nullptr);
}

void runAfter(int delayMs, std::function<void()> work) {
    g_timeout_add(static_cast<guint>(std::max(delayMs, 0)), runTimer, new Work(std::move(work)));
}

bool replaceFile(std::filesystem::path const& from, std::filesystem::path const& to) {
    std::error_code error;
    std::filesystem::rename(from, to, error);
    return !error;
}

void openExternal(std::wstring const& url) {
    GError* error = nullptr;

    if (!g_app_info_launch_default_for_uri(toUtf8(url).c_str(), nullptr, &error)) {
        logWarn(L"Could not open " + url + L": " + toWide(error->message));
        g_error_free(error);
    }
}

GDBusConnection* sessionBus() {
    static GDBusConnection* const bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
    return bus;
}

void showItemInFolder(std::filesystem::path const& path) {
    gchar* uri = g_filename_to_uri(path.c_str(), nullptr, nullptr);
    GVariantBuilder items;
    g_variant_builder_init(&items, G_VARIANT_TYPE("as"));
    g_variant_builder_add(&items, "s", uri);

    g_dbus_connection_call(sessionBus(), "org.freedesktop.FileManager1", "/org/freedesktop/FileManager1",
                           "org.freedesktop.FileManager1", "ShowItems", g_variant_new("(ass)", &items, ""), nullptr,
                           G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr, nullptr);

    g_free(uri);
}

bool isSystemDark() {
    GVariant* reply = g_dbus_connection_call_sync(
        sessionBus(), "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Settings",
        "ReadOne", g_variant_new("(ss)", "org.freedesktop.appearance", "color-scheme"), G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE, 1000, nullptr, nullptr);
    if (!reply) return false;

    GVariant* value = nullptr;
    g_variant_get(reply, "(v)", &value);
    bool isDark = g_variant_is_of_type(value, G_VARIANT_TYPE_UINT32) && g_variant_get_uint32(value) == 1;

    g_variant_unref(value);
    g_variant_unref(reply);
    return isDark;
}

bool isNameBefore(std::wstring const& a, std::wstring const& b) {
    gchar* first = g_utf8_casefold(toUtf8(a).c_str(), -1);
    gchar* second = g_utf8_casefold(toUtf8(b).c_str(), -1);
    bool isBefore = g_utf8_collate(first, second) < 0;

    g_free(first);
    g_free(second);
    return isBefore;
}

}

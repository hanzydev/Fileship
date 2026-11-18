#include "host.h"
#include "portal.h"

#include <host/harbor.h>
#include <host/recording.h>
#include <shared/i18n.h>
#include <shared/log.h>

#include <glib-unix.h>
#include <glib.h>

#include <fcntl.h>
#include <sys/file.h>

#include <clocale>

namespace fileship::host {

namespace {

GMainLoop* loop = nullptr;
bool isQuitting = false;

bool acquireInstanceLock() {
    auto path = std::filesystem::path(toUtf8(pipeName())).replace_extension(".lock");
    int file = open(path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
    return file >= 0 && flock(file, LOCK_EX | LOCK_NB) == 0;
}

gboolean handleSignal(gpointer) {
    quitApp();
    return G_SOURCE_REMOVE;
}

}

bool isHostQuitting() {
    return isQuitting;
}

void quitApp() {
    if (isQuitting) return;
    isQuitting = true;

    logInfo(L"Quitting");
    broadcast(L"app:quit", JsonValue::CreateNullValue());
    unregisterHotkeys();
    removeTray();

    runAsync([] {
        recording::quitRecorder();
        harbor::stopHarbor();
        postToMainThread([] { g_main_loop_quit(loop); });
    });
}

}

int main(int count, char** values) {
    using namespace fileship;
    using namespace fileship::host;

    setlocale(LC_COLLATE, "");

    std::vector<std::wstring> arguments;
    for (int index = 1; index < count; index++) arguments.push_back(toWide(values[index]));
    parseOptions(arguments);

    if (!acquireInstanceLock()) {
        auto const& forwarded = launchOptions().arguments;
        bool isOnlyHidden = forwarded.size() == 1 && forwarded[0] == L"--hidden";
        if (!isOnlyHidden) forwardToRunningHost(forwarded);

        return 0;
    }

    initLog(userDataPath() / L"logs" / L"host.log");
    initI18n(resourcesPath() / L"i18n");
    logInfo(L"Fileship host " + appVersion() + (isPackaged() ? L"" : L" (development)"));

    if (!sessionBus()) {
        logError(L"There is no D-Bus session bus");
        return 1;
    }

    registerPortalApp();

    loadStore();
    setLanguage(resolveLanguage(jsonString(settings(), L"language", L"system")));

    registerHandlers();
    harbor::registerHarborHandlers();
    recording::registerRecordingHandlers();
    startRpcServer();

    applySettings();
    initNotifications();
    createTray();
    registerHotkeys();
    startUpdater();
    runAsync([] { harbor::startHarbor(); });

    if (!handleLaunch(launchOptions().arguments, std::filesystem::current_path()) && !launchOptions().hidden) {
        showMainWindow();
    }

    loop = g_main_loop_new(nullptr, FALSE);
    g_unix_signal_add(SIGTERM, handleSignal, nullptr);
    g_unix_signal_add(SIGINT, handleSignal, nullptr);
    g_main_loop_run(loop);

    stopRpcServer();
    logInfo(L"Stopped");
    return 0;
}

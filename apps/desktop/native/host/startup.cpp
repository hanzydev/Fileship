#include "app.h"
#include "rpc.h"

#include <host/harbor.h>
#include <host/recording.h>
#include <shared/i18n.h>
#include <shared/log.h>

#include <algorithm>
#include <atomic>

namespace fileship::host {

namespace {

std::atomic<bool> isQuitting = false;

void setOverlayStore() {
    ui::setOverlayPreferencesStore({
        [] {
            auto value = storeValue(L"overlay");
            return value.ValueType() == JsonValueType::Null ? std::string() : stringify(value);
        },
        [](std::string const& text) {
            JsonValue value{nullptr};
            if (JsonValue::TryParse(toWide(text), value)) setStoreValue(L"overlay", value);
        },
    });
}

}

void parseOptions(std::vector<std::wstring> const& arguments) {
    auto& options = launchOptions();
    std::vector<std::wstring> rest;

    for (size_t index = 0; index < arguments.size(); index++) {
        auto const& argument = arguments[index];
        bool hasValue = index + 1 < arguments.size();

        if (argument == L"--hidden") options.hidden = true;
        else if (argument == L"--dev") options.dev = true;
        else if (argument == L"--user-data-dir" && hasValue) options.userData = arguments[++index];
        else if (argument.starts_with(L"--user-data-dir=")) options.userData = argument.substr(16);
        else if (argument == L"--renderer-url" && hasValue) options.rendererUrl = arguments[++index];
        else if (argument == L"--electron" && hasValue) options.electronCommand = arguments[++index];
        else if (argument == L"--update-feed" && hasValue) options.updateFeed = arguments[++index];
        else if (argument == L"--pipe" && hasValue) options.pipe = arguments[++index];
        else rest.push_back(argument);
    }

    if (options.hidden) rest.push_back(L"--hidden");
    options.arguments = rest;
}

bool shouldForwardLaunch() {
    auto const& arguments = launchOptions().arguments;
    return std::find(arguments.begin(), arguments.end(), L"--hidden") == arguments.end() || arguments.size() > 1;
}

void initHost() {
    initLog(userDataPath() / L"logs" / L"host.log");
    initI18n(resourcesPath() / L"i18n");
    logInfo(L"Fileship host " + appVersion() + (isPackaged() ? L"" : L" (development)"));

    loadStore();
    setLanguage(resolveLanguage(jsonString(settings(), L"language", L"system")));
}

void startHost() {
    initNotifications();
    setOverlayStore();

    registerHandlers();
    harbor::registerHarborHandlers();
    recording::registerRecordingHandlers();
    recording::setAreaPicker([](ui::OverlayPurpose purpose, recording::AreaCallback done) {
        pickArea(purpose, [done](core::Rect area) { done({area.x, area.y, area.width, area.height}); });
        return true;
    });
    startRpcServer();

    applySettings();
    registerShellMenu();
    createTray();
    registerHotkeys();
    startUpdater();
    runAsync([] { harbor::startHarbor(); });

    auto const& arguments = launchOptions().arguments;
    if (!handleLaunch(arguments, std::filesystem::current_path()) && !launchOptions().hidden) showMainWindow();
}

void quitApp() {
    if (isQuitting.exchange(true)) return;

    logInfo(L"Quitting");
    broadcast(L"app:quit", JsonValue::CreateNullValue());
    unregisterHotkeys();
    removeTray();

    runAsync([] {
        recording::quitRecorder();
        harbor::stopHarbor();
        installOnQuit();
        postToMainThread(stopMainLoop);
    });
}

}

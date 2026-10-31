#include "tray.h"

#include "app.h"

#include <host/harbor.h>
#include <host/recording.h>
#include <shared/log.h>

namespace fileship::host {

namespace {

MenuItem separator() {
    MenuItem item;
    item.separator = true;
    return item;
}

MenuItem action(std::wstring label, std::function<void()> click, bool enabled = true) {
    return MenuItem{std::move(label), std::move(click), enabled, false, {}};
}

MenuItem submenu(std::wstring label, std::vector<MenuItem> items, bool enabled = true) {
    return MenuItem{std::move(label), {}, enabled, false, std::move(items)};
}

void openHarbor() {
    runAsync([] {
        try {
            invokeHandler(L"harbor:open", JsonArray());
        } catch (std::exception const& error) {
            logWarn(L"Could not open Fileship Harbor: " + errorMessage(error));
        }
    });
}

MenuItem harborMenu(bool connected) {
    auto status = harbor::harborStatus();
    auto state = jsonString(status, L"state", L"off");

    if (!connected || !jsonBool(jsonObject(settings(), L"harbor"), L"enabled", true) || state == L"off")
        return action(t(L"tray.openHarbor"), {}, false);

    std::vector<MenuItem> items{action(t(L"harborSettings.states." + state), {}, false)};

    auto storage = jsonObject(status, L"storage");
    if (storage.HasKey(L"used")) {
        auto used = formatStorageSize(static_cast<int64_t>(jsonNumber(storage, L"used")));
        auto limit = storage.GetNamedValue(L"limit");
        items.push_back(action(
            limit.ValueType() == JsonValueType::Number
                ? t(L"harbor.quotaUsed", {{L"used", used}, {L"total", formatStorageSize(static_cast<int64_t>(limit.GetNumber()))}})
                : t(L"harbor.quotaUnlimited", {{L"used", used}}),
            {}, false));
    }

    items.push_back(separator());
    items.push_back(action(t(L"tray.openHarbor"), openHarbor));

    if (state == L"paused") {
        items.push_back(action(t(L"harbor.resume"), [] { runAsync([] { harbor::pauseHarbor(false); }); }));
    } else {
        items.push_back(action(t(L"harbor.pause"), [] { runAsync([] { harbor::pauseHarbor(true); }); },
                               state == L"ready" || state == L"syncing"));
    }

    return submenu(t(L"tray.harbor"), std::move(items));
}

}

std::vector<MenuItem> trayMenu() {
    bool connected = isConnected();
    auto update = updateStatus();
    auto recorder = recording::recorderState();
    auto phase = jsonString(recorder, L"phase", L"idle");
    bool recordingActive = phase != L"idle";

    auto run = [](wchar_t const* name) { return [name] { runAction(name); }; };
    auto control = [](wchar_t const* name) { return [name] { runAsync([name] { recording::controlRecording(name); }); }; };

    std::vector<MenuItem> items;

    if (recordingActive) {
        items.push_back(action(jsonString(recorder, L"kind") == L"gif" ? t(L"tray.stopGifRecording") : t(L"tray.stopRecording"),
                               control(L"stop"), phase != L"processing"));
        items.push_back(phase == L"paused" ? action(t(L"tray.resume"), control(L"resume"))
                                           : action(t(L"tray.pause"), control(L"pause"), phase == L"recording"));
        items.push_back(action(t(L"tray.discardRecording"), control(L"abort"), phase != L"processing"));
        items.push_back(separator());
    }

    items.push_back(submenu(t(L"tray.capture"), {
                                                    action(t(L"tray.region"), run(L"captureRegion")),
                                                    action(t(L"tray.entireScreen"), run(L"captureFullscreen")),
                                                    action(t(L"tray.activeWindow"), run(L"captureWindow")),
                                                    action(t(L"tray.activeMonitor"), run(L"captureMonitor")),
                                                    action(t(L"tray.lastRegion"), run(L"captureLastRegion")),
                                                    separator(),
                                                    action(t(L"tray.textFromRegion"), run(L"captureText"), connected),
                                                }));
    items.push_back(submenu(t(L"tray.record"),
                            {action(t(L"tray.screenRecording"), run(L"recordVideo")), action(t(L"tray.gif"), run(L"recordGif"))},
                            !recordingActive));
    items.push_back(submenu(t(L"tray.upload"),
                            {
                                action(t(L"tray.file"), run(L"uploadFile")),
                                action(t(L"tray.fromClipboard"), run(L"uploadClipboard")),
                                action(t(L"tray.shortenFromClipboard"), run(L"shortenClipboard")),
                            },
                            connected));
    items.push_back(separator());
    items.push_back(action(t(L"tray.openFileship"), [] { showMainWindow(); }));
    items.push_back(harborMenu(connected));
    items.push_back(action(t(L"tray.openDashboard"), [] { openDashboard(); }, connected));
    items.push_back(separator());

    if (jsonString(update, L"state") == L"ready") {
        items.push_back(action(t(L"tray.restartToUpdate", {{L"version", jsonString(update, L"version")}}),
                               [] { runAsync(installUpdate); }));
    } else {
        items.push_back(action(t(L"tray.checkForUpdates"), [] { runAsync(checkForUpdates); }));
    }

    items.push_back(action(t(L"tray.settings"), [] { showMainWindow(L"/settings/general"); }));
    items.push_back(separator());
    items.push_back(action(t(L"tray.quit"), quitApp));

    return items;
}

std::wstring trayTooltip() {
    auto phase = jsonString(recording::recorderState(), L"phase", L"idle");

    if (phase == L"preparing" || phase == L"countdown") return t(L"tray.startingTooltip");
    if (phase == L"recording") return t(L"tray.recordingTooltip");
    if (phase == L"paused") return t(L"tray.pausedTooltip");
    if (phase == L"processing") return t(L"tray.processingTooltip");

    if (auto uploads = uploadSummary()) {
        return t(L"tray.uploadingTooltip",
                 {{L"count", std::to_wstring(uploads->count)}, {L"progress", std::to_wstring(uploads->progress)}});
    }

    return currentUser() ? L"Fileship · " + siteName() : L"Fileship";
}

void handleTrayClick() {
    if (jsonString(recording::recorderState(), L"phase", L"idle") == L"idle") return runAction(L"captureRegion");
    runAsync([] { recording::controlRecording(L"stop"); });
}

}

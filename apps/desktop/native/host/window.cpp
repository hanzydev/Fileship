#include "app.h"
#include "rpc.h"

#include <shared/log.h>

#include <chrono>
#include <mutex>

namespace fileship::host {

namespace {

constexpr int64_t LAUNCH_TIMEOUT_MS = 30000;

std::mutex mutex;
int64_t launchedAt = 0;
std::vector<std::pair<std::wstring, JsonObject>> pendingEvents;

int64_t steadyMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

JsonObject routePayload(std::wstring const& route) {
    JsonObject payload;
    payload.SetNamedValue(L"route", JsonValue::CreateStringValue(route));
    return payload;
}

void requestWindow(std::wstring const& event, std::wstring const& route, std::vector<std::wstring> arguments) {
    if (hasWindowClient()) {
        broadcast(event, routePayload(route));
        return;
    }

    {
        std::lock_guard lock(mutex);
        if (launchedAt && steadyMs() - launchedAt < LAUNCH_TIMEOUT_MS) {
            pendingEvents.emplace_back(event, routePayload(route));
            return;
        }

        launchedAt = steadyMs();
        pendingEvents.clear();
    }

    if (launchElectron(arguments)) return;

    std::lock_guard lock(mutex);
    launchedAt = 0;
}

}

void flushWindowEvents(uint64_t client) {
    std::vector<std::pair<std::wstring, JsonObject>> events;

    {
        std::lock_guard lock(mutex);
        launchedAt = 0;
        events.swap(pendingEvents);
    }

    for (auto const& [event, payload] : events) sendTo(client, event, payload);
}

void showMainWindow(std::wstring const& route) {
    requestWindow(L"window:show", route, {L"--route", route});
}

void openDashboard(std::wstring const& route) {
    if (instanceUrl().empty() || token().empty()) return;
    requestWindow(L"window:dashboard", route, {L"--dashboard", route});
}

}

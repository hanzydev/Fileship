#include <host/app.h>
#include <host/platform.h>
#include <shared/log.h>

namespace fileship::host {

void startUpdater() {}

JsonObject updateStatus() {
    JsonObject status;
    status.SetNamedValue(L"state", JsonValue::CreateStringValue(L"unsupported"));
    return status;
}

void checkForUpdates() {}

void installUpdate() {}

void notifyIdle() {}

void setShellHarborRoot(std::optional<std::filesystem::path> const&) {}

}

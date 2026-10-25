#include <host/harbor.h>

namespace fileship::host::harbor {

void startHarbor() {}
void stopHarbor(StopOptions) {}

JsonObject harborStatus() {
    return JsonObject();
}

JsonArray harborActivity() {
    return JsonArray();
}

void pauseHarbor(bool) {}
void runHarborAction(std::wstring const&, std::vector<std::wstring> const&) {}

std::optional<std::filesystem::path> harborPathFor(std::wstring const&) {
    return std::nullopt;
}

void registerHarborHandlers() {}

}

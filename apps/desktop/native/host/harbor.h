#pragma once

#include <shared/json.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fileship::host::harbor {

struct StopOptions {
    bool remove = false;
    bool forget = false;
};

void startHarbor();
void stopHarbor(StopOptions options = {});
JsonObject harborStatus();
JsonArray harborActivity();
void pauseHarbor(bool paused);
void runHarborAction(std::wstring const& action, std::vector<std::wstring> const& paths);
std::optional<std::filesystem::path> harborPathFor(std::wstring const& fileId);
void registerHarborHandlers();

}

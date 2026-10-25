#pragma once

#include <host/harbor.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace fileship::host::harbor {

std::filesystem::path harborFolder();
void startHarborAndWait();
int64_t freeUpHarborSpace();

}

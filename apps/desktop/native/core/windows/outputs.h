#pragma once

#include <core/core.h>

namespace fileship::core {

struct DisplayOutput {
    uint32_t adapter = 0;
    uint32_t output = 0;
    Rect rect;
};

std::vector<DisplayOutput> listOutputs();

}

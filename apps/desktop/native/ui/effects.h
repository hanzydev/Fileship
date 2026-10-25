#pragma once

#include <cstdint>

namespace fileship::ui {

void stackBlur(uint8_t* pixels, int width, int height, int radius);
void pixelate(uint8_t* pixels, int width, int height, int pixelSize);

}

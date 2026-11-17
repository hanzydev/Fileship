#pragma once

#include <windows.h>

#include <cstdint>

namespace fileship::ui {

void stackBlur(uint8_t* pixels, int width, int height, int radius);
void pixelate(uint8_t* pixels, int width, int height, int pixelSize);
int redactBlock(int width, int height);
void redactFill(uint8_t* bgra, int width, int height, int stride, RECT rect, uint32_t seed);

}

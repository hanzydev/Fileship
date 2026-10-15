#pragma once

#include <windows.h>

#include <napi.h>

#include <cstdint>
#include <string>
#include <vector>

namespace native {

struct Rect {
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
};

Rect readRect(Napi::Object const& object);
Napi::Object rectObject(Napi::Env env, Rect const& rect);
std::vector<uint64_t> readHandles(Napi::Value const& value);

void registerCapture(Napi::Env env, Napi::Object exports);
void registerWindows(Napi::Env env, Napi::Object exports);
void registerOutputs(Napi::Env env, Napi::Object exports);
void registerImage(Napi::Env env, Napi::Object exports);
void registerSound(Napi::Env env, Napi::Object exports);

}

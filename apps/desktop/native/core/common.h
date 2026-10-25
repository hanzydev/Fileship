#pragma once

#include <windows.h>

#include <napi.h>

#include "lib/core.h"

#include <cstdint>
#include <string>
#include <vector>

namespace native {

using fileship::core::Rect;

Rect readRect(Napi::Object const& object);
Napi::Object rectObject(Napi::Env env, Rect const& rect);
std::vector<uint64_t> readHandles(Napi::Value const& value);

void registerCapture(Napi::Env env, Napi::Object exports);
void registerWindows(Napi::Env env, Napi::Object exports);
void registerOutputs(Napi::Env env, Napi::Object exports);
void registerImage(Napi::Env env, Napi::Object exports);
void registerSound(Napi::Env env, Napi::Object exports);

}

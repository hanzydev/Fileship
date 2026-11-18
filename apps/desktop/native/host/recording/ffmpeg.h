#pragma once

#include <shared/json.h>

#include <optional>
#include <string>
#include <vector>

namespace fileship::host::recording {

struct Ffmpeg {
    std::wstring path;
    std::wstring version;
    bool ddagrab = false;
    std::vector<std::wstring> encoders;
};

std::optional<std::wstring> ffmpegPath();
std::optional<Ffmpeg> getFfmpeg();
JsonObject ffmpegStatus();
void publishFfmpegStatus();
void clearFfmpegProbe();

}

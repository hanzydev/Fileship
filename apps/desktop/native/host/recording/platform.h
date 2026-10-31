#pragma once

#include "command.h"

#include <filesystem>
#include <optional>
#include <string>

namespace fileship::host::recording {

struct FfmpegBuild {
    std::wstring version;
    std::wstring url;
    std::string sha256;
};

VideoSource videoSource(JsonObject const& recording, bool hasDdagrab, bool hasOutput);
std::wstring videoInput(VideoSource source, std::filesystem::path const& folder);

std::wstring ffmpegFileName();
std::optional<std::wstring> systemFfmpeg();
std::optional<FfmpegBuild> managedFfmpegBuild();
void extractFfmpeg(std::filesystem::path const& archive, std::filesystem::path const& folder);

}

#pragma once

#include "command.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace fileship::host::recording {

struct FfmpegBuild {
    std::wstring version;
    std::wstring url;
    std::string sha256;
};

class VideoCapture {
public:
    virtual ~VideoCapture() = default;

    virtual FrameInfo frameInfo() const = 0;
    virtual std::wstring startSegment(std::filesystem::path const& folder, std::wstring const& name, int fps) = 0;
    virtual void endSegment() = 0;
    virtual void closeSegment() = 0;
};

VideoSource videoSource(JsonObject const& recording, bool hasDdagrab, bool hasOutput);
std::wstring videoInput(VideoSource source, std::filesystem::path const& folder);
std::unique_ptr<VideoCapture> openVideoCapture(VideoSource source, RecordTarget const& target, bool cursor,
                                               std::function<void()> onEnded);

std::wstring ffmpegFileName();
std::optional<std::wstring> systemFfmpeg();
std::optional<FfmpegBuild> managedFfmpegBuild();
void extractFfmpeg(std::filesystem::path const& archive, std::filesystem::path const& folder);

}

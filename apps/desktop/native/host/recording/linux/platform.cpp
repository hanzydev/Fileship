#include <host/recording/platform.h>

#include <shared/utf8.h>

#include <glib.h>

#include <cstdlib>
#include <stdexcept>

namespace fileship::host::recording {

namespace {

bool isWayland() {
    auto session = std::getenv("XDG_SESSION_TYPE");
    if (session && std::string(session) == "x11") return false;
    return std::getenv("WAYLAND_DISPLAY") != nullptr;
}

}

VideoSource videoSource(JsonObject const& recording, bool, bool hasOutput) {
    auto source = text(recording, L"source", L"auto");
    if (source == L"x11grab") return VideoSource::X11grab;
    if (source == L"pipewire" && hasOutput) return VideoSource::Pipewire;
    return isWayland() && hasOutput ? VideoSource::Pipewire : VideoSource::X11grab;
}

std::wstring videoInput(VideoSource source, std::filesystem::path const& folder) {
    if (source == VideoSource::Pipewire) return (folder / L"video.raw").wstring();

    auto display = std::getenv("DISPLAY");
    return toWide(display && *display ? display : ":0");
}

std::wstring ffmpegFileName() {
    return L"ffmpeg";
}

std::optional<std::wstring> systemFfmpeg() {
    auto path = g_find_program_in_path("ffmpeg");
    if (!path) return std::nullopt;

    auto result = toWide(path);
    g_free(path);
    return result;
}

std::optional<FfmpegBuild> managedFfmpegBuild() {
    return std::nullopt;
}

void extractFfmpeg(std::filesystem::path const&, std::filesystem::path const&) {
    throw std::runtime_error("ffmpeg cannot be downloaded on this platform");
}

}

#include <host/recording/platform.h>
#include <host/recording/process.h>

#include <windows.h>

namespace fileship::host::recording {

VideoSource videoSource(JsonObject const& recording, bool hasDdagrab, bool hasOutput) {
    return text(recording, L"source", L"auto") != L"gdigrab" && hasDdagrab && hasOutput ? VideoSource::Ddagrab
                                                                                          : VideoSource::Gdigrab;
}

std::wstring videoInput(VideoSource, std::filesystem::path const&) {
    return L"desktop";
}

std::unique_ptr<VideoCapture> openVideoCapture(VideoSource, RecordTarget const&, bool, std::function<void()>) {
    return nullptr;
}

std::wstring ffmpegFileName() {
    return L"ffmpeg.exe";
}

std::optional<std::wstring> systemFfmpeg() {
    return std::nullopt;
}

std::optional<FfmpegBuild> managedFfmpegBuild() {
    return FfmpegBuild{
        L"9.0.2",
        L"https://github.com/GyanD/codexffmpeg/releases/download/9.0.2/ffmpeg-9.0.2-essentials_build.zip",
        "60f467265b1e312373dbcd92200c2618a74850f98d3d078e94296bb3fa2047ba",
    };
}

void extractFfmpeg(std::filesystem::path const& archive, std::filesystem::path const& folder) {
    wchar_t system[MAX_PATH]{};
    GetSystemDirectoryW(system, MAX_PATH);
    runProcess((std::filesystem::path(system) / L"tar.exe").wstring(),
               {L"-xf", archive.wstring(), L"-C", folder.wstring(), L"--strip-components", L"2",
                L"*/bin/ffmpeg.exe"});
}

}

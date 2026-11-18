#include "../test.h"

#include <host/windows/recording/video.h>

using namespace fileship;
using namespace fileship::host::recording;

namespace {

std::wstring commandOf(wchar_t const* json, std::wstring const& kind, std::wstring const& codec,
                       ui::ScreenRect const& rect, std::optional<OutputTarget> const& output,
                       std::vector<std::wstring> const& pipes, std::wstring const& file) {
    auto settings = JsonObject::Parse(json);
    auto request = videoRequest(settings, kind, codec, rect);
    auto tracks = kind == L"gif" ? std::vector<AudioTrack>{} : audioTracks(settings.GetNamedObject(L"audio"));

    return formatCommand(L"ffmpeg", recordArgs(settings, {kind, codec, output ? ddaInput(request, *output)
                                                                             : gdiInput(request),
                                                          pipes, tracks, file}));
}

}

TEST(recordingBuildsTheDesktopDuplicationCommand) {
    CHECK_EQUAL(commandOf(LR"cmd({"recording":{"codec":"libx264","fps":30,"quality":23,"showCursor":true,"container":"mp4"},"audio":{"microphone":true,"system":true,"tracks":"mixed","codec":"aac","bitrate":192,"delay":0},"gif":{"fps":15}})cmd", L"video", L"libx264", {100, 50, 801, 601}, OutputTarget{1, 0, {0, 0, 2560, 1440}},
                          {L"pipe-a"}, L"segment-0.mkv"),
                LR"cmd(ffmpeg -hide_banner -loglevel repeat+level+info -nostats -y -init_hw_device d3d11va=dda:1 -filter_hw_device dda -thread_queue_size 4096 -probesize 32 -analyzeduration 0 -itsoffset -0.1 -f f32le -ar 48000 -ac 2 -i pipe-a -filter_complex "ddagrab=output_idx=0:framerate=30:draw_mouse=1:offset_x=100:offset_y=50:video_size=800x600,showinfo=checksum=0,hwdownload,format=bgra,format=yuv420p[v]" -map "[v]" -map 0:a -filter:a:0 atrim=start=0 -metadata:s:a:0 title=recording.mixedAudio -c:v libx264 -preset veryfast -crf 23 -c:a aac -b:a 192k -ar 48000 -f matroska segment-0.mkv)cmd");
}

TEST(recordingBuildsTheGdiCommand) {
    CHECK_EQUAL(commandOf(LR"cmd({"recording":{"codec":"libvpx-vp9","fps":60,"quality":30,"showCursor":false,"container":"webm","rateControl":"bitrate","bitrate":6000,"inputArgs":"-probesize 5M","outputArgs":"-g 120 -tag:v \"vp09\""},"audio":{"microphone":true,"system":true,"tracks":"both","codec":"aac","bitrate":128,"delay":250},"gif":{"fps":15}})cmd", L"video", L"libvpx-vp9", {-1920, 0, 1920, 1080}, std::nullopt,
                          {L"pipe-a", L"pipe-b", L"pipe-c"}, L"out.mkv"),
                LR"cmd(ffmpeg -hide_banner -loglevel repeat+level+info -nostats -y -probesize 5M -f gdigrab -framerate 60 -draw_mouse 0 -offset_x -1920 -offset_y 0 -video_size 1920x1080 -i desktop -thread_queue_size 4096 -probesize 32 -analyzeduration 0 -itsoffset 0.15 -f f32le -ar 48000 -ac 2 -i pipe-a -thread_queue_size 4096 -probesize 32 -analyzeduration 0 -itsoffset 0.15 -f f32le -ar 48000 -ac 2 -i pipe-b -thread_queue_size 4096 -probesize 32 -analyzeduration 0 -itsoffset 0.15 -f f32le -ar 48000 -ac 2 -i pipe-c -filter_complex "[0:v]showinfo=checksum=0,fps=60,format=yuv420p[v]" -map "[v]" -map 1:a -filter:a:0 atrim=start=0 -metadata:s:a:0 title=recording.mixedAudio -map 2:a -filter:a:1 atrim=start=0 -metadata:s:a:1 title=recording.microphone -map 3:a -filter:a:2 atrim=start=0 -metadata:s:a:2 title=recording.systemAudio -c:v libvpx-vp9 -deadline realtime -cpu-used 8 -row-mt 1 -b:v 6000k -maxrate 6000k -bufsize 12000k -c:a libopus -b:a 128k -ar 48000 -g 120 -tag:v vp09 -f matroska out.mkv)cmd");
}

TEST(recordingBuildsTheGifCommand) {
    CHECK_EQUAL(commandOf(LR"cmd({"recording":{"codec":"h264_nvenc","fps":30,"showCursor":true},"audio":{"microphone":true},"gif":{"fps":12}})cmd", L"gif", L"libx264", {10, 20, 333, 222}, OutputTarget{0, 1, {0, 0, 1920, 1080}}, {},
                          L"gif.mkv"),
                LR"cmd(ffmpeg -hide_banner -loglevel repeat+level+info -nostats -y -init_hw_device d3d11va=dda:0 -filter_hw_device dda -filter_complex "ddagrab=output_idx=1:framerate=12:draw_mouse=1:offset_x=10:offset_y=20:video_size=332x222,showinfo=checksum=0,hwdownload,format=bgra,format=yuv444p[v]" -map "[v]" -c:v libx264 -preset ultrafast -qp 0 -f matroska gif.mkv)cmd");
}

TEST(recordingBuildsTheQuickSyncCommand) {
    CHECK_EQUAL(commandOf(LR"cmd({"recording":{"codec":"h264_qsv","fps":30,"quality":20,"showCursor":true,"container":"mkv","preset":"fast"},"audio":{"system":true,"codec":"libopus","bitrate":96},"gif":{}})cmd", L"video", L"h264_qsv", {0, 0, 1280, 720}, OutputTarget{0, 0, {0, 0, 1280, 720}},
                          {L"pipe-s"}, L"q.mkv"),
                LR"cmd(ffmpeg -hide_banner -loglevel repeat+level+info -nostats -y -init_hw_device d3d11va=dda:0 -filter_hw_device dda -thread_queue_size 4096 -probesize 32 -analyzeduration 0 -itsoffset -0.1 -f f32le -ar 48000 -ac 2 -i pipe-s -filter_complex "ddagrab=output_idx=0:framerate=30:draw_mouse=1:offset_x=0:offset_y=0:video_size=1280x720,showinfo=checksum=0,hwdownload,format=bgra,format=nv12[v]" -map "[v]" -map 0:a -filter:a:0 atrim=start=0 -metadata:s:a:0 title=recording.systemAudio -c:v h264_qsv -preset fast -global_quality 20 -c:a libopus -b:a 96k -ar 48000 -f matroska q.mkv)cmd");
}

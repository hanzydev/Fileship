#include "../test.h"

#include <host/linux/recording/video.h>

using namespace fileship;
using namespace fileship::host::recording;

namespace {

wchar_t const* const SETTINGS =
    LR"({"recording":{"codec":"libx264","fps":30,"quality":23,"showCursor":true,"container":"mp4"},"audio":{"microphone":true,"system":true,"tracks":"mixed","codec":"aac","bitrate":192,"delay":0},"gif":{"fps":15}})";

}

TEST(recordingBuildsTheX11Command) {
    auto settings = JsonObject::Parse(SETTINGS);
    auto request = videoRequest(settings, L"video", L"libx264", {100, 50, 801, 601});
    auto tracks = audioTracks(settings.GetNamedObject(L"audio"));

    CHECK_EQUAL(
        formatCommand(L"ffmpeg", recordArgs(settings, {L"video", L"libx264", x11Input(request, L":0"), {L"pipe-a"},
                                                       tracks, L"segment-0.mkv"})),
        LR"cmd(ffmpeg -hide_banner -loglevel repeat+level+info -nostats -y -f x11grab -framerate 30 -draw_mouse 1 -video_size 800x600 -i ":0+100,50" -thread_queue_size 4096 -probesize 32 -analyzeduration 0 -itsoffset -0.1 -f f32le -ar 48000 -ac 2 -i pipe-a -filter_complex "[0:v]showinfo=checksum=0,fps=30,format=yuv420p[v]" -map "[v]" -map 1:a -filter:a:0 atrim=start=0 -metadata:s:a:0 title=recording.mixedAudio -c:v libx264 -preset veryfast -crf 23 -c:a aac -b:a 192k -ar 48000 -f matroska segment-0.mkv)cmd");
}

TEST(recordingBuildsTheFrameStreamCommand) {
    auto settings = JsonObject::Parse(SETTINGS);
    auto request = videoRequest(settings, L"gif", L"libx264", {10, 20, 333, 222});

    CHECK_EQUAL(
        formatCommand(L"ffmpeg",
                      recordArgs(settings, {L"gif", L"libx264", framesInput(request, L"/run/video"), {}, {}, L"gif.mkv"})),
        LR"cmd(ffmpeg -hide_banner -loglevel repeat+level+info -nostats -y -f rawvideo -pixel_format bgr0 -video_size 332x222 -framerate 15 -i /run/video -filter_complex "[0:v]showinfo=checksum=0,fps=15,format=yuv444p[v]" -map "[v]" -c:v libx264 -preset ultrafast -qp 0 -f matroska gif.mkv)cmd");
}

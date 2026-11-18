#include "test.h"

#include <host/recording/command.h>

using namespace fileship;
using namespace fileship::host::recording;

namespace {

std::wstring joined(std::vector<std::wstring> const& args) {
    std::wstring result;
    for (auto const& arg : args) result += (result.empty() ? L"" : L" ") + arg;
    return result;
}

VideoInput const INPUT{{L"-i", L"screen"}, L"[0:v]format=FORMAT[v]", true};

}

TEST(recordingPicksAHardwareEncoderFirst) {
    CHECK_EQUAL(autoCodec({L"libx264", L"h264_vaapi", L"libopenh264"}), L"h264_vaapi");
    CHECK_EQUAL(autoCodec({L"libx264", L"h264_qsv", L"h264_nvenc"}), L"h264_nvenc");
}

TEST(recordingFallsBackToTheSoftwareEncoderThatExists) {
    CHECK_EQUAL(autoCodec({L"libx264", L"libopenh264"}), L"libx264");
    CHECK_EQUAL(autoCodec({L"libvpx-vp9", L"libopenh264"}), L"libopenh264");
    CHECK_EQUAL(autoCodec({}), L"libx264");
}

TEST(recordingPicksALosslessCodecForGifs) {
    CHECK_EQUAL(gifCodec({L"libx264", L"ffv1"}), L"libx264");
    CHECK_EQUAL(gifCodec({L"libopenh264", L"ffv1"}), L"ffv1");
}

TEST(recordingBuildsOpenH264Arguments) {
    auto quality = JsonObject::Parse(LR"({"quality":28})");
    CHECK_EQUAL(joined(videoCodecArgs(L"libopenh264", quality)), L"-c:v libopenh264 -rc_mode off -qmin 28 -qmax 28");

    auto bitrate = JsonObject::Parse(LR"({"rateControl":"bitrate","bitrate":4000})");
    CHECK_EQUAL(joined(videoCodecArgs(L"libopenh264", bitrate)),
                L"-c:v libopenh264 -rc_mode bitrate -b:v 4000k -maxrate 4000k -bufsize 8000k");
}

TEST(recordingBuildsVaapiArguments) {
    auto quality = JsonObject::Parse(LR"({"quality":24})");
    CHECK_EQUAL(joined(videoCodecArgs(L"h264_vaapi", quality)), L"-c:v h264_vaapi -rc_mode CQP -qp 24");
    CHECK_EQUAL(pixelFormat(L"hevc_vaapi"), L"nv12,hwupload");

    auto settings = JsonObject::Parse(LR"({"recording":{"quality":24}})");
    auto args = joined(recordArgs(settings, {L"video", L"h264_vaapi", INPUT, {}, {}, L"out.mkv"}));
    CHECK(args.find(L"-vaapi_device /dev/dri/renderD128 -i screen") != std::wstring::npos);
}

TEST(recordingKeepsGifsLosslessWithoutX264) {
    auto settings = JsonObject::Parse(LR"({"gif":{"fps":10}})");
    auto args = joined(recordArgs(settings, {L"gif", L"ffv1", INPUT, {}, {}, L"gif.mkv"}));
    CHECK(args.find(L"-c:v ffv1 -f matroska gif.mkv") != std::wstring::npos);
}

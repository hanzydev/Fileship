#pragma once

#include <shared/json.h>
#include <ui/overlay.h>

#include <optional>
#include <string>
#include <vector>

namespace fileship::host::recording {

constexpr int SAMPLE_RATE = 48'000;
constexpr int BYTES_PER_SECOND = SAMPLE_RATE * 2 * 4;
constexpr double PRIMING_SECONDS = 0.1;
constexpr char const* FIRST_FRAME_MARKER = "Parsed_showinfo";

extern std::vector<std::wstring> const VIDEO_CODECS;
extern std::vector<std::wstring> const HARDWARE_CODECS;

JsonObject section(JsonObject const& settings, wchar_t const* name);
std::wstring text(JsonObject const& object, wchar_t const* key, std::wstring const& fallback = {});
int number(JsonObject const& object, wchar_t const* key, int fallback);
bool flag(JsonObject const& object, wchar_t const* key, bool fallback);

struct AudioTrack {
    std::wstring id;
    std::wstring title;
    std::vector<std::wstring> sources;
};

struct VideoRequest {
    ui::ScreenRect rect;
    int fps = 30;
    int width = 0;
    int height = 0;
    std::wstring format;
    bool showsCursor = true;

    std::wstring rate() const {
        return std::to_wstring(fps);
    }

    std::wstring size() const {
        return std::to_wstring(width) + L"x" + std::to_wstring(height);
    }

    std::wstring cursor() const {
        return showsCursor ? L"1" : L"0";
    }
};

struct VideoInput {
    std::vector<std::wstring> args;
    std::wstring filter;
    bool isInput = true;
};

struct RecordCommand {
    std::wstring kind;
    std::wstring codec;
    VideoInput video;
    std::vector<std::wstring> pipes;
    std::vector<AudioTrack> tracks;
    std::wstring output;
};

std::vector<std::wstring> splitArgs(std::wstring const& value);
std::wstring autoCodec(std::vector<std::wstring> const& encoders);
std::wstring gifCodec(std::vector<std::wstring> const& encoders);
std::vector<std::wstring> testEncodeArgs(std::wstring const& codec);
std::vector<AudioTrack> audioTracks(JsonObject const& audio);
std::wstring defaultPreset(std::wstring const& codec);
std::wstring pixelFormat(std::wstring const& codec);
std::vector<std::wstring> videoCodecArgs(std::wstring const& codec, JsonObject const& recording);
std::optional<std::wstring> containerProblem(std::wstring const& container, std::wstring const& codec);
VideoRequest videoRequest(JsonObject const& settings, std::wstring const& kind, std::wstring const& codec,
                          ui::ScreenRect const& rect);
std::vector<std::wstring> recordArgs(JsonObject const& settings, RecordCommand const& command);
std::vector<std::wstring> segmentInput(std::vector<std::wstring> const& segments, std::wstring const& list);
std::vector<std::wstring> joinArgs(std::vector<std::wstring> const& input, std::wstring const& output,
                                   std::wstring const& container, std::vector<AudioTrack> const& tracks);
std::vector<std::wstring> gifArgs(std::vector<std::wstring> const& input, std::wstring const& output,
                                  JsonObject const& gif);
std::vector<std::wstring> thumbnailArgs(std::wstring const& input);
std::wstring formatCommand(std::wstring const& path, std::vector<std::wstring> const& args);
JsonArray tracksJson(std::vector<AudioTrack> const& tracks);

inline JsonValue jsonValue(JsonObject const& object) {
    return JsonValue::Parse(object.Stringify());
}

inline JsonValue jsonValue(JsonArray const& array) {
    return JsonValue::Parse(array.Stringify());
}

}

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

struct OutputTarget {
    int adapter = 0;
    int index = 0;
    ui::ScreenRect bounds;
};

struct RecordTarget {
    ui::ScreenRect rect;
    std::optional<OutputTarget> output;
};

struct RecordCommand {
    std::wstring kind;
    std::wstring codec;
    bool useDda = false;
    RecordTarget target;
    std::vector<std::wstring> pipes;
    std::vector<AudioTrack> tracks;
    std::wstring output;
};

std::vector<std::wstring> splitArgs(std::wstring const& value);
std::vector<AudioTrack> audioTracks(JsonObject const& audio);
std::wstring defaultPreset(std::wstring const& codec);
std::wstring pixelFormat(std::wstring const& codec);
std::vector<std::wstring> videoCodecArgs(std::wstring const& codec, JsonObject const& recording);
std::optional<std::wstring> containerProblem(std::wstring const& container, std::wstring const& codec);
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

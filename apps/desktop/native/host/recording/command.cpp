#include "command.h"

#include "process.h"

#include <shared/i18n.h>

#include <algorithm>
#include <charconv>
#include <cwctype>
#include <map>

namespace fileship::host::recording {

std::vector<std::wstring> const VIDEO_CODECS = {
    L"auto",      L"libx264",    L"libopenh264", L"libx265",   L"h264_nvenc", L"hevc_nvenc",
    L"av1_nvenc", L"h264_amf",   L"hevc_amf",    L"av1_amf",   L"h264_qsv",   L"hevc_qsv",
    L"av1_qsv",   L"h264_vaapi", L"hevc_vaapi",  L"av1_vaapi", L"libvpx-vp9", L"libsvtav1",
};

std::vector<std::wstring> const HARDWARE_CODECS = {L"h264_nvenc", L"h264_amf", L"h264_qsv", L"h264_vaapi"};

namespace {

std::map<std::wstring, std::wstring> const DEFAULT_PRESETS = {
    {L"libx264", L"veryfast"},  {L"libx265", L"veryfast"}, {L"h264_nvenc", L"p4"},
    {L"hevc_nvenc", L"p4"},     {L"av1_nvenc", L"p4"},     {L"h264_amf", L"balanced"},
    {L"hevc_amf", L"balanced"}, {L"av1_amf", L"balanced"}, {L"h264_qsv", L"veryfast"},
    {L"hevc_qsv", L"veryfast"}, {L"av1_qsv", L"veryfast"}, {L"libvpx-vp9", L"8"},
    {L"libsvtav1", L"10"},
};

bool endsWith(std::wstring const& value, std::wstring_view suffix) {
    return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool startsWith(std::wstring const& value, std::wstring_view prefix) {
    return value.compare(0, prefix.size(), prefix) == 0;
}

std::wstring decimal(double value) {
    char buffer[64]{};
    auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return std::wstring(buffer, result.ptr);
}

constexpr wchar_t const* VAAPI_DEVICE = L"/dev/dri/renderD128";

bool has(std::vector<std::wstring> const& values, std::wstring const& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

int even(int value) {
    return std::max(2, value - value % 2);
}

void append(std::vector<std::wstring>& target, std::vector<std::wstring> const& items) {
    target.insert(target.end(), items.begin(), items.end());
}

}

JsonObject section(JsonObject const& settings, wchar_t const* name) {
    return settings.GetNamedObject(name, JsonObject());
}

std::wstring text(JsonObject const& object, wchar_t const* key, std::wstring const& fallback) {
    return std::wstring(object.GetNamedString(key, fallback));
}

int number(JsonObject const& object, wchar_t const* key, int fallback) {
    return static_cast<int>(object.GetNamedNumber(key, fallback));
}

bool flag(JsonObject const& object, wchar_t const* key, bool fallback) {
    return object.GetNamedBoolean(key, fallback);
}

std::vector<std::wstring> splitArgs(std::wstring const& value) {
    std::vector<std::wstring> args;
    size_t index = 0;

    while (index < value.size()) {
        if (std::iswspace(value[index])) {
            index++;
            continue;
        }

        wchar_t quote = value[index];
        if (quote == L'"' || quote == L'\'') {
            size_t end = value.find(quote, index + 1);
            if (end != std::wstring::npos) {
                args.push_back(value.substr(index + 1, end - index - 1));
                index = end + 1;
                continue;
            }
        }

        size_t end = index;
        while (end < value.size() && !std::iswspace(value[end])) end++;
        args.push_back(value.substr(index, end - index));
        index = end;
    }

    return args;
}

std::wstring autoCodec(std::vector<std::wstring> const& encoders) {
    for (auto const& codec : HARDWARE_CODECS) {
        if (has(encoders, codec)) return codec;
    }

    return !has(encoders, L"libx264") && has(encoders, L"libopenh264") ? L"libopenh264" : L"libx264";
}

std::wstring gifCodec(std::vector<std::wstring> const& encoders) {
    return has(encoders, L"libx264") ? L"libx264" : L"ffv1";
}

std::vector<std::wstring> testEncodeArgs(std::wstring const& codec) {
    std::vector<std::wstring> args = {L"-hide_banner", L"-loglevel", L"error"};
    bool isVaapi = endsWith(codec, L"_vaapi");

    if (isVaapi) append(args, {L"-vaapi_device", VAAPI_DEVICE});
    append(args, {L"-f", L"lavfi", L"-i", L"color=c=black:s=256x256:r=30", L"-frames:v", L"3"});
    if (isVaapi) append(args, {L"-vf", L"format=nv12,hwupload"});
    append(args, {L"-c:v", codec, L"-f", L"null", L"-"});

    return args;
}

std::vector<AudioTrack> audioTracks(JsonObject const& audio) {
    std::vector<std::wstring> sources;
    if (flag(audio, L"microphone", false)) sources.push_back(L"microphone");
    if (flag(audio, L"system", false)) sources.push_back(L"system");
    if (sources.empty()) return {};

    std::vector<AudioTrack> separate;
    for (auto const& source : sources) {
        separate.push_back({source, t(source == L"microphone" ? L"recording.microphone" : L"recording.systemAudio"),
                            {source}});
    }

    auto tracks = text(audio, L"tracks", L"mixed");
    if (sources.size() == 1 || tracks == L"separate") return separate;

    AudioTrack mixed{L"mixed", t(L"recording.mixedAudio"), sources};
    if (tracks != L"both") return {mixed};

    separate.insert(separate.begin(), mixed);
    return separate;
}

std::wstring defaultPreset(std::wstring const& codec) {
    auto found = DEFAULT_PRESETS.find(codec);
    return found == DEFAULT_PRESETS.end() ? L"" : found->second;
}

std::wstring pixelFormat(std::wstring const& codec) {
    if (endsWith(codec, L"_vaapi")) return L"nv12,hwupload";
    return endsWith(codec, L"_qsv") ? L"nv12" : L"yuv420p";
}

std::vector<std::wstring> videoCodecArgs(std::wstring const& codec, JsonObject const& recording) {
    auto preset = text(recording, L"preset");
    if (preset.empty()) preset = defaultPreset(codec);

    auto quality = std::to_wstring(number(recording, L"quality", 23));
    auto kbps = number(recording, L"bitrate", 8000);
    std::optional<std::vector<std::wstring>> bitrate;

    if (text(recording, L"rateControl", L"quality") == L"bitrate") {
        bitrate = std::vector<std::wstring>{L"-b:v",     std::to_wstring(kbps) + L"k",
                                            L"-maxrate", std::to_wstring(kbps) + L"k",
                                            L"-bufsize", std::to_wstring(kbps * 2) + L"k"};
    }

    auto rate = [&](std::vector<std::wstring> const& fallback) { return bitrate ? *bitrate : fallback; };
    std::vector<std::wstring> args;

    if (codec == L"libx264" || codec == L"libx265") {
        args = {L"-c:v", codec, L"-preset", preset};
        append(args, rate({L"-crf", quality}));
    } else if (endsWith(codec, L"_nvenc")) {
        args = {L"-c:v", codec, L"-preset", preset, L"-tune", L"hq", L"-rc", L"vbr"};
        append(args, rate({L"-cq", quality, L"-b:v", L"0"}));
    } else if (endsWith(codec, L"_amf")) {
        args = {L"-c:v", codec, L"-quality", preset};
        if (bitrate) {
            append(args, {L"-rc", L"vbr_peak"});
            append(args, *bitrate);
        } else {
            append(args, {L"-rc", L"cqp", L"-qp_i", quality, L"-qp_p", quality});
        }
    } else if (endsWith(codec, L"_qsv")) {
        args = {L"-c:v", codec, L"-preset", preset};
        append(args, rate({L"-global_quality", quality}));
    } else if (endsWith(codec, L"_vaapi")) {
        args = {L"-c:v", codec};
        if (bitrate) {
            append(args, {L"-rc_mode", L"VBR"});
            append(args, *bitrate);
        } else {
            append(args, {L"-rc_mode", L"CQP", L"-qp", quality});
        }
    } else if (codec == L"libopenh264") {
        args = {L"-c:v", codec};
        if (bitrate) {
            append(args, {L"-rc_mode", L"bitrate"});
            append(args, *bitrate);
        } else {
            append(args, {L"-rc_mode", L"off", L"-qmin", quality, L"-qmax", quality});
        }
    } else if (codec == L"libvpx-vp9") {
        args = {L"-c:v", codec, L"-deadline", L"realtime", L"-cpu-used", preset, L"-row-mt", L"1"};
        append(args, rate({L"-crf", quality, L"-b:v", L"0"}));
    } else {
        args = {L"-c:v", codec, L"-preset", preset};
        append(args, rate({L"-crf", quality}));
    }

    return args;
}

std::optional<std::wstring> containerProblem(std::wstring const& container, std::wstring const& codec) {
    if (container != L"webm") return std::nullopt;
    if (codec == L"libvpx-vp9" || startsWith(codec, L"av1") || codec == L"libsvtav1") return std::nullopt;
    return t(L"recording.webmCodec");
}

VideoRequest videoRequest(JsonObject const& settings, std::wstring const& kind, std::wstring const& codec,
                          ui::ScreenRect const& rect) {
    auto recording = section(settings, L"recording");
    bool isGif = kind == L"gif";

    return {rect,
            isGif ? number(section(settings, L"gif"), L"fps", 15) : number(recording, L"fps", 30),
            even(rect.width),
            even(rect.height),
            isGif ? std::wstring(L"yuv444p") : pixelFormat(codec),
            flag(recording, L"showCursor", true)};
}

std::string lastLine(std::string const& text) {
    size_t end = text.find_last_not_of(" \t\r\n");
    if (end == std::string::npos) return {};

    size_t start = text.find_last_of("\r\n", end);
    start = start == std::string::npos ? 0 : start + 1;

    return text.substr(start, std::min<size_t>(end - start + 1, 200));
}

std::vector<std::wstring> recordArgs(JsonObject const& settings, RecordCommand const& command) {
    auto recording = section(settings, L"recording");
    auto audio = section(settings, L"audio");
    bool isGif = command.kind == L"gif";

    std::vector<std::wstring> args = {L"-hide_banner", L"-loglevel", L"repeat+level+info", L"-nostats", L"-y"};
    append(args, splitArgs(text(recording, L"inputArgs")));
    if (endsWith(command.codec, L"_vaapi")) append(args, {L"-vaapi_device", VAAPI_DEVICE});
    append(args, command.video.args);

    size_t firstAudio = command.video.isInput ? 1 : 0;
    auto tracks = isGif ? std::vector<AudioTrack>{} : command.tracks;
    auto offset = decimal(number(audio, L"delay", 0) / 1000.0 - PRIMING_SECONDS);

    for (size_t index = 0; index < std::min(command.pipes.size(), tracks.size()); index++) {
        append(args, {L"-thread_queue_size", L"4096", L"-probesize", L"32", L"-analyzeduration", L"0",
                      L"-itsoffset", offset, L"-f", L"f32le", L"-ar", std::to_wstring(SAMPLE_RATE), L"-ac",
                      L"2", L"-i", command.pipes[index]});
    }

    append(args, {L"-filter_complex", command.video.filter, L"-map", L"[v]"});

    for (size_t index = 0; index < tracks.size(); index++) {
        auto stream = std::to_wstring(index);
        append(args, {L"-map", std::to_wstring(firstAudio + index) + L":a", L"-filter:a:" + stream,
                      L"atrim=start=0", L"-metadata:s:a:" + stream, L"title=" + tracks[index].title});
    }

    if (isGif && command.codec == L"ffv1") {
        append(args, {L"-c:v", L"ffv1"});
    } else if (isGif) {
        append(args, {L"-c:v", L"libx264", L"-preset", L"ultrafast", L"-qp", L"0"});
    } else {
        append(args, videoCodecArgs(command.codec, recording));
    }

    if (!tracks.empty()) {
        auto codec = text(recording, L"container", L"mp4") == L"webm" ? std::wstring(L"libopus")
                                                                       : text(audio, L"codec", L"aac");
        append(args, {L"-c:a", codec, L"-b:a", std::to_wstring(number(audio, L"bitrate", 192)) + L"k", L"-ar",
                      std::to_wstring(SAMPLE_RATE)});
    }

    if (!isGif) append(args, splitArgs(text(recording, L"outputArgs")));
    append(args, {L"-f", L"matroska", command.output});

    return args;
}

std::vector<std::wstring> segmentInput(std::vector<std::wstring> const& segments, std::wstring const& list) {
    if (segments.size() == 1) return {L"-i", segments[0]};
    return {L"-f", L"concat", L"-safe", L"0", L"-i", list};
}

std::vector<std::wstring> joinArgs(std::vector<std::wstring> const& input, std::wstring const& output,
                                   std::wstring const& container, std::vector<AudioTrack> const& tracks) {
    std::vector<std::wstring> args = {L"-hide_banner", L"-loglevel", L"error", L"-y"};
    append(args, input);
    append(args, {L"-map", L"0", L"-c", L"copy"});

    for (size_t index = 0; index < tracks.size(); index++) {
        auto metadata = L"-metadata:s:a:" + std::to_wstring(index);
        append(args, {metadata, L"title=" + tracks[index].title, metadata, L"handler_name=" + tracks[index].title});
    }

    if (container == L"mp4") append(args, {L"-movflags", L"+faststart"});
    args.push_back(output);

    return args;
}

std::vector<std::wstring> gifArgs(std::vector<std::wstring> const& input, std::wstring const& output,
                                  JsonObject const& gif) {
    auto maxWidth = number(gif, L"maxWidth", 0);
    auto scale = maxWidth ? L",scale='min(iw," + std::to_wstring(maxWidth) + L")':-1:flags=lanczos" : L"";
    auto dither = text(gif, L"dither", L"sierra2_4a");
    if (dither == L"bayer") dither = L"bayer:bayer_scale=3";
    auto statsMode = text(gif, L"statsMode", L"diff");

    std::vector<std::wstring> args = {L"-hide_banner", L"-loglevel", L"error", L"-y"};
    append(args, input);
    append(args, {L"-filter_complex",
                  L"[0:v]fps=" + std::to_wstring(number(gif, L"fps", 15)) + scale +
                      L",split[a][b];[a]palettegen=stats_mode=" + statsMode + L"[p];[b][p]paletteuse=dither=" +
                      dither + (statsMode == L"diff" ? L":diff_mode=rectangle" : L""),
                  L"-loop", flag(gif, L"loop", true) ? L"0" : L"-1", output});

    return args;
}

std::vector<std::wstring> thumbnailArgs(std::wstring const& input) {
    return {L"-hide_banner", L"-loglevel", L"error",      L"-i",   input,  L"-frames:v",
            L"1",            L"-vf",       L"scale=480:-2", L"-f",  L"image2pipe", L"-c:v",
            L"mjpeg",        L"-q:v",      L"5",           L"pipe:1"};
}

std::wstring formatCommand(std::wstring const& path, std::vector<std::wstring> const& args) {
    std::wstring result;

    auto add = [&](std::wstring const& arg) {
        if (!result.empty()) result += L' ';

        bool needsQuotes = std::any_of(arg.begin(), arg.end(), [](wchar_t c) {
            return std::iswspace(c) || c == L'"' || c == L'\'' || c == L'[' || c == L']' || c == L';' || c == L',';
        });
        if (!needsQuotes) {
            result += arg;
            return;
        }

        result += L'"';
        for (auto c : arg) {
            if (c == L'"') result += L'\\';
            result += c;
        }
        result += L'"';
    };

    add(path);
    for (auto const& arg : args) add(arg);

    return result;
}

JsonArray tracksJson(std::vector<AudioTrack> const& tracks) {
    JsonArray result;

    for (auto const& track : tracks) {
        JsonObject item;
        JsonArray sources;
        for (auto const& source : track.sources) sources.Append(JsonValue::CreateStringValue(source));

        item.SetNamedValue(L"id", JsonValue::CreateStringValue(track.id));
        item.SetNamedValue(L"title", JsonValue::CreateStringValue(track.title));
        item.SetNamedValue(L"sources", sources);
        result.Append(item);
    }

    return result;
}

}

#include <core/core.h>

#include <pulse/simple.h>

#include <cstring>
#include <thread>

namespace fileship::core {

namespace {

struct Wave {
    pa_sample_spec spec{};
    std::vector<uint8_t> samples;
};

uint32_t readNumber(uint8_t const* data, int bytes) {
    uint32_t value = 0;
    for (int index = bytes - 1; index >= 0; index--) value = (value << 8) | data[index];
    return value;
}

std::optional<Wave> parseWave(uint8_t const* data, size_t size) {
    if (size < 12 || std::memcmp(data, "RIFF", 4) != 0 || std::memcmp(data + 8, "WAVE", 4) != 0)
        return std::nullopt;

    Wave wave;
    bool hasFormat = false;

    for (size_t offset = 12; offset + 8 <= size;) {
        auto length = readNumber(data + offset + 4, 4);
        auto body = data + offset + 8;
        if (offset + 8 + length > size) break;

        if (std::memcmp(data + offset, "fmt ", 4) == 0 && length >= 16) {
            auto format = readNumber(body, 2);
            auto bits = readNumber(body + 14, 2);
            if (format != 1 || (bits != 8 && bits != 16 && bits != 24 && bits != 32)) return std::nullopt;

            wave.spec.format = bits == 8    ? PA_SAMPLE_U8
                               : bits == 16 ? PA_SAMPLE_S16LE
                               : bits == 24 ? PA_SAMPLE_S24LE
                                            : PA_SAMPLE_S32LE;
            wave.spec.channels = static_cast<uint8_t>(readNumber(body + 2, 2));
            wave.spec.rate = readNumber(body + 4, 4);
            hasFormat = true;
        } else if (std::memcmp(data + offset, "data", 4) == 0 && hasFormat) {
            wave.samples.assign(body, body + length);
            return wave;
        }

        offset += 8 + length + (length & 1);
    }

    return std::nullopt;
}

}

bool playSound(uint8_t const* wav, size_t size) {
    auto wave = parseWave(wav, size);
    if (!wave || !pa_sample_spec_valid(&wave->spec)) return false;

    std::thread([wave = std::move(*wave)] {
        int error = 0;
        auto stream = pa_simple_new(nullptr, "Fileship", PA_STREAM_PLAYBACK, nullptr, "sound", &wave.spec, nullptr,
                                    nullptr, &error);
        if (!stream) return;

        pa_simple_write(stream, wave.samples.data(), wave.samples.size(), &error);
        pa_simple_drain(stream, &error);
        pa_simple_free(stream);
    }).detach();

    return true;
}

}

#include <core/core.h>

#include <pulse/simple.h>

#include <algorithm>
#include <cstring>
#include <thread>

namespace fileship::core {

namespace {

template <typename Value> Value read(uint8_t const* data) {
    Value value{};
    std::memcpy(&value, data, sizeof(value));
    return value;
}

}

bool playSound(uint8_t const* wav, size_t size) {
    pa_sample_spec format{PA_SAMPLE_S16LE, 0, 0};
    std::vector<uint8_t> samples;

    for (size_t offset = 12; offset + 8 <= size;) {
        auto body = wav + offset + 8;
        auto length = std::min<size_t>(read<uint32_t>(wav + offset + 4), size - offset - 8);

        if (!std::memcmp(wav + offset, "fmt ", 4)) {
            format.channels = static_cast<uint8_t>(read<uint16_t>(body + 2));
            format.rate = read<uint32_t>(body + 4);
        } else if (!std::memcmp(wav + offset, "data", 4)) {
            samples.assign(body, body + length);
        }

        offset += 8 + length + (length & 1);
    }

    if (!pa_sample_spec_valid(&format) || samples.empty()) return false;

    std::thread([format, samples = std::move(samples)] {
        pa_simple* stream = pa_simple_new(nullptr, "Fileship", PA_STREAM_PLAYBACK, nullptr, "Fileship", &format,
                                          nullptr, nullptr, nullptr);
        if (!stream) return;

        pa_simple_write(stream, samples.data(), samples.size(), nullptr);
        pa_simple_drain(stream, nullptr);
        pa_simple_free(stream);
    }).detach();

    return true;
}

}

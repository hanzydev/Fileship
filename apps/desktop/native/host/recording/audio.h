#pragma once

#include "command.h"

#include <shared/json.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace fileship::host::recording {

class AudioEngine {
public:
    using ChunkHandler = std::function<void(size_t track, std::vector<float> const& samples)>;

    static std::unique_ptr<AudioEngine> create(JsonObject const& audio, std::vector<AudioTrack> const& tracks,
                                               ChunkHandler onChunk);

    virtual ~AudioEngine() = default;

    virtual void setMuted(std::wstring const& source, bool muted) = 0;
};

JsonArray listMicrophones();

}

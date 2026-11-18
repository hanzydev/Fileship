#pragma once

#include <memory>
#include <string>

namespace fileship::host::recording {

class AudioPipe {
public:
    static std::wstring nameOf(std::wstring const& label);
    static std::unique_ptr<AudioPipe> create(std::wstring const& label, size_t primingBytes);

    virtual ~AudioPipe() = default;

    virtual std::wstring const& name() const = 0;
    virtual void write(char const* data, size_t size) = 0;
    virtual void end() = 0;
};

}

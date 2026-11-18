#pragma once

#include <cstddef>
#include <cstdint>
#include <random>

namespace fileship {

inline void randomBytes(void* data, size_t size) {
    std::random_device device;
    auto bytes = static_cast<uint8_t*>(data);

    for (size_t index = 0; index < size; index++) bytes[index] = static_cast<uint8_t>(device());
}

}

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fileship {

std::wstring base64(void const* data, size_t size);
std::vector<uint8_t> fromBase64(std::wstring const& text);

}

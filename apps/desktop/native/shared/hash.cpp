#include "hash.h"

namespace fileship {

std::string toHex(std::vector<uint8_t> const& bytes) {
    static constexpr char DIGITS[] = "0123456789abcdef";
    std::string result;
    result.reserve(bytes.size() * 2);
    for (auto byte : bytes) {
        result += DIGITS[byte >> 4];
        result += DIGITS[byte & 0xF];
    }
    return result;
}

std::string hashHex(HashKind kind, std::string_view data) {
    Hasher hasher(kind);
    hasher.update(data.data(), data.size());
    return toHex(hasher.finish());
}

}

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace fileship {

enum class HashKind { Sha1, Sha256, Sha512 };

class Hasher {
public:
    explicit Hasher(HashKind kind);
    ~Hasher();
    Hasher(Hasher const&) = delete;
    Hasher& operator=(Hasher const&) = delete;

    void update(void const* data, size_t size);
    std::vector<uint8_t> finish();

private:
    struct State;
    std::unique_ptr<State> state;
};

std::string toHex(std::vector<uint8_t> const& bytes);
std::string hashHex(HashKind kind, std::string_view data);

}

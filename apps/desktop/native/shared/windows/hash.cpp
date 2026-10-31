#include <shared/hash.h>

#include <windows.h>

#include <bcrypt.h>

namespace fileship {

struct Hasher::State {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    ULONG length = 0;
};

Hasher::Hasher(HashKind kind) : state(std::make_unique<State>()) {
    auto name = kind == HashKind::Sha1     ? BCRYPT_SHA1_ALGORITHM
                : kind == HashKind::Sha256 ? BCRYPT_SHA256_ALGORITHM
                                           : BCRYPT_SHA512_ALGORITHM;
    BCryptOpenAlgorithmProvider(&state->algorithm, name, nullptr, 0);

    ULONG size = 0;
    BCryptGetProperty(state->algorithm, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&state->length),
                      sizeof(state->length), &size, 0);
    BCryptCreateHash(state->algorithm, &state->hash, nullptr, 0, nullptr, 0, 0);
}

Hasher::~Hasher() {
    if (state->hash) BCryptDestroyHash(state->hash);
    if (state->algorithm) BCryptCloseAlgorithmProvider(state->algorithm, 0);
}

void Hasher::update(void const* data, size_t size) {
    BCryptHashData(state->hash, static_cast<PUCHAR>(const_cast<void*>(data)), static_cast<ULONG>(size), 0);
}

std::vector<uint8_t> Hasher::finish() {
    std::vector<uint8_t> digest(state->length);
    BCryptFinishHash(state->hash, digest.data(), state->length, 0);
    return digest;
}

}

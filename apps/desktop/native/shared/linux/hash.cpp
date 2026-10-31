#include <shared/hash.h>

#include <glib.h>

namespace fileship {

struct Hasher::State {
    GChecksum* checksum = nullptr;
};

Hasher::Hasher(HashKind kind) : state(std::make_unique<State>()) {
    state->checksum = g_checksum_new(kind == HashKind::Sha1     ? G_CHECKSUM_SHA1
                                     : kind == HashKind::Sha256 ? G_CHECKSUM_SHA256
                                                                : G_CHECKSUM_SHA512);
}

Hasher::~Hasher() {
    g_checksum_free(state->checksum);
}

void Hasher::update(void const* data, size_t size) {
    g_checksum_update(state->checksum, static_cast<guchar const*>(data), static_cast<gssize>(size));
}

std::vector<uint8_t> Hasher::finish() {
    std::vector<uint8_t> digest(64);
    gsize length = digest.size();
    g_checksum_get_digest(state->checksum, digest.data(), &length);
    digest.resize(length);
    return digest;
}

}

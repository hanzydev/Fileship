#include "core.h"

#include <mmsystem.h>

#include <algorithm>
#include <mutex>

namespace fileship::core {

namespace {

std::mutex mutex;
std::vector<uint8_t> sound;

}

bool playSound(uint8_t const* wav, size_t size) {
    std::lock_guard lock(mutex);
    bool isSame = size == sound.size() && std::equal(sound.begin(), sound.end(), wav);

    if (!isSame) {
        PlaySoundW(nullptr, nullptr, 0);
        sound.assign(wav, wav + size);
    }

    return PlaySoundW(reinterpret_cast<LPCWSTR>(sound.data()), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT) !=
           FALSE;
}

}

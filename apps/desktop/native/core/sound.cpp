#include "common.h"

#include <mmsystem.h>

#include <algorithm>

namespace native {

namespace {

std::vector<uint8_t> sound;

Napi::Value playSound(Napi::CallbackInfo const& info) {
    auto env = info.Env();
    if (!info[0].IsBuffer()) throw Napi::TypeError::New(env, "Expected a WAV buffer");

    auto buffer = info[0].As<Napi::Buffer<uint8_t>>();
    bool isSame = buffer.Length() == sound.size() && std::equal(sound.begin(), sound.end(), buffer.Data());

    if (!isSame) {
        PlaySoundW(nullptr, nullptr, 0);
        sound.assign(buffer.Data(), buffer.Data() + buffer.Length());
    }

    BOOL played = PlaySoundW(reinterpret_cast<LPCWSTR>(sound.data()), nullptr,
                             SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
    return Napi::Boolean::New(env, played != FALSE);
}

}

void registerSound(Napi::Env env, Napi::Object exports) {
    exports.Set("playSound", Napi::Function::New(env, playSound));
}

}

#include "common.h"

namespace native {

namespace {

Napi::Value playSound(Napi::CallbackInfo const& info) {
    auto env = info.Env();
    if (!info[0].IsBuffer()) throw Napi::TypeError::New(env, "Expected a WAV buffer");

    auto buffer = info[0].As<Napi::Buffer<uint8_t>>();
    return Napi::Boolean::New(env, fileship::core::playSound(buffer.Data(), buffer.Length()));
}

}

void registerSound(Napi::Env env, Napi::Object exports) {
    exports.Set("playSound", Napi::Function::New(env, playSound));
}

}

#include "common.h"

namespace native {

namespace {

Napi::Value listOutputs(Napi::CallbackInfo const& info) {
    auto env = info.Env();
    auto outputs = fileship::core::listOutputs();
    auto result = Napi::Array::New(env, outputs.size());

    for (size_t index = 0; index < outputs.size(); index++) {
        auto item = Napi::Object::New(env);
        item.Set("adapter", outputs[index].adapter);
        item.Set("output", outputs[index].output);
        item.Set("rect", rectObject(env, outputs[index].rect));
        result.Set(static_cast<uint32_t>(index), item);
    }

    return result;
}

}

void registerOutputs(Napi::Env env, Napi::Object exports) {
    exports.Set("listOutputs", Napi::Function::New(env, listOutputs));
}

}

#include "common.h"

namespace native {

Rect readRect(Napi::Object const& object) {
    return Rect{
        object.Get("x").ToNumber().Int32Value(),
        object.Get("y").ToNumber().Int32Value(),
        object.Get("width").ToNumber().Int32Value(),
        object.Get("height").ToNumber().Int32Value(),
    };
}

Napi::Object rectObject(Napi::Env env, Rect const& rect) {
    auto object = Napi::Object::New(env);
    object.Set("x", rect.x);
    object.Set("y", rect.y);
    object.Set("width", rect.width);
    object.Set("height", rect.height);
    return object;
}

std::vector<uint64_t> readHandles(Napi::Value const& value) {
    std::vector<uint64_t> handles;
    if (!value.IsArray()) return handles;

    auto array = value.As<Napi::Array>();

    for (uint32_t index = 0; index < array.Length(); index++) {
        Napi::Value item = array.Get(index);
        bool isLossless = false;

        if (item.IsBigInt()) handles.push_back(item.As<Napi::BigInt>().Uint64Value(&isLossless));
    }

    return handles;
}

}

Napi::Object initialize(Napi::Env env, Napi::Object exports) {
    native::registerCapture(env, exports);
    native::registerWindows(env, exports);
    native::registerOutputs(env, exports);
    native::registerImage(env, exports);
    return exports;
}

NODE_API_MODULE(fileship_native, initialize)

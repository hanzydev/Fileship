#include "common.h"

namespace native {

namespace {

using fileship::core::WindowInfo;

Napi::String wideString(Napi::Env env, std::wstring const& text) {
    return Napi::String::New(env, reinterpret_cast<char16_t const*>(text.c_str()), text.size());
}

Napi::Object windowObject(Napi::Env env, WindowInfo const& window) {
    auto object = rectObject(env, window.frame);
    auto controls = Napi::Array::New(env, window.controls.size());

    for (size_t index = 0; index < window.controls.size(); index++)
        controls.Set(static_cast<uint32_t>(index), rectObject(env, window.controls[index]));

    object.Set("title", wideString(env, window.title));
    object.Set("controls", controls);
    return object;
}

class ListWindowsWorker : public Napi::AsyncWorker {
public:
    ListWindowsWorker(Napi::Env env, std::vector<uint64_t> ignored, bool controls)
        : Napi::AsyncWorker(env), deferred(Napi::Promise::Deferred::New(env)), ignored(std::move(ignored)),
          controls(controls) {}

    Napi::Promise promise() {
        return deferred.Promise();
    }

protected:
    void Execute() override {
        windows = fileship::core::listWindows(ignored, controls);
    }

    void OnOK() override {
        auto env = Env();
        auto result = Napi::Array::New(env, windows.size());

        for (size_t index = 0; index < windows.size(); index++)
            result.Set(static_cast<uint32_t>(index), windowObject(env, windows[index]));

        deferred.Resolve(result);
    }

    void OnError(Napi::Error const& error) override {
        deferred.Reject(error.Value());
    }

private:
    Napi::Promise::Deferred deferred;
    std::vector<uint64_t> ignored;
    bool controls;
    std::vector<WindowInfo> windows;
};

Napi::Value listWindows(Napi::CallbackInfo const& info) {
    auto env = info.Env();
    bool controls = info[1].IsObject() && info[1].As<Napi::Object>().Get("controls").ToBoolean().Value();

    auto worker = new ListWindowsWorker(env, readHandles(info[0]), controls);
    auto promise = worker->promise();
    worker->Queue();
    return promise;
}

Napi::Value foregroundWindow(Napi::CallbackInfo const& info) {
    auto env = info.Env();
    auto window = fileship::core::foregroundWindow(readHandles(info[0]));
    if (!window) return env.Null();

    auto object = rectObject(env, window->frame);
    object.Set("title", wideString(env, window->title));
    return object;
}

Napi::Value forceForeground(Napi::CallbackInfo const& info) {
    auto env = info.Env();

    if (!info[0].IsBigInt()) throw Napi::TypeError::New(env, "Expected a window handle");

    bool isLossless = false;
    auto window = reinterpret_cast<HWND>(static_cast<uintptr_t>(info[0].As<Napi::BigInt>().Uint64Value(&isLossless)));
    fileship::core::forceForeground(window);

    return env.Undefined();
}

Napi::Value doubleClickTime(Napi::CallbackInfo const& info) {
    return Napi::Number::New(info.Env(), GetDoubleClickTime());
}

}

void registerWindows(Napi::Env env, Napi::Object exports) {
    exports.Set("listWindows", Napi::Function::New(env, listWindows));
    exports.Set("foregroundWindow", Napi::Function::New(env, foregroundWindow));
    exports.Set("forceForeground", Napi::Function::New(env, forceForeground));
    exports.Set("doubleClickTime", Napi::Function::New(env, doubleClickTime));
}

}

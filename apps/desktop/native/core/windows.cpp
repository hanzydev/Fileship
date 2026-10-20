#include "common.h"

#include <dwmapi.h>

#include <algorithm>

namespace native {

namespace {

constexpr int MAX_CONTROLS = 3000;

struct WindowInfo {
    Rect frame;
    std::u16string title;
    std::vector<Rect> controls;
};

std::u16string readTitle(HWND window) {
    wchar_t buffer[256]{};
    int length = GetWindowTextW(window, buffer, 256);
    return std::u16string(reinterpret_cast<char16_t const*>(buffer), std::max(length, 0));
}

bool readFrame(HWND window, Rect& frame) {
    RECT rect{};

    if (FAILED(DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect))) &&
        !GetWindowRect(window, &rect))
        return false;

    frame = Rect{rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top};
    return frame.width > 0 && frame.height > 0;
}

bool isCloaked(HWND window) {
    DWORD cloaked = 0;
    return SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked != 0;
}

bool isIgnored(std::vector<uint64_t> const& ignored, HWND window) {
    auto handle = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(window));
    return std::find(ignored.begin(), ignored.end(), handle) != ignored.end();
}

struct Enumeration {
    std::vector<uint64_t> const* ignored;
    bool controls;
    int controlCount = 0;
    std::vector<WindowInfo> windows;
    WindowInfo* current = nullptr;
};

BOOL CALLBACK handleChild(HWND child, LPARAM parameter) {
    auto& state = *reinterpret_cast<Enumeration*>(parameter);
    RECT rect{};

    if (IsWindowVisible(child) && GetWindowRect(child, &rect)) {
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        if (width > 4 && height > 4 && state.controlCount < MAX_CONTROLS) {
            state.current->controls.push_back(Rect{rect.left, rect.top, width, height});
            state.controlCount++;
        }
    }

    return state.controlCount < MAX_CONTROLS;
}

BOOL CALLBACK handleWindow(HWND window, LPARAM parameter) {
    auto& state = *reinterpret_cast<Enumeration*>(parameter);

    if (isIgnored(*state.ignored, window)) return TRUE;
    if (!IsWindowVisible(window) || IsIconic(window) || isCloaked(window)) return TRUE;

    auto exStyle = GetWindowLongPtrW(window, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW || exStyle & WS_EX_NOACTIVATE) return TRUE;

    WindowInfo info;
    if (!readFrame(window, info.frame)) return TRUE;

    info.title = readTitle(window);

    if (state.controls && state.controlCount < MAX_CONTROLS) {
        state.current = &info;
        EnumChildWindows(window, handleChild, parameter);
        state.current = nullptr;
    }

    state.windows.push_back(std::move(info));
    return TRUE;
}

Napi::Object windowObject(Napi::Env env, WindowInfo const& window) {
    auto object = rectObject(env, window.frame);
    auto controls = Napi::Array::New(env, window.controls.size());

    for (size_t index = 0; index < window.controls.size(); index++)
        controls.Set(static_cast<uint32_t>(index), rectObject(env, window.controls[index]));

    object.Set("title", Napi::String::New(env, window.title));
    object.Set("controls", controls);
    return object;
}

class ListWindowsWorker : public Napi::AsyncWorker {
public:
    ListWindowsWorker(Napi::Env env, std::vector<uint64_t> ignored, bool controls)
        : Napi::AsyncWorker(env), deferred(Napi::Promise::Deferred::New(env)), ignored(std::move(ignored)) {
        state.ignored = &this->ignored;
        state.controls = controls;
    }

    Napi::Promise promise() {
        return deferred.Promise();
    }

protected:
    void Execute() override {
        EnumWindows(handleWindow, reinterpret_cast<LPARAM>(&state));
    }

    void OnOK() override {
        auto env = Env();
        auto result = Napi::Array::New(env, state.windows.size());

        for (size_t index = 0; index < state.windows.size(); index++)
            result.Set(static_cast<uint32_t>(index), windowObject(env, state.windows[index]));

        deferred.Resolve(result);
    }

    void OnError(Napi::Error const& error) override {
        deferred.Reject(error.Value());
    }

private:
    Napi::Promise::Deferred deferred;
    std::vector<uint64_t> ignored;
    Enumeration state;
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
    HWND window = GetForegroundWindow();

    if (!window || isIgnored(readHandles(info[0]), window)) return env.Null();

    Rect frame;
    if (!readFrame(window, frame)) return env.Null();

    auto object = rectObject(env, frame);
    object.Set("title", Napi::String::New(env, readTitle(window)));
    return object;
}

Napi::Value forceForeground(Napi::CallbackInfo const& info) {
    auto env = info.Env();

    if (!info[0].IsBigInt()) throw Napi::TypeError::New(env, "Expected a window handle");

    bool isLossless = false;
    auto window = reinterpret_cast<HWND>(static_cast<uintptr_t>(info[0].As<Napi::BigInt>().Uint64Value(&isLossless)));

    HWND foreground = GetForegroundWindow();
    DWORD current = GetCurrentThreadId();
    DWORD target = foreground ? GetWindowThreadProcessId(foreground, nullptr) : 0;
    bool shouldAttach = target != 0 && target != current;

    if (shouldAttach) AttachThreadInput(current, target, TRUE);

    BringWindowToTop(window);
    SetForegroundWindow(window);
    SetFocus(window);

    if (shouldAttach) AttachThreadInput(current, target, FALSE);

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

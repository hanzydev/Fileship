#include "common.h"

namespace native {

namespace {

class CaptureWorker : public Napi::AsyncWorker {
public:
    CaptureWorker(Napi::Env env, std::vector<Rect> rects, bool cursor)
        : Napi::AsyncWorker(env), deferred(Napi::Promise::Deferred::New(env)), rects(std::move(rects)),
          cursor(cursor) {
        auto buffers = Napi::Array::New(env, this->rects.size());

        for (size_t index = 0; index < this->rects.size(); index++) {
            auto const& rect = this->rects[index];
            auto buffer = Napi::Buffer<uint8_t>::New(env, static_cast<size_t>(rect.width) * rect.height * 4);
            buffers.Set(static_cast<uint32_t>(index), buffer);
            targets.push_back(buffer.Data());
        }

        output = Napi::Persistent(buffers);
    }

    Napi::Promise promise() {
        return deferred.Promise();
    }

protected:
    void Execute() override {
        if (!fileship::core::captureRects(rects, cursor, targets)) SetError("Could not capture the screen");
    }

    void OnOK() override {
        deferred.Resolve(output.Value());
    }

    void OnError(Napi::Error const& error) override {
        deferred.Reject(error.Value());
    }

private:
    Napi::Promise::Deferred deferred;
    std::vector<Rect> rects;
    bool cursor;
    std::vector<uint8_t*> targets;
    Napi::Reference<Napi::Array> output;
};

Napi::Value captureRects(Napi::CallbackInfo const& info) {
    auto env = info.Env();

    if (!info[0].IsArray()) throw Napi::TypeError::New(env, "Expected an array of rectangles");

    auto list = info[0].As<Napi::Array>();
    std::vector<Rect> rects;

    for (uint32_t index = 0; index < list.Length(); index++) {
        auto rect = readRect(list.Get(index).As<Napi::Object>());
        if (rect.width <= 0 || rect.height <= 0) throw Napi::RangeError::New(env, "Invalid capture area");
        rects.push_back(rect);
    }

    bool cursor = info[1].IsObject() && info[1].As<Napi::Object>().Get("cursor").ToBoolean().Value();

    auto worker = new CaptureWorker(env, std::move(rects), cursor);
    auto promise = worker->promise();
    worker->Queue();
    return promise;
}

}

void registerCapture(Napi::Env env, Napi::Object exports) {
    exports.Set("captureRects", Napi::Function::New(env, captureRects));
}

}

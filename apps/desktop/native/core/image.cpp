#include "common.h"

namespace native {

namespace {

using fileship::core::EncodeOptions;
using fileship::core::ImageFormat;
using fileship::core::PixelOrder;

class EncodeWorker : public Napi::AsyncWorker {
public:
    EncodeWorker(Napi::Env env, Napi::Buffer<uint8_t> input, uint32_t width, uint32_t height, EncodeOptions options)
        : Napi::AsyncWorker(env), deferred(Napi::Promise::Deferred::New(env)), pixels(input.Data()), width(width),
          height(height), options(options) {
        source = Napi::Persistent(input);
    }

    Napi::Promise promise() {
        return deferred.Promise();
    }

protected:
    void Execute() override {
        try {
            output = fileship::core::encodeImage(pixels, width, height, options);
        } catch (std::exception const& error) {
            SetError(error.what());
        }
    }

    void OnOK() override {
        deferred.Resolve(Napi::Buffer<uint8_t>::Copy(Env(), output.data(), output.size()));
    }

    void OnError(Napi::Error const& error) override {
        deferred.Reject(error.Value());
    }

private:
    Napi::Promise::Deferred deferred;
    Napi::Reference<Napi::Buffer<uint8_t>> source;
    uint8_t const* pixels;
    uint32_t width;
    uint32_t height;
    EncodeOptions options;
    std::vector<uint8_t> output;
};

Napi::Value encodeImage(Napi::CallbackInfo const& info) {
    auto env = info.Env();

    if (!info[0].IsBuffer()) throw Napi::TypeError::New(env, "Expected a pixel buffer");

    auto input = info[0].As<Napi::Buffer<uint8_t>>();
    auto width = info[1].ToNumber().Uint32Value();
    auto height = info[2].ToNumber().Uint32Value();

    if (!width || !height || input.Length() < static_cast<size_t>(width) * height * 4)
        throw Napi::RangeError::New(env, "The pixel buffer does not match the image size");

    EncodeOptions options;

    if (info[3].IsObject()) {
        auto object = info[3].As<Napi::Object>();
        if (object.Get("order").ToString().Utf8Value() == "rgba") options.order = PixelOrder::Rgba;
        if (object.Get("format").ToString().Utf8Value() == "jpeg") options.format = ImageFormat::Jpeg;
        if (object.Has("quality")) options.quality = object.Get("quality").ToNumber().FloatValue() / 100.0f;
        if (object.Has("maxWidth")) options.maxWidth = object.Get("maxWidth").ToNumber().Uint32Value();
    }

    auto worker = new EncodeWorker(env, input, width, height, options);
    auto promise = worker->promise();
    worker->Queue();
    return promise;
}

}

void registerImage(Napi::Env env, Napi::Object exports) {
    exports.Set("encodeImage", Napi::Function::New(env, encodeImage));
}

}

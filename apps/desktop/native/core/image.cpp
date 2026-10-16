#include "common.h"

#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

namespace native {

namespace {

struct EncodeOptions {
    bool isRgba = false;
    bool isJpeg = false;
    float quality = 0.9f;
    uint32_t maxWidth = 0;
};

void check(HRESULT result, char const* step) {
    if (FAILED(result)) throw std::runtime_error(std::string("Could not encode the image: ") + step);
}

struct ComScope {
    bool isInitialized;

    ComScope() : isInitialized(SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {}

    ~ComScope() {
        if (isInitialized) CoUninitialize();
    }
};

std::vector<uint8_t> encode(uint8_t const* pixels, uint32_t width, uint32_t height, EncodeOptions const& options) {
    ComScope com;

    ComPtr<IWICImagingFactory> factory;
    check(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)),
          "factory");

    ComPtr<IWICBitmap> bitmap;
    check(factory->CreateBitmapFromMemory(width, height,
                                          options.isRgba ? GUID_WICPixelFormat32bppRGBA
                                                         : GUID_WICPixelFormat32bppBGRA,
                                          width * 4, width * height * 4, const_cast<BYTE*>(pixels), &bitmap),
          "source");

    ComPtr<IWICBitmapSource> source = bitmap;
    uint32_t targetWidth = width;
    uint32_t targetHeight = height;

    if (options.maxWidth && width > options.maxWidth) {
        targetWidth = options.maxWidth;
        targetHeight = std::max<uint32_t>(1, static_cast<uint32_t>(std::lround(
                                                 static_cast<double>(height) * options.maxWidth / width)));

        ComPtr<IWICBitmapScaler> scaler;
        check(factory->CreateBitmapScaler(&scaler), "scaler");
        check(scaler->Initialize(source.Get(), targetWidth, targetHeight, WICBitmapInterpolationModeHighQualityCubic),
              "scale");
        source = scaler;
    }

    ComPtr<IStream> stream;
    check(CreateStreamOnHGlobal(nullptr, TRUE, &stream), "stream");

    ComPtr<IWICBitmapEncoder> encoder;
    check(factory->CreateEncoder(options.isJpeg ? GUID_ContainerFormatJpeg : GUID_ContainerFormatPng, nullptr,
                                 &encoder),
          "encoder");
    check(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache), "encoder");

    ComPtr<IWICBitmapFrameEncode> frame;
    ComPtr<IPropertyBag2> properties;
    check(encoder->CreateNewFrame(&frame, &properties), "frame");

    if (options.isJpeg) {
        PROPBAG2 option{};
        option.pstrName = const_cast<LPOLESTR>(L"ImageQuality");

        VARIANT value;
        VariantInit(&value);
        value.vt = VT_R4;
        value.fltVal = options.quality;
        check(properties->Write(1, &option, &value), "quality");
    }

    check(frame->Initialize(properties.Get()), "frame");
    check(frame->SetSize(targetWidth, targetHeight), "size");

    WICPixelFormatGUID format = options.isJpeg ? GUID_WICPixelFormat24bppBGR : GUID_WICPixelFormat32bppBGRA;
    check(frame->SetPixelFormat(&format), "format");

    ComPtr<IWICFormatConverter> converter;
    check(factory->CreateFormatConverter(&converter), "converter");
    check(converter->Initialize(source.Get(), format, WICBitmapDitherTypeNone, nullptr, 0,
                                WICBitmapPaletteTypeCustom),
          "convert");

    check(frame->WriteSource(converter.Get(), nullptr), "write");
    check(frame->Commit(), "commit");
    check(encoder->Commit(), "commit");

    HGLOBAL memory = nullptr;
    check(GetHGlobalFromStream(stream.Get(), &memory), "output");

    STATSTG stat{};
    check(stream->Stat(&stat, STATFLAG_NONAME), "output");

    auto size = static_cast<size_t>(stat.cbSize.QuadPart);
    auto data = static_cast<uint8_t const*>(GlobalLock(memory));
    std::vector<uint8_t> output(data, data + size);
    GlobalUnlock(memory);

    return output;
}

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
            output = encode(pixels, width, height, options);
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
        options.isRgba = object.Get("order").ToString().Utf8Value() == "rgba";
        options.isJpeg = object.Get("format").ToString().Utf8Value() == "jpeg";
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

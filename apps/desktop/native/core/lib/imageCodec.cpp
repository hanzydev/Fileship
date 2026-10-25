#include "core.h"

#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

namespace fileship::core {

namespace {

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

ComPtr<IWICImagingFactory> imagingFactory() {
    ComPtr<IWICImagingFactory> factory;
    check(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)),
          "factory");
    return factory;
}

std::optional<Bitmap> readFrame(IWICImagingFactory* factory, IWICBitmapDecoder* decoder) {
    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame))) return std::nullopt;

    ComPtr<IWICFormatConverter> converter;
    if (FAILED(factory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr,
                                     0, WICBitmapPaletteTypeCustom)))
        return std::nullopt;

    Bitmap bitmap;
    if (FAILED(converter->GetSize(&bitmap.width, &bitmap.height)) || !bitmap.width || !bitmap.height)
        return std::nullopt;

    bitmap.pixels.resize(static_cast<size_t>(bitmap.width) * bitmap.height * 4);
    if (FAILED(converter->CopyPixels(nullptr, bitmap.width * 4, static_cast<UINT>(bitmap.pixels.size()),
                                     bitmap.pixels.data())))
        return std::nullopt;

    return bitmap;
}

}

std::vector<uint8_t> encodeImage(uint8_t const* pixels, uint32_t width, uint32_t height,
                                 EncodeOptions const& options) {
    ComScope com;
    auto factory = imagingFactory();
    bool isJpeg = options.format == ImageFormat::Jpeg;

    ComPtr<IWICBitmap> bitmap;
    check(factory->CreateBitmapFromMemory(width, height,
                                          options.order == PixelOrder::Rgba ? GUID_WICPixelFormat32bppRGBA
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
    check(factory->CreateEncoder(isJpeg ? GUID_ContainerFormatJpeg : GUID_ContainerFormatPng, nullptr, &encoder),
          "encoder");
    check(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache), "encoder");

    ComPtr<IWICBitmapFrameEncode> frame;
    ComPtr<IPropertyBag2> properties;
    check(encoder->CreateNewFrame(&frame, &properties), "frame");

    if (isJpeg) {
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

    WICPixelFormatGUID format = isJpeg ? GUID_WICPixelFormat24bppBGR : GUID_WICPixelFormat32bppBGRA;
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

std::optional<Bitmap> decodeImage(uint8_t const* data, size_t size) {
    try {
        ComScope com;
        auto factory = imagingFactory();

        ComPtr<IWICStream> stream;
        if (FAILED(factory->CreateStream(&stream)) ||
            FAILED(stream->InitializeFromMemory(const_cast<BYTE*>(data), static_cast<DWORD>(size))))
            return std::nullopt;

        ComPtr<IWICBitmapDecoder> decoder;
        if (FAILED(factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand,
                                                    &decoder)))
            return std::nullopt;

        return readFrame(factory.Get(), decoder.Get());
    } catch (std::exception const&) {
        return std::nullopt;
    }
}

std::optional<Bitmap> decodeImageFile(std::filesystem::path const& path) {
    try {
        ComScope com;
        auto factory = imagingFactory();

        ComPtr<IWICBitmapDecoder> decoder;
        if (FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                                      WICDecodeMetadataCacheOnDemand, &decoder)))
            return std::nullopt;

        return readFrame(factory.Get(), decoder.Get());
    } catch (std::exception const&) {
        return std::nullopt;
    }
}

}

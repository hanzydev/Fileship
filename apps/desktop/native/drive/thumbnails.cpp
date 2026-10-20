#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <unknwn.h>

#include "com.h"
#include "http.h"
#include "thumbnails.h"

#include <shlobj.h>
#include <shlwapi.h>
#include <thumbcache.h>
#include <wincodec.h>

#include <algorithm>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "windowscodecs.lib")

using namespace winrt;

namespace drive {

CLSID const CLSID_FileshipThumbnails{0x9fdb7cd2, 0xc34c, 0x4340, {0xa8, 0x39, 0x34, 0xe6, 0x7c, 0x33, 0x35, 0xad}};

namespace {

constexpr int ORIENTATIONS[] = {
    WICBitmapTransformRotate0,
    WICBitmapTransformRotate0,
    WICBitmapTransformFlipHorizontal,
    WICBitmapTransformRotate180,
    WICBitmapTransformFlipVertical,
    WICBitmapTransformRotate90 | WICBitmapTransformFlipHorizontal,
    WICBitmapTransformRotate90,
    WICBitmapTransformRotate270 | WICBitmapTransformFlipHorizontal,
    WICBitmapTransformRotate270,
};

bool isMedia(std::wstring const& path) {
    PERCEIVED type = PERCEIVED_TYPE_UNSPECIFIED;
    PERCEIVEDFLAG flags = 0;
    AssocGetPerceivedType(PathFindExtensionW(path.c_str()), &type, &flags, nullptr);
    return type == PERCEIVED_TYPE_IMAGE || type == PERCEIVED_TYPE_VIDEO;
}

std::optional<std::wstring> fileIdOf(std::wstring const& path) {
    file_handle file(CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                 FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    if (!file) return std::nullopt;

    std::vector<BYTE> buffer(sizeof(CF_PLACEHOLDER_BASIC_INFO) + 1024);
    DWORD returned = 0;
    if (FAILED(CfGetPlaceholderInfo(file.get(), CF_PLACEHOLDER_INFO_BASIC, buffer.data(),
                                    static_cast<DWORD>(buffer.size()), &returned))) {
        return std::nullopt;
    }

    auto info = reinterpret_cast<CF_PLACEHOLDER_BASIC_INFO const*>(buffer.data());
    std::wstring identity(reinterpret_cast<wchar_t const*>(info->FileIdentity),
                          info->FileIdentityLength / sizeof(wchar_t));
    if (!identity.starts_with(L"f:") || identity.size() == 2) return std::nullopt;

    return identity.substr(2);
}

bool isLocal(std::wstring const& path) {
    DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
           !(attributes & (FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS | FILE_ATTRIBUTE_RECALL_ON_OPEN |
                           FILE_ATTRIBUTE_OFFLINE));
}

com_ptr<IWICImagingFactory> imaging() {
    return create_instance<IWICImagingFactory>(CLSID_WICImagingFactory);
}

WICBitmapTransformOptions orientationOf(IWICBitmapFrameDecode* frame) {
    com_ptr<IWICMetadataQueryReader> reader;
    if (FAILED(frame->GetMetadataQueryReader(reader.put()))) return WICBitmapTransformRotate0;

    PROPVARIANT value;
    PropVariantInit(&value);
    int transform = WICBitmapTransformRotate0;

    if (SUCCEEDED(reader->GetMetadataByName(L"System.Photo.Orientation", &value)) && value.vt == VT_UI2 &&
        value.uiVal < std::size(ORIENTATIONS)) {
        transform = ORIENTATIONS[value.uiVal];
    }

    PropVariantClear(&value);
    return static_cast<WICBitmapTransformOptions>(transform);
}

HBITMAP render(IWICImagingFactory* factory, IWICBitmapDecoder* decoder, UINT size, bool orient) {
    com_ptr<IWICBitmapFrameDecode> frame;
    check_hresult(decoder->GetFrame(0, frame.put()));
    com_ptr<IWICBitmapSource> source = frame;

    UINT width = 0;
    UINT height = 0;
    check_hresult(source->GetSize(&width, &height));

    double scale = std::min(1.0, static_cast<double>(size) / std::max(width, height));
    UINT targetWidth = std::max(1u, static_cast<UINT>(width * scale + 0.5));
    UINT targetHeight = std::max(1u, static_cast<UINT>(height * scale + 0.5));

    if (targetWidth != width || targetHeight != height) {
        com_ptr<IWICBitmapScaler> scaler;
        check_hresult(factory->CreateBitmapScaler(scaler.put()));
        check_hresult(scaler->Initialize(source.get(), targetWidth, targetHeight,
                                         WICBitmapInterpolationModeHighQualityCubic));
        source = scaler;
    }

    auto transform = orient ? orientationOf(frame.get()) : WICBitmapTransformRotate0;

    if (transform != WICBitmapTransformRotate0) {
        com_ptr<IWICBitmapFlipRotator> rotator;
        check_hresult(factory->CreateBitmapFlipRotator(rotator.put()));
        check_hresult(rotator->Initialize(source.get(), transform));
        source = rotator;
        check_hresult(source->GetSize(&targetWidth, &targetHeight));
    }

    com_ptr<IWICFormatConverter> converter;
    check_hresult(factory->CreateFormatConverter(converter.put()));
    check_hresult(converter->Initialize(source.get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone,
                                        nullptr, 0, WICBitmapPaletteTypeCustom));

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = static_cast<LONG>(targetWidth);
    info.bmiHeader.biHeight = -static_cast<LONG>(targetHeight);
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bitmap) throw_last_error();

    HRESULT result = converter->CopyPixels(nullptr, targetWidth * 4, targetWidth * targetHeight * 4,
                                           static_cast<BYTE*>(bits));
    if (FAILED(result)) {
        DeleteObject(bitmap);
        throw_hresult(result);
    }

    return bitmap;
}

HBITMAP fromFile(std::wstring const& path, UINT size) {
    file_handle file(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                 nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_NO_RECALL, nullptr));
    if (!file) throw_last_error();

    auto factory = imaging();
    com_ptr<IWICBitmapDecoder> decoder;
    check_hresult(factory->CreateDecoderFromFileHandle(reinterpret_cast<ULONG_PTR>(file.get()), nullptr,
                                                       WICDecodeMetadataCacheOnDemand, decoder.put()));
    return render(factory.get(), decoder.get(), size, true);
}

HBITMAP fromServer(std::wstring const& fileId, UINT size) {
    auto response = httpGet(L"/u/" + fileId + L"/thumbnail");
    if (response.status != 200 || response.body.empty()) throw_hresult(E_FAIL);

    com_ptr<IStream> stream;
    stream.attach(SHCreateMemStream(reinterpret_cast<BYTE const*>(response.body.data()),
                                    static_cast<UINT>(response.body.size())));
    if (!stream) throw_hresult(E_OUTOFMEMORY);

    auto factory = imaging();
    com_ptr<IWICBitmapDecoder> decoder;
    check_hresult(factory->CreateDecoderFromStream(stream.get(), nullptr, WICDecodeMetadataCacheOnDemand,
                                                   decoder.put()));
    return render(factory.get(), decoder.get(), size, false);
}

struct Thumbnails : implements<Thumbnails, IInitializeWithItem, IThumbnailProvider> {
    ServerLock lock;
    std::wstring path;

    HRESULT __stdcall Initialize(IShellItem* item, DWORD) noexcept final {
        wchar_t* name = nullptr;
        HRESULT result = item->GetDisplayName(SIGDN_FILESYSPATH, &name);
        if (FAILED(result)) return result;

        path = name;
        CoTaskMemFree(name);
        return S_OK;
    }

    HRESULT __stdcall GetThumbnail(UINT size, HBITMAP* bitmap, WTS_ALPHATYPE* alpha) noexcept final {
        *bitmap = nullptr;
        *alpha = WTSAT_ARGB;

        try {
            if (path.empty() || !isMedia(path)) return E_FAIL;

            auto fileId = fileIdOf(path);
            if (!fileId) return E_FAIL;

            if (isLocal(path)) {
                try {
                    *bitmap = fromFile(path, size);
                } catch (...) {
                }
            }

            if (!*bitmap) *bitmap = fromServer(*fileId, size);
            return S_OK;
        } catch (...) {
            return to_hresult();
        }
    }
};

}

com_ptr<IClassFactory> thumbnailFactory() {
    return classFactory<Thumbnails>();
}

}

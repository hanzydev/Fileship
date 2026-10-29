#include "core.h"

#include <d3d11.h>
#include <dxgi1_5.h>
#include <wrl/client.h>

#include <cstring>
#include <thread>

using Microsoft::WRL::ComPtr;

namespace fileship::core {

namespace {

bool duplicateOutput(DisplayOutput const& output, Rect const& rect, uint8_t* pixels) {
    auto part = intersectRects(rect, output.rect);
    if (!part) return true;

    ComPtr<IDXGIFactory1> factory;
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIOutput> baseOutput;
    ComPtr<IDXGIOutput5> dxgiOutput;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))) ||
        FAILED(factory->EnumAdapters1(output.adapter, &adapter)) ||
        FAILED(adapter->EnumOutputs(output.output, &baseOutput)) || FAILED(baseOutput.As(&dxgiOutput)))
        return false;

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    if (FAILED(D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                 nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context)))
        return false;

    DXGI_FORMAT formats[] = {DXGI_FORMAT_B8G8R8A8_UNORM};
    ComPtr<IDXGIOutputDuplication> duplication;
    if (FAILED(dxgiOutput->DuplicateOutput1(device.Get(), 0, 1, formats, &duplication))) return false;

    DXGI_OUTDUPL_DESC desc{};
    duplication->GetDesc(&desc);
    if (desc.Rotation != DXGI_MODE_ROTATION_IDENTITY && desc.Rotation != DXGI_MODE_ROTATION_UNSPECIFIED)
        return false;

    DXGI_OUTDUPL_FRAME_INFO info{};
    ComPtr<IDXGIResource> resource;

    for (int attempt = 0; !info.LastPresentTime.QuadPart; attempt++) {
        if (resource) {
            resource.Reset();
            duplication->ReleaseFrame();
        }

        if (attempt == 5 || FAILED(duplication->AcquireNextFrame(100, &info, &resource))) return false;
    }

    ComPtr<ID3D11Texture2D> frame;
    bool isCaptured = SUCCEEDED(resource.As(&frame));

    D3D11_TEXTURE2D_DESC textureDesc{};
    ComPtr<ID3D11Texture2D> staging;

    if (isCaptured) {
        frame->GetDesc(&textureDesc);
        textureDesc.Usage = D3D11_USAGE_STAGING;
        textureDesc.BindFlags = 0;
        textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        textureDesc.MiscFlags = 0;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        isCaptured = SUCCEEDED(device->CreateTexture2D(&textureDesc, nullptr, &staging));
    }

    if (isCaptured) context->CopyResource(staging.Get(), frame.Get());
    duplication->ReleaseFrame();

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (!isCaptured || FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return false;

    int sourceX = part->x - output.rect.x;
    int sourceY = part->y - output.rect.y;
    isCaptured = sourceX + part->width <= static_cast<int>(textureDesc.Width) &&
                 sourceY + part->height <= static_cast<int>(textureDesc.Height);

    if (isCaptured) {
        for (int row = 0; row < part->height; row++) {
            auto source = static_cast<uint8_t const*>(mapped.pData) +
                          static_cast<size_t>(sourceY + row) * mapped.RowPitch + static_cast<size_t>(sourceX) * 4;
            auto target = pixels + (static_cast<size_t>(part->y - rect.y + row) * rect.width + (part->x - rect.x)) * 4;
            std::memcpy(target, source, static_cast<size_t>(part->width) * 4);
        }
    }

    context->Unmap(staging.Get(), 0);
    return isCaptured;
}

bool duplicateRect(Rect const& rect, uint8_t* pixels) {
    auto outputs = listOutputs();
    if (outputs.empty()) return false;

    std::memset(pixels, 0, static_cast<size_t>(rect.width) * rect.height * 4);

    std::vector<char> results(outputs.size(), 0);
    std::vector<std::thread> threads;

    for (size_t index = 0; index < outputs.size(); index++) {
        threads.emplace_back([&, index] { results[index] = duplicateOutput(outputs[index], rect, pixels); });
    }

    for (auto& thread : threads) thread.join();

    for (char result : results) {
        if (!result) return false;
    }

    return true;
}

void drawCursor(HDC context, Rect const& origin) {
    CURSORINFO info{};
    info.cbSize = sizeof(info);
    if (!GetCursorInfo(&info) || !(info.flags & CURSOR_SHOWING) || !info.hCursor) return;

    ICONINFO icon{};
    bool hasIcon = GetIconInfo(info.hCursor, &icon) != FALSE;

    DrawIconEx(context, info.ptScreenPos.x - origin.x - (hasIcon ? static_cast<int>(icon.xHotspot) : 0),
               info.ptScreenPos.y - origin.y - (hasIcon ? static_cast<int>(icon.yHotspot) : 0), info.hCursor,
               0, 0, 0, nullptr, DI_NORMAL);

    if (hasIcon) {
        if (icon.hbmMask) DeleteObject(icon.hbmMask);
        if (icon.hbmColor) DeleteObject(icon.hbmColor);
    }
}

BITMAPINFO bitmapInfo(Rect const& rect) {
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = rect.width;
    info.bmiHeader.biHeight = -rect.height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    return info;
}

void drawCursorOnto(Rect const& rect, uint8_t* pixels) {
    auto info = bitmapInfo(rect);
    size_t length = static_cast<size_t>(rect.width) * rect.height * 4;

    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bitmap) return;

    HDC memory = CreateCompatibleDC(nullptr);
    HGDIOBJ previous = SelectObject(memory, bitmap);
    std::memcpy(bits, pixels, length);
    drawCursor(memory, rect);
    GdiFlush();
    std::memcpy(pixels, bits, length);

    SelectObject(memory, previous);
    DeleteDC(memory);
    DeleteObject(bitmap);
}

bool blitRect(Rect const& rect, bool cursor, uint8_t* pixels) {
    HDC screen = GetDC(nullptr);
    HDC memory = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateCompatibleBitmap(screen, rect.width, rect.height);
    HGDIOBJ previous = SelectObject(memory, bitmap);

    bool isCaptured = BitBlt(memory, 0, 0, rect.width, rect.height, screen, rect.x, rect.y,
                             SRCCOPY | CAPTUREBLT) != FALSE;

    if (isCaptured && cursor) drawCursor(memory, rect);

    SelectObject(memory, previous);

    auto info = bitmapInfo(rect);
    if (isCaptured)
        isCaptured = GetDIBits(memory, bitmap, 0, rect.height, pixels, &info, DIB_RGB_COLORS) == rect.height;

    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(nullptr, screen);

    return isCaptured;
}

}

bool captureRectInto(Rect const& rect, bool cursor, uint8_t* pixels) {
    bool isCaptured = duplicateRect(rect, pixels);
    if (isCaptured && cursor) drawCursorOnto(rect, pixels);
    if (!isCaptured && !blitRect(rect, cursor, pixels)) return false;

    size_t length = static_cast<size_t>(rect.width) * rect.height * 4;
    for (size_t index = 3; index < length; index += 4) pixels[index] = 255;

    return true;
}

bool captureRects(std::vector<Rect> const& rects, bool cursor, std::vector<uint8_t*> const& targets) {
    std::vector<char> results(rects.size(), 0);
    std::vector<std::thread> threads;

    for (size_t index = 0; index < rects.size(); index++) {
        threads.emplace_back([&, index] { results[index] = captureRectInto(rects[index], cursor, targets[index]); });
    }

    for (auto& thread : threads) thread.join();

    for (char result : results) {
        if (!result) return false;
    }

    return true;
}

std::optional<Bitmap> captureRect(Rect const& rect, bool cursor) {
    if (rect.width <= 0 || rect.height <= 0) return std::nullopt;

    Bitmap bitmap;
    bitmap.width = static_cast<uint32_t>(rect.width);
    bitmap.height = static_cast<uint32_t>(rect.height);
    bitmap.pixels.resize(static_cast<size_t>(rect.width) * rect.height * 4);

    if (!captureRectInto(rect, cursor, bitmap.pixels.data())) return std::nullopt;
    return bitmap;
}

}

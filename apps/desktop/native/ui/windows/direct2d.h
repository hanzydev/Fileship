#pragma once

#include <ui/gfx.h>

#include <windows.h>

#include <d2d1_3.h>
#include <d3d11.h>
#include <dcomp.h>
#include <dwrite_3.h>
#include <dxgi1_3.h>

#include <winrt/base.h>

namespace fileship::ui {

using winrt::com_ptr;

struct Gpu {
    com_ptr<ID3D11Device> d3d;
    com_ptr<IDXGIDevice> dxgi;
    com_ptr<ID2D1Device6> device;
    com_ptr<ID2D1DeviceContext5> context;
    com_ptr<IDCompositionDevice> composition;

    static std::unique_ptr<Gpu> create(Resources& resources);
};

class SwapSurface {
public:
    bool create(Gpu& gpu, HWND window, int width, int height, bool transparent);
    ID2D1DeviceContext5* begin(Gpu& gpu);
    void present(Gpu& gpu);
    void release();

private:
    com_ptr<IDXGISwapChain1> chain;
    com_ptr<IDCompositionTarget> target;
    com_ptr<IDCompositionVisual> visual;
    com_ptr<ID2D1Bitmap1> buffer;
    bool committed = false;
};

std::unique_ptr<Painter> createPainter(ID2D1DeviceContext* context, Resources& resources);
Gpu& gpuOf(Device& device);
ID2D1Factory7* factoryOf(Resources& resources);

}

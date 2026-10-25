#include "core.h"

#include <dxgi1_2.h>
#include <shellscalingapi.h>
#include <wrl/client.h>

#include <algorithm>

#pragma comment(lib, "shcore.lib")

using Microsoft::WRL::ComPtr;

namespace fileship::core {

namespace {

int64_t monitorId(wchar_t const* device) {
    uint32_t hash = 2166136261u;

    for (auto character = device; *character; character++) {
        hash ^= static_cast<uint32_t>(*character);
        hash *= 16777619u;
    }

    return hash;
}

BOOL CALLBACK handleMonitor(HMONITOR monitor, HDC, LPRECT, LPARAM parameter) {
    auto& displays = *reinterpret_cast<std::vector<DisplayInfo>*>(parameter);

    MONITORINFOEXW info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(monitor, &info)) return TRUE;

    UINT dpiX = 96;
    UINT dpiY = 96;
    GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

    auto const& rect = info.rcMonitor;
    displays.push_back(DisplayInfo{
        monitorId(info.szDevice),
        Rect{rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top},
        dpiX / 96.0,
        (info.dwFlags & MONITORINFOF_PRIMARY) != 0,
    });

    return TRUE;
}

}

std::vector<DisplayOutput> listOutputs() {
    std::vector<DisplayOutput> result;

    ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return result;

    ComPtr<IDXGIAdapter1> adapter;

    for (UINT adapterIndex = 0; factory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND;
         adapterIndex++) {
        ComPtr<IDXGIOutput> output;

        for (UINT outputIndex = 0; adapter->EnumOutputs(outputIndex, &output) != DXGI_ERROR_NOT_FOUND;
             outputIndex++) {
            DXGI_OUTPUT_DESC desc{};

            if (SUCCEEDED(output->GetDesc(&desc))) {
                auto const& coordinates = desc.DesktopCoordinates;
                result.push_back(DisplayOutput{adapterIndex, outputIndex,
                                               Rect{coordinates.left, coordinates.top,
                                                    coordinates.right - coordinates.left,
                                                    coordinates.bottom - coordinates.top}});
            }

            output.Reset();
        }

        adapter.Reset();
    }

    return result;
}

std::vector<DisplayInfo> listDisplays() {
    std::vector<DisplayInfo> displays;
    EnumDisplayMonitors(nullptr, nullptr, handleMonitor, reinterpret_cast<LPARAM>(&displays));
    return displays;
}

Rect virtualScreen() {
    int left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    return Rect{left, top, GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN)};
}

DisplayInfo displayNearest(POINT point) {
    HMONITOR monitor = MonitorFromPoint(point, MONITOR_DEFAULTTONEAREST);
    std::vector<DisplayInfo> displays;
    handleMonitor(monitor, nullptr, nullptr, reinterpret_cast<LPARAM>(&displays));
    return displays.empty() ? DisplayInfo{0, virtualScreen(), 1, true} : displays[0];
}

std::optional<Rect> intersectRects(Rect const& a, Rect const& b) {
    int x = std::max(a.x, b.x);
    int y = std::max(a.y, b.y);
    int right = std::min(a.x + a.width, b.x + b.width);
    int bottom = std::min(a.y + a.height, b.y + b.height);

    if (right <= x || bottom <= y) return std::nullopt;
    return Rect{x, y, right - x, bottom - y};
}

}

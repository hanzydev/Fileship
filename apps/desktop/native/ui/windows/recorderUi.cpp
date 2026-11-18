#include "d2d.h"

#include <ui/fixture.h>
#include <ui/recorderToolbar.h>
#include <ui/uiThread.h>

#include <shared/i18n.h>
#include <shared/log.h>

#include <shellscalingapi.h>
#include <wincodec.h>
#include <windowsx.h>

#include <algorithm>
#include <array>
#include <cmath>

#pragma comment(lib, "shcore.lib")

namespace fileship::ui {

namespace {

constexpr float BORDER_PADDING = 3;
constexpr float BORDER_WIDTH = 2;
constexpr UINT_PTR FADE_TIMER = 1;
constexpr UINT_PTR FRAME_TIMER = 2;

struct MonitorPart {
    RECT bounds{};
    RECT work{};
    double scale = 1;
    RECT part{};
    std::wstring sides;
};

struct LayeredWindow {
    HWND window = nullptr;
    int width = 0;
    int height = 0;
    HDC memory = nullptr;
    HBITMAP bitmap = nullptr;
    HGDIOBJ previous = nullptr;
    com_ptr<ID2D1DCRenderTarget> target;
    BYTE opacity = 0;
    POINT position{};

    bool create(D2DResources& resources, RECT const& rect, DWORD exStyle, void* owner) {
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
        position = {rect.left, rect.top};
        if (width <= 0 || height <= 0) return false;

        window = CreateWindowExW(exStyle, L"FileshipRecorder", L"Fileship", WS_POPUP, rect.left, rect.top, width, height,
                                 nullptr, nullptr, GetModuleHandleW(nullptr), owner);
        if (!window) return false;

        SetWindowDisplayAffinity(window, WDA_EXCLUDEFROMCAPTURE);

        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(info.bmiHeader);
        info.bmiHeader.biWidth = width;
        info.bmiHeader.biHeight = -height;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;

        void* bits = nullptr;
        HDC screen = GetDC(nullptr);
        memory = CreateCompatibleDC(screen);
        bitmap = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
        ReleaseDC(nullptr, screen);
        if (!memory || !bitmap) return false;
        previous = SelectObject(memory, bitmap);

        auto properties = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_SOFTWARE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96, 96);
        winrt::check_hresult(resources.factory()->CreateDCRenderTarget(&properties, target.put()));
        target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        return true;
    }

    template <typename Draw> void draw(D2DResources& resources, Draw&& paint) {
        RECT area{0, 0, width, height};
        winrt::check_hresult(target->BindDC(memory, &area));
        auto context = target.as<ID2D1DeviceContext>();

        target->BeginDraw();
        target->SetTransform(D2D1::Matrix3x2F::Identity());
        target->Clear(D2D1::ColorF(0, 0, 0, 0));
        D2DPainter painter(context.get(), resources);
        paint(painter);
        target->EndDraw();
        present();
    }

    void present() {
        SIZE size{width, height};
        POINT origin{0, 0};
        BLENDFUNCTION blend{AC_SRC_OVER, 0, opacity, AC_SRC_ALPHA};
        HDC screen = GetDC(nullptr);
        UpdateLayeredWindow(window, screen, &position, &size, memory, &origin, 0, &blend, ULW_ALPHA);
        ReleaseDC(nullptr, screen);
    }

    void destroy() {
        target = nullptr;
        if (memory) {
            SelectObject(memory, previous);
            DeleteDC(memory);
            memory = nullptr;
        }
        if (bitmap) {
            DeleteObject(bitmap);
            bitmap = nullptr;
        }
        if (window) {
            SetWindowLongPtrW(window, GWLP_USERDATA, 0);
            DestroyWindow(window);
            window = nullptr;
        }
    }
};

class RecorderUi {
public:
    RecorderUi(ScreenRect area, ThemeMode theme, RecorderUiCallbacks value, bool showBorder)
        : resources(D2DResources::acquire()), onVisible(value.onVisible), bar(*resources, theme, std::move(value)) {
        bar.state.phase = RecorderPhase::Preparing;

        static bool isRegistered = false;
        if (!isRegistered) {
            WNDCLASSW windowClass{};
            windowClass.lpfnWndProc = handleMessage;
            windowClass.hInstance = GetModuleHandleW(nullptr);
            windowClass.lpszClassName = L"FileshipRecorder";
            windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            RegisterClassW(&windowClass);
            isRegistered = true;
        }

        auto parts = monitorParts(area);
        MonitorPart const* host = nullptr;
        for (auto const& part : parts) {
            auto areaOf = [](RECT const& rect) {
                return static_cast<int64_t>(rect.right - rect.left) * (rect.bottom - rect.top);
            };
            if (!host || areaOf(part.part) > areaOf(host->part)) host = &part;
        }

        if (showBorder) {
            for (auto const& part : parts) createBorders(part);
        }

        createToolbar(host, area);
    }

    RecorderUi(ThemeMode theme, RecorderViewState const& value, double factor)
        : resources(D2DResources::acquire()), bar(*resources, theme, {}) {
        bar.state = value;
        bar.scale = factor;
    }

    std::vector<uint8_t> renderImage(int& width, int& height) {
        width = static_cast<int>(std::round(RECORDER_WIDTH * bar.scale));
        height = static_cast<int>(std::round(RECORDER_HEIGHT * bar.scale));

        com_ptr<IWICImagingFactory> wic;
        winrt::check_hresult(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                              IID_PPV_ARGS(wic.put())));
        com_ptr<IWICBitmap> bitmap;
        winrt::check_hresult(wic->CreateBitmap(width, height, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad,
                                               bitmap.put()));

        auto properties = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_SOFTWARE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96, 96);
        com_ptr<ID2D1RenderTarget> target;
        winrt::check_hresult(resources->factory()->CreateWicBitmapRenderTarget(bitmap.get(), properties, target.put()));
        auto context = target.as<ID2D1DeviceContext>();

        target->BeginDraw();
        target->Clear(D2D1::ColorF(0, 0, 0, 0));
        D2DPainter painter(context.get(), *resources);
        bar.paint(painter, nowMs());
        winrt::check_hresult(target->EndDraw());

        std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
        WICRect rect{0, 0, width, height};
        winrt::check_hresult(bitmap->CopyPixels(&rect, width * 4, static_cast<UINT>(pixels.size()), pixels.data()));
        return pixels;
    }

    ~RecorderUi() {
        toolbar.destroy();
        for (auto& border : borders) border.destroy();
    }

    void update(RecorderViewState const& value) {
        bar.state = value;
        render();
    }

private:
    static std::vector<MonitorPart> monitorParts(ScreenRect const& area) {
        std::vector<MonitorPart> parts;
        std::pair<RECT, std::vector<MonitorPart>*> context{{area.x, area.y, area.x + area.width, area.y + area.height},
                                                           &parts};

        EnumDisplayMonitors(
            nullptr, nullptr,
            [](HMONITOR monitor, HDC, LPRECT, LPARAM data) -> BOOL {
                auto& context = *reinterpret_cast<std::pair<RECT, std::vector<MonitorPart>*>*>(data);
                MONITORINFO info{sizeof(info)};
                if (!GetMonitorInfoW(monitor, &info)) return TRUE;

                RECT part{};
                if (!IntersectRect(&part, &info.rcMonitor, &context.first)) return TRUE;

                UINT dpiX = 96;
                UINT dpiY = 96;
                GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

                MonitorPart item;
                item.bounds = info.rcMonitor;
                item.work = info.rcWork;
                item.scale = dpiX / 96.0;
                item.part = part;
                auto const& rect = context.first;
                if (part.top == rect.top) item.sides += L't';
                if (part.right == rect.right) item.sides += L'r';
                if (part.bottom == rect.bottom) item.sides += L'b';
                if (part.left == rect.left) item.sides += L'l';
                context.second->push_back(item);
                return TRUE;
            },
            reinterpret_cast<LPARAM>(&context));

        return parts;
    }

    void createBorders(MonitorPart const& part) {
        if (part.sides.empty()) return;

        auto pad = [&](wchar_t side) {
            return part.sides.find(side) != std::wstring::npos ? static_cast<LONG>(std::lround(BORDER_PADDING * part.scale))
                                                               : 0;
        };
        LONG left = std::max(part.part.left - pad(L'l'), part.bounds.left);
        LONG top = std::max(part.part.top - pad(L't'), part.bounds.top);
        LONG right = std::min(part.part.right + pad(L'r'), part.bounds.right);
        LONG bottom = std::min(part.part.bottom + pad(L'b'), part.bounds.bottom);
        LONG width = std::max(1L, static_cast<LONG>(std::lround(BORDER_WIDTH * part.scale)));

        for (wchar_t side : part.sides) {
            RECT rect{left, top, right, bottom};
            if (side == L't') rect.bottom = top + width;
            if (side == L'b') rect.top = bottom - width;
            if (side == L'l') rect.right = left + width;
            if (side == L'r') rect.left = right - width;

            LayeredWindow border;
            DWORD style = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
            if (!border.create(*resources, rect, style, nullptr)) {
                border.destroy();
                continue;
            }

            bool isHorizontal = side == L't' || side == L'b';
            float dash = static_cast<float>(width * 3);
            auto tint = bar.borderColor();
            int length = isHorizontal ? border.width : border.height;
            float offset = isHorizontal ? static_cast<float>(rect.left - left) : static_cast<float>(rect.top - top);

            border.draw(*resources, [&](Painter& painter) {
                for (float start = -std::fmod(offset, dash * 2); start < length; start += dash * 2) {
                    float end = std::min(static_cast<float>(length), start + dash);
                    float begin = std::max(0.0f, start);
                    if (end <= begin) continue;
                    painter.fillRect(isHorizontal ? RectF(begin, 0, end, static_cast<float>(border.height))
                                                  : RectF(0, begin, static_cast<float>(border.width), end),
                                     tint);
                }
            });

            ShowWindow(border.window, SW_SHOWNOACTIVATE);
            borders.push_back(border);
        }
    }

    void createToolbar(MonitorPart const* host, ScreenRect const& area) {
        RECT part = host ? host->part : RECT{area.x, area.y, area.x + area.width, area.y + area.height};
        RECT work = host ? host->work : part;
        if (!host) {
            HMONITOR monitor = MonitorFromRect(&part, MONITOR_DEFAULTTONEAREST);
            MONITORINFO info{sizeof(info)};
            GetMonitorInfoW(monitor, &info);
            work = info.rcWork;
            UINT dpiX = 96;
            UINT dpiY = 96;
            GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
            bar.scale = dpiX / 96.0;
        } else {
            bar.scale = host->scale;
        }

        double scale = bar.scale;

        double width = std::round(RECORDER_WIDTH * scale);
        double height = std::round(RECORDER_HEIGHT * scale);
        double margin = 8 * scale;
        double centerX = (part.left + part.right) / 2.0;
        double x = std::round(std::min(std::max(centerX - width / 2, static_cast<double>(work.left)), work.right - width));
        double y = part.bottom + margin;
        if (y + height > work.bottom) y = part.top - height - margin;
        if (y < work.top) y = part.bottom - height - margin;

        RECT rect{static_cast<LONG>(x), static_cast<LONG>(std::round(y)), static_cast<LONG>(x + width),
                  static_cast<LONG>(std::round(y) + height)};
        DWORD style = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
        if (!toolbar.create(*resources, rect, style, this)) return;

        render();
        ShowWindow(toolbar.window, SW_SHOWNOACTIVATE);
        SetTimer(toolbar.window, FADE_TIMER, 16, nullptr);
        SetTimer(toolbar.window, FRAME_TIMER, 16, nullptr);
        if (onVisible) onVisible();
    }

    void render() {
        if (!toolbar.window) return;

        double now = nowMs();
        toolbar.draw(*resources, [&](Painter& painter) { bar.paint(painter, now); });
    }

    static LRESULT CALLBACK handleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == WM_NCCREATE) {
            auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        }

        auto self = reinterpret_cast<RecorderUi*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (!self) return DefWindowProcW(window, message, wParam, lParam);
        return self->handle(window, message, wParam, lParam);
    }

    LRESULT handle(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case WM_TIMER:
            if (wParam == FADE_TIMER) {
                for (auto* layered : allWindows()) {
                    layered->opacity = static_cast<BYTE>(std::min(255, layered->opacity + 51));
                    layered->present();
                }
                if (toolbar.opacity >= 255) KillTimer(window, FADE_TIMER);
                return 0;
            }
            if (wParam == FRAME_TIMER) {
                double now = nowMs();
                if (bar.isAnimating(now) || now - lastFrame >= 250) {
                    lastFrame = now;
                    render();
                }
                return 0;
            }
            return 0;
        case WM_MOUSEMOVE: {
            if (!isTracking) {
                TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, window, 0};
                TrackMouseEvent(&track);
                isTracking = true;
            }
            bar.pointerMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            render();
            return 0;
        }
        case WM_MOUSELEAVE:
            isTracking = false;
            bar.pointerLeave();
            render();
            return 0;
        case WM_LBUTTONDOWN:
            SetCapture(window);
            bar.pointerDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            render();
            return 0;
        case WM_LBUTTONUP:
            ReleaseCapture();
            bar.pointerUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            render();
            return 0;
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_NCHITTEST: {
            POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(window, &point);
            return bar.contains(point.x, point.y) ? HTCLIENT : HTTRANSPARENT;
        }
        default:
            break;
        }

        return DefWindowProcW(window, message, wParam, lParam);
    }

    std::vector<LayeredWindow*> allWindows() {
        std::vector<LayeredWindow*> result{&toolbar};
        for (auto& border : borders) result.push_back(&border);
        return result;
    }

    std::shared_ptr<D2DResources> resources;
    std::function<void()> onVisible;
    RecorderToolbar bar;
    LayeredWindow toolbar;
    std::vector<LayeredWindow> borders;
    bool isTracking = false;
    double lastFrame = 0;
};

std::unique_ptr<RecorderUi> current;

}

FixtureImage renderRecorderFixture(RecorderViewState const& state, ThemeMode theme, double scale) {
    RecorderUi view(theme, state, scale);
    FixtureImage image;
    image.bgra = view.renderImage(image.width, image.height);
    return image;
}

void showRecorderUi(ScreenRect area, ThemeMode theme, RecorderUiCallbacks callbacks, bool showBorder) {
    runOnUi([area, theme, callbacks = std::move(callbacks), showBorder]() mutable {
        current.reset();
        try {
            current = std::make_unique<RecorderUi>(area, theme, std::move(callbacks), showBorder);
        } catch (winrt::hresult_error const& error) {
            logError(L"Could not show the recorder toolbar: " + std::wstring(error.message()));
            current.reset();
        }
    });
}

void updateRecorderUi(RecorderViewState const& state) {
    runOnUi([state] {
        if (current) current->update(state);
    });
}

void hideRecorderUi() {
    runOnUi([] { current.reset(); });
}

}

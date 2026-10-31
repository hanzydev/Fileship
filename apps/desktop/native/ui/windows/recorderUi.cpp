#include "direct2d.h"

#include <ui/recorderToolbar.h>
#include <ui/uiThread.h>

#include <shared/log.h>

#include <shellscalingapi.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>

#pragma comment(lib, "shcore.lib")

namespace fileship::ui {

namespace {

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

    bool create(Resources& resources, RECT const& rect, DWORD exStyle, void* owner) {
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
        winrt::check_hresult(factoryOf(resources)->CreateDCRenderTarget(&properties, target.put()));
        target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        return true;
    }

    template <typename Draw> void draw(Resources& resources, Draw&& paint) {
        RECT area{0, 0, width, height};
        winrt::check_hresult(target->BindDC(memory, &area));
        auto context = target.as<ID2D1DeviceContext>();

        target->BeginDraw();
        target->SetTransform(D2D1::Matrix3x2F::Identity());
        target->Clear(D2D1::ColorF(0, 0, 0, 0));
        {
            auto painter = createPainter(context.get(), resources);
            paint(*painter);
        }
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
    RecorderUi(ScreenRect area, ThemeMode theme, RecorderUiCallbacks callbacks, bool showBorder)
        : toolbar(theme, std::move(callbacks)) {
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

    ~RecorderUi() {
        window.destroy();
        for (auto& border : borders) border.destroy();
    }

    void update(RecorderViewState const& value) {
        toolbar.setState(value);
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
            return part.sides.find(side) != std::wstring::npos
                       ? static_cast<LONG>(std::lround(RECORDER_BORDER_PADDING * part.scale))
                       : 0;
        };
        LONG left = std::max(part.part.left - pad(L'l'), part.bounds.left);
        LONG top = std::max(part.part.top - pad(L't'), part.bounds.top);
        LONG right = std::min(part.part.right + pad(L'r'), part.bounds.right);
        LONG bottom = std::min(part.part.bottom + pad(L'b'), part.bounds.bottom);
        LONG width = std::max(1L, static_cast<LONG>(std::lround(RECORDER_BORDER_WIDTH * part.scale)));

        for (wchar_t side : part.sides) {
            RECT rect{left, top, right, bottom};
            if (side == L't') rect.bottom = top + width;
            if (side == L'b') rect.top = bottom - width;
            if (side == L'l') rect.right = left + width;
            if (side == L'r') rect.left = right - width;

            LayeredWindow border;
            DWORD style = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
            if (!border.create(toolbar.resources(), rect, style, nullptr)) {
                border.destroy();
                continue;
            }

            bool isHorizontal = side == L't' || side == L'b';
            auto tint = color(toolbar.palette().theme.destructive);
            float offset = isHorizontal ? static_cast<float>(rect.left - left) : static_cast<float>(rect.top - top);

            border.draw(toolbar.resources(), [&](Painter& painter) {
                paintRecorderBorder(painter, tint, isHorizontal, border.width, border.height, offset,
                                    static_cast<float>(width * 3));
            });

            ShowWindow(border.window, SW_SHOWNOACTIVATE);
            borders.push_back(border);
        }
    }

    void createToolbar(MonitorPart const* host, ScreenRect const& area) {
        RECT part = host ? host->part : RECT{area.x, area.y, area.x + area.width, area.y + area.height};
        RECT work = host ? host->work : part;
        double scale = 1;
        if (!host) {
            HMONITOR monitor = MonitorFromRect(&part, MONITOR_DEFAULTTONEAREST);
            MONITORINFO info{sizeof(info)};
            GetMonitorInfoW(monitor, &info);
            work = info.rcWork;
            UINT dpiX = 96;
            UINT dpiY = 96;
            GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
            scale = dpiX / 96.0;
        } else {
            scale = host->scale;
        }
        toolbar.setScale(scale);

        double width = std::round(RECORDER_TOOLBAR_WIDTH * scale);
        double height = std::round(RECORDER_TOOLBAR_HEIGHT * scale);
        double margin = 8 * scale;
        double centerX = (part.left + part.right) / 2.0;
        double x = std::round(std::min(std::max(centerX - width / 2, static_cast<double>(work.left)), work.right - width));
        double y = part.bottom + margin;
        if (y + height > work.bottom) y = part.top - height - margin;
        if (y < work.top) y = part.bottom - height - margin;

        RECT rect{static_cast<LONG>(x), static_cast<LONG>(std::round(y)), static_cast<LONG>(x + width),
                  static_cast<LONG>(std::round(y) + height)};
        DWORD style = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
        if (!window.create(toolbar.resources(), rect, style, this)) return;

        render();
        ShowWindow(window.window, SW_SHOWNOACTIVATE);
        SetTimer(window.window, FADE_TIMER, 16, nullptr);
        SetTimer(window.window, FRAME_TIMER, 16, nullptr);
        toolbar.notifyVisible();
    }

    void render() {
        if (!window.window) return;

        double now = nowMs();
        window.draw(toolbar.resources(), [&](Painter& painter) { toolbar.paint(painter, now); });
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

    LRESULT handle(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) {
        double x = GET_X_LPARAM(lParam);
        double y = GET_Y_LPARAM(lParam);

        switch (message) {
        case WM_TIMER:
            if (wParam == FADE_TIMER) {
                for (auto* layered : allWindows()) {
                    layered->opacity = static_cast<BYTE>(std::min(255, layered->opacity + 51));
                    layered->present();
                }
                if (window.opacity >= 255) KillTimer(handle, FADE_TIMER);
                return 0;
            }
            if (wParam == FRAME_TIMER) {
                double now = nowMs();
                if (toolbar.isAnimating(now) || now - lastFrame >= 250) {
                    lastFrame = now;
                    render();
                }
                return 0;
            }
            return 0;
        case WM_MOUSEMOVE: {
            if (!isTracking) {
                TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, handle, 0};
                TrackMouseEvent(&track);
                isTracking = true;
            }
            toolbar.pointerMove(x, y);
            render();
            return 0;
        }
        case WM_MOUSELEAVE:
            isTracking = false;
            toolbar.pointerLeave();
            render();
            return 0;
        case WM_LBUTTONDOWN:
            SetCapture(handle);
            toolbar.pointerDown(x, y);
            render();
            return 0;
        case WM_LBUTTONUP:
            ReleaseCapture();
            toolbar.pointerUp(x, y);
            render();
            return 0;
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_NCHITTEST: {
            POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(handle, &point);
            return toolbar.contains(point.x, point.y) ? HTCLIENT : HTTRANSPARENT;
        }
        default:
            break;
        }

        return DefWindowProcW(handle, message, wParam, lParam);
    }

    std::vector<LayeredWindow*> allWindows() {
        std::vector<LayeredWindow*> result{&window};
        for (auto& border : borders) result.push_back(&border);
        return result;
    }

    RecorderToolbar toolbar;
    LayeredWindow window;
    std::vector<LayeredWindow> borders;
    bool isTracking = false;
    double lastFrame = 0;
};

std::unique_ptr<RecorderUi> current;

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

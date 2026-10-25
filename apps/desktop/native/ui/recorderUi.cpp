#include "recorderUi.h"

#include "fixture.h"
#include "gfx.h"
#include "uiThread.h"
#include "widgets.h"

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

constexpr float TOOLBAR_WIDTH = 440;
constexpr float TOOLBAR_HEIGHT = 64;
constexpr float BORDER_PADDING = 3;
constexpr float BORDER_WIDTH = 2;
constexpr UINT_PTR FADE_TIMER = 1;
constexpr UINT_PTR FRAME_TIMER = 2;

int64_t epochMs() {
    FILETIME time{};
    GetSystemTimePreciseAsFileTime(&time);
    ULARGE_INTEGER value{{time.dwLowDateTime, time.dwHighDateTime}};
    return static_cast<int64_t>((value.QuadPart - 116444736000000000ULL) / 10000);
}

std::wstring formatTime(int64_t milliseconds) {
    int64_t total = std::max<int64_t>(0, milliseconds / 1000);
    int64_t hours = total / 3600;
    int64_t minutes = total % 3600 / 60;
    int64_t seconds = total % 60;
    wchar_t buffer[32];

    if (hours) {
        swprintf(buffer, 32, L"%lld:%02lld:%02lld", hours, minutes, seconds);
    } else {
        swprintf(buffer, 32, L"%lld:%02lld", minutes, seconds);
    }
    return buffer;
}

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
        winrt::check_hresult(resources.factory()->CreateDCRenderTarget(&properties, target.put()));
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
        Painter painter(context.get(), resources);
        paint(context.get(), painter);
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

enum class Control { None, Microphone, System, Pause, Discard, Stop };

struct ControlRect {
    Control control;
    D2D1_RECT_F rect;
    bool isDisabled;
};

class RecorderUi {
public:
    RecorderUi(ScreenRect area, ThemeMode theme, RecorderUiCallbacks value, bool showBorder)
        : callbacks(std::move(value)) {
        resources = Resources::acquire();
        palette = {theme == ThemeMode::Dark ? DARK_THEME : LIGHT_THEME, theme == ThemeMode::Dark};
        state.phase = RecorderPhase::Preparing;

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

    RecorderUi(ThemeMode theme, RecorderViewState const& value, double factor) : state(value), scale(factor) {
        resources = Resources::acquire();
        palette = {theme == ThemeMode::Dark ? DARK_THEME : LIGHT_THEME, theme == ThemeMode::Dark};
    }

    std::vector<uint8_t> renderImage(int& width, int& height) {
        width = static_cast<int>(std::round(TOOLBAR_WIDTH * scale));
        height = static_cast<int>(std::round(TOOLBAR_HEIGHT * scale));

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
        Painter painter(context.get(), *resources);
        paint(context.get(), painter, nowMs());
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
        state = value;
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
            auto tint = color(palette.theme.destructive);
            int length = isHorizontal ? border.width : border.height;
            float offset = isHorizontal ? static_cast<float>(rect.left - left) : static_cast<float>(rect.top - top);

            border.draw(*resources, [&](ID2D1DeviceContext*, Painter& painter) {
                for (float start = -std::fmod(offset, dash * 2); start < length; start += dash * 2) {
                    float end = std::min(static_cast<float>(length), start + dash);
                    float begin = std::max(0.0f, start);
                    if (end <= begin) continue;
                    painter.fillRect(isHorizontal ? D2D1::RectF(begin, 0, end, static_cast<float>(border.height))
                                                  : D2D1::RectF(0, begin, static_cast<float>(border.width), end),
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
            scale = dpiX / 96.0;
        } else {
            scale = host->scale;
        }

        double width = std::round(TOOLBAR_WIDTH * scale);
        double height = std::round(TOOLBAR_HEIGHT * scale);
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
        if (callbacks.onVisible) callbacks.onVisible();
    }

    bool isBusy() const {
        return state.phase == RecorderPhase::Preparing || state.phase == RecorderPhase::Countdown ||
               state.phase == RecorderPhase::Processing;
    }

    bool isMuted(AudioSource source) const {
        return std::find(state.muted.begin(), state.muted.end(), source) != state.muted.end();
    }

    std::wstring label() const {
        switch (state.phase) {
        case RecorderPhase::Preparing:
        case RecorderPhase::Idle:
            return t(L"recorder.starting");
        case RecorderPhase::Countdown:
            return t(L"recorder.startingIn", {{L"count", std::to_wstring(state.countdown)}});
        case RecorderPhase::Processing:
            return t(L"recorder.processing");
        default:
            break;
        }

        int64_t elapsed = state.elapsedMs + (state.sinceMs ? epochMs() - state.sinceMs : 0);
        auto time = formatTime(elapsed);
        return state.maxDurationMs ? time + L" / " + formatTime(state.maxDurationMs) : time;
    }

    std::vector<ControlRect> layout(float& pillLeft, float& pillWidth, float& statusWidth) {
        auto text = label();
        float textWidth = resources->measure(text, 14, DWRITE_FONT_WEIGHT_MEDIUM, FontKind::Sans, true);
        float indicator = isBusy() || state.phase == RecorderPhase::Paused ? 16.0f : 10.0f;
        statusWidth = std::max(96.0f, 10 + indicator + 8 + textWidth);

        float stopWidth = 12 + 16 + 4 + resources->measure(t(L"recorder.stop"), 14, DWRITE_FONT_WEIGHT_MEDIUM) + 12;
        std::vector<Control> buttons;
        std::vector<AudioSource> unique;
        for (auto source : state.sources)
            if (std::find(unique.begin(), unique.end(), source) == unique.end()) unique.push_back(source);
        for (auto source : unique) buttons.push_back(source == AudioSource::Microphone ? Control::Microphone : Control::System);
        buttons.push_back(Control::Pause);
        buttons.push_back(Control::Discard);

        float content = statusWidth + 4 + 9 + static_cast<float>(buttons.size()) * (4 + BUTTON_SIZE) + 4 + stopWidth;
        pillWidth = 1 + 6 + content + 6 + 1;
        pillLeft = (TOOLBAR_WIDTH - pillWidth) / 2;

        std::vector<ControlRect> result;
        float x = pillLeft + 7 + statusWidth + 4 + 9;
        float top = (TOOLBAR_HEIGHT - BUTTON_SIZE) / 2;

        for (auto control : buttons) {
            x += 4;
            bool isDisabled = control == Control::Pause    ? state.phase != RecorderPhase::Recording &&
                                                                 state.phase != RecorderPhase::Paused
                              : control == Control::Discard ? state.phase == RecorderPhase::Processing
                                                            : isBusy();
            result.push_back({control, D2D1::RectF(x, top, x + BUTTON_SIZE, top + BUTTON_SIZE), isDisabled});
            x += BUTTON_SIZE;
        }

        x += 4;
        result.push_back({Control::Stop, D2D1::RectF(x, top, x + stopWidth, top + BUTTON_SIZE), isBusy()});
        return result;
    }

    Animation& hoverOf(Control control) {
        return hovers[static_cast<size_t>(control)];
    }

    void render() {
        if (!toolbar.window) return;

        double now = nowMs();
        toolbar.draw(*resources, [&](ID2D1DeviceContext* context, Painter& painter) { paint(context, painter, now); });
    }

    void paint(ID2D1DeviceContext* context, Painter& painter, double now) {
        auto s = static_cast<float>(scale);
        Transform transform(context, D2D1::Matrix3x2F::Scale(s, s));
        auto const& theme = palette.theme;

        float pillLeft = 0;
        float pillWidth = 0;
        float statusWidth = 0;
        auto controls = layout(pillLeft, pillWidth, statusWidth);

        auto pill = D2D1::RectF(pillLeft, 8, pillLeft + pillWidth, 56);
        painter.boxShadow(pill, 24, 4, 6, -1, 0.1f);
        painter.boxShadow(pill, 24, 2, 4, -2, 0.1f);
        painter.fillRounded(pill, 24, color(theme.popover));
        painter.strokeRounded(D2D1::RectF(pill.left + 0.5f, pill.top + 0.5f, pill.right - 0.5f, pill.bottom - 0.5f),
                              23.5f, color(theme.border), 1);

        auto foreground = color(theme.popoverForeground);
        float x = pillLeft + 7 + 10;
        float center = TOOLBAR_HEIGHT / 2;

        if (isBusy()) {
            drawSpinner(painter, x, center - 8, 16, foreground, now);
            x += 16;
        } else if (state.phase == RecorderPhase::Paused) {
            painter.icon(icons::pause, x, center - 8, 16, color(theme.mutedForeground));
            x += 16;
        } else {
            double phase = std::fmod(now, 2000.0) / 2000.0;
            double wave = phase < 0.5 ? cubicBezier(0.4, 0, 0.6, 1, phase * 2) : 1 - cubicBezier(0.4, 0, 0.6, 1, (phase - 0.5) * 2);
            painter.fillEllipse(x + 5, center, 5, 5, color(theme.destructive, static_cast<float>(1 - 0.5 * wave)));
            x += 10;
        }

        x += 8;
        auto text = label();
        auto layout = resources->layout(text, 14, DWRITE_FONT_WEIGHT_MEDIUM, FontKind::Sans, 0, true);
        DWRITE_LINE_METRICS line{};
        UINT32 count = 0;
        layout->GetLineMetrics(&line, 1, &count);
        painter.text(layout.get(), x, painter.middleBaseline(center, 14) - line.baseline, foreground);

        float separatorX = pillLeft + 7 + statusWidth + 4 + 4;
        painter.fillRect(D2D1::RectF(separatorX, 20, separatorX + 1, 44), color(theme.border));

        for (auto const& control : controls) {
            ButtonStyle style;
            style.variant = control.control == Control::Stop ? ButtonVariant::Destructive : ButtonVariant::Ghost;
            style.hover = hoverOf(control.control).value(now);
            style.isPressed = pressed == control.control && hovered == control.control;
            style.isDisabled = control.isDisabled;

            if (style.isDisabled) painter.pushOpacity(0.5f);
            drawButton(painter, palette, control.rect, style);

            auto tint = buttonForeground(palette, style);
            float offset = buttonOffset(style);
            float iconX = control.rect.left + 8;
            float iconY = control.rect.top + 8 + offset;

            switch (control.control) {
            case Control::Microphone:
                painter.icon(isMuted(AudioSource::Microphone) ? icons::micOff : icons::mic, iconX, iconY, 16,
                             isMuted(AudioSource::Microphone) ? color(theme.destructive) : tint);
                break;
            case Control::System:
                painter.icon(isMuted(AudioSource::System) ? icons::volumeX : icons::volume2, iconX, iconY, 16,
                             isMuted(AudioSource::System) ? color(theme.destructive) : tint);
                break;
            case Control::Pause:
                painter.icon(state.phase == RecorderPhase::Paused ? icons::play : icons::pause, iconX, iconY, 16,
                             tint);
                break;
            case Control::Discard:
                painter.icon(icons::trash2, iconX, iconY, 16, tint);
                break;
            case Control::Stop: {
                float left = control.rect.left + 12;
                painter.icon(icons::square, left, iconY, 16, tint, true);
                painter.textAt(t(L"recorder.stop"), left + 20, painter.middleBaseline(center + offset, 14), 14,
                               DWRITE_FONT_WEIGHT_MEDIUM, tint);
                break;
            }
            default:
                break;
            }

            if (style.isDisabled) painter.popOpacity();
        }
    }

    bool isAnimating(double now) {
        for (auto const& hover : hovers)
            if (hover.isRunning(now)) return true;
        return state.phase != RecorderPhase::Paused;
    }

    Control controlAt(LPARAM lParam) {
        float pillLeft = 0;
        float pillWidth = 0;
        float statusWidth = 0;
        double x = GET_X_LPARAM(lParam) / scale;
        double y = GET_Y_LPARAM(lParam) / scale;

        for (auto const& control : layout(pillLeft, pillWidth, statusWidth)) {
            auto const& r = control.rect;
            if (!control.isDisabled && x >= r.left && x < r.right && y >= r.top && y < r.bottom) return control.control;
        }
        return Control::None;
    }

    void activate(Control control) {
        switch (control) {
        case Control::Microphone:
            if (callbacks.onMute) callbacks.onMute(AudioSource::Microphone, !isMuted(AudioSource::Microphone));
            break;
        case Control::System:
            if (callbacks.onMute) callbacks.onMute(AudioSource::System, !isMuted(AudioSource::System));
            break;
        case Control::Pause:
            if (callbacks.onControl)
                callbacks.onControl(state.phase == RecorderPhase::Paused ? RecorderAction::Resume : RecorderAction::Pause);
            break;
        case Control::Discard:
            if (callbacks.onControl) callbacks.onControl(RecorderAction::Abort);
            break;
        case Control::Stop:
            if (callbacks.onControl) callbacks.onControl(RecorderAction::Stop);
            break;
        default:
            break;
        }
    }

    void setHovered(Control control) {
        if (control == hovered) return;
        double now = nowMs();
        if (hovered != Control::None) hoverOf(hovered).animate(0, now, BUTTON_DURATION);
        if (control != Control::None) hoverOf(control).animate(1, now, BUTTON_DURATION);
        hovered = control;
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
                if (isAnimating(now) || now - lastFrame >= 250) {
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
            setHovered(controlAt(lParam));
            render();
            return 0;
        }
        case WM_MOUSELEAVE:
            isTracking = false;
            setHovered(Control::None);
            render();
            return 0;
        case WM_LBUTTONDOWN:
            SetCapture(window);
            pressed = controlAt(lParam);
            render();
            return 0;
        case WM_LBUTTONUP: {
            ReleaseCapture();
            auto control = controlAt(lParam);
            auto previous = std::exchange(pressed, Control::None);
            render();
            if (control != Control::None && control == previous) activate(control);
            return 0;
        }
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_NCHITTEST: {
            POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(window, &point);
            float pillLeft = 0;
            float pillWidth = 0;
            float statusWidth = 0;
            layout(pillLeft, pillWidth, statusWidth);
            double x = point.x / scale;
            double y = point.y / scale;
            bool isInside = x >= pillLeft && x < pillLeft + pillWidth && y >= 8 && y < 56;
            return isInside ? HTCLIENT : HTTRANSPARENT;
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

    RecorderUiCallbacks callbacks;
    std::shared_ptr<Resources> resources;
    Palette palette;
    RecorderViewState state;
    LayeredWindow toolbar;
    std::vector<LayeredWindow> borders;
    double scale = 1;
    std::array<Animation, 6> hovers{};
    Control hovered = Control::None;
    Control pressed = Control::None;
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

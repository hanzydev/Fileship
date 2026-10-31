#include "direct2d.h"

#include <ui/overlayController.h>
#include <shared/log.h>
#include <shared/system.h>

#include <commdlg.h>
#include <imm.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>

namespace fileship::ui {

namespace {

constexpr UINT_PTR CARET_TIMER = 1;

void forceForeground(HWND window) {
    HWND foreground = GetForegroundWindow();
    DWORD current = GetCurrentThreadId();
    DWORD target = foreground ? GetWindowThreadProcessId(foreground, nullptr) : 0;
    bool shouldAttach = target != 0 && target != current;

    if (shouldAttach) AttachThreadInput(current, target, TRUE);
    BringWindowToTop(window);
    SetForegroundWindow(window);
    SetFocus(window);
    if (shouldAttach) AttachThreadInput(current, target, FALSE);
}

Key keyOf(WPARAM key) {
    if (key >= '0' && key <= '9') return static_cast<Key>(static_cast<int>(Key::Digit0) + static_cast<int>(key - '0'));
    if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9)
        return static_cast<Key>(static_cast<int>(Key::Digit0) + static_cast<int>(key - VK_NUMPAD0));

    switch (key) {
    case VK_ESCAPE:
        return Key::Escape;
    case VK_RETURN:
        return Key::Enter;
    case VK_SPACE:
        return Key::Space;
    case VK_TAB:
        return Key::Tab;
    case VK_BACK:
        return Key::Backspace;
    case VK_DELETE:
        return Key::Delete;
    case VK_LEFT:
        return Key::Left;
    case VK_RIGHT:
        return Key::Right;
    case VK_UP:
        return Key::Up;
    case VK_DOWN:
        return Key::Down;
    case VK_HOME:
        return Key::Home;
    case VK_END:
        return Key::End;
    case 'A':
        return Key::A;
    case 'C':
        return Key::C;
    case 'V':
        return Key::V;
    case 'X':
        return Key::X;
    case 'Y':
        return Key::Y;
    case 'Z':
        return Key::Z;
    default:
        return Key::Unknown;
    }
}

Modifiers modifiers() {
    return {GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, GetKeyState(VK_MENU) < 0};
}

LPCWSTR cursorOf(CursorKind kind) {
    switch (kind) {
    case CursorKind::Arrow:
        return IDC_ARROW;
    case CursorKind::Hand:
        return IDC_HAND;
    case CursorKind::IBeam:
        return IDC_IBEAM;
    case CursorKind::Move:
        return IDC_SIZEALL;
    case CursorKind::ResizeNwse:
        return IDC_SIZENWSE;
    case CursorKind::ResizeNesw:
        return IDC_SIZENESW;
    case CursorKind::ResizeNs:
        return IDC_SIZENS;
    case CursorKind::ResizeWe:
        return IDC_SIZEWE;
    default:
        return IDC_CROSS;
    }
}

class Win32OverlayWindow : public OverlayWindow {
public:
    explicit Win32OverlayWindow(OverlayController& controller) : controller(controller) {}

    ~Win32OverlayWindow() override {
        destroy();
    }

    bool open() override {
        static bool isRegistered = false;
        if (!isRegistered) {
            WNDCLASSW windowClass{};
            windowClass.lpfnWndProc = handleMessage;
            windowClass.hInstance = GetModuleHandleW(nullptr);
            windowClass.lpszClassName = L"FileshipOverlay";
            windowClass.hCursor = nullptr;
            RegisterClassW(&windowClass);
            isRegistered = true;
        }

        auto const& bounds = controller.screenBounds();
        window = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOREDIRECTIONBITMAP, L"FileshipOverlay",
                                 L"Fileship", WS_POPUP, bounds.x, bounds.y, bounds.width, bounds.height, nullptr,
                                 nullptr, GetModuleHandleW(nullptr), this);
        if (!window) return false;

        auto& gpu = gpuOf(controller.device());
        ImmAssociateContextEx(window, nullptr, 0);
        surface.create(gpu, window, bounds.width, bounds.height, false);
        paint();
        gpu.composition->WaitForCommitCompletion();

        ShowWindow(window, SW_SHOW);
        forceForeground(window);
        return true;
    }

    void destroy() override {
        if (window) {
            KillTimer(window, CARET_TIMER);
            SetWindowLongPtrW(window, GWLP_USERDATA, 0);
            DestroyWindow(window);
            window = nullptr;
        }

        surface.release();
    }

    void invalidate() override {
        if (window) InvalidateRect(window, nullptr, FALSE);
    }

    void textInputChanged(bool isEditing) override {
        ImmAssociateContextEx(window, nullptr, isEditing ? IACE_DEFAULT : 0);
        if (isEditing) {
            SetTimer(window, CARET_TIMER, 50, nullptr);
        } else {
            KillTimer(window, CARET_TIMER);
        }
    }

    void caretMoved() override {
        auto caret = controller.caretRect();
        if (!caret) return;

        HIMC context = ImmGetContext(window);
        if (!context) return;

        double ratio = controller.scale();
        COMPOSITIONFORM composition{};
        composition.dwStyle = CFS_POINT;
        composition.ptCurrentPos = {static_cast<LONG>(caret->x * ratio), static_cast<LONG>(caret->y * ratio)};
        ImmSetCompositionWindow(context, &composition);

        CANDIDATEFORM candidate{};
        candidate.dwIndex = 0;
        candidate.dwStyle = CFS_CANDIDATEPOS;
        candidate.ptCurrentPos = {static_cast<LONG>(caret->x * ratio), static_cast<LONG>(caret->bottom() * ratio)};
        ImmSetCandidateWindow(context, &candidate);
        ImmReleaseContext(window, context);
    }

    std::optional<std::string> pickColor(std::string const& current) override {
        static COLORREF custom[16]{};
        auto parsed = parseHex(current);
        CHOOSECOLORW dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = window;
        dialog.rgbResult = RGB(std::lround(parsed.r * 255), std::lround(parsed.g * 255), std::lround(parsed.b * 255));
        dialog.lpCustColors = custom;
        dialog.Flags = CC_FULLOPEN | CC_RGBINIT;

        if (!ChooseColorW(&dialog)) return std::nullopt;
        return toHex(rgba(GetRValue(dialog.rgbResult), GetGValue(dialog.rgbResult), GetBValue(dialog.rgbResult)));
    }

    void copyText(std::wstring const& text) override {
        if (!OpenClipboard(nullptr)) return;

        EmptyClipboard();
        size_t bytes = (text.size() + 1) * sizeof(wchar_t);
        if (HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes)) {
            memcpy(GlobalLock(memory), text.c_str(), bytes);
            GlobalUnlock(memory);
            SetClipboardData(CF_UNICODETEXT, memory);
        }
        CloseClipboard();
    }

    std::wstring pasteText() override {
        std::wstring pasted;
        if (!OpenClipboard(nullptr)) return pasted;

        if (HANDLE data = GetClipboardData(CF_UNICODETEXT)) {
            pasted = static_cast<wchar_t const*>(GlobalLock(data));
            GlobalUnlock(data);
        }
        CloseClipboard();
        return pasted;
    }

private:
    Point localPoint(LPARAM lParam) const {
        return {GET_X_LPARAM(lParam) / controller.scale(), GET_Y_LPARAM(lParam) / controller.scale()};
    }

    void paint() {
        if (!window) return;

        auto& gpu = gpuOf(controller.device());
        auto context = surface.begin(gpu);
        {
            auto painter = createPainter(context, controller.resources());
            controller.render(*painter);
        }
        surface.present(gpu);
    }

    std::wstring compositionString(HIMC context, DWORD kind) {
        LONG bytes = ImmGetCompositionStringW(context, kind, nullptr, 0);
        if (bytes <= 0) return {};
        std::wstring text(static_cast<size_t>(bytes) / sizeof(wchar_t), L'\0');
        ImmGetCompositionStringW(context, kind, text.data(), bytes);
        return text;
    }

    static LRESULT CALLBACK handleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == WM_NCCREATE) {
            auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            reinterpret_cast<Win32OverlayWindow*>(create->lpCreateParams)->window = window;
        }

        auto self = reinterpret_cast<Win32OverlayWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (!self) return DefWindowProcW(window, message, wParam, lParam);

        try {
            return self->handle(message, wParam, lParam);
        } catch (...) {
            logError(L"The capture overlay failed: " + describeException(std::current_exception()));
            self->controller.close();
            return 0;
        }
    }

    LRESULT handle(UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case WM_PAINT: {
            ValidateRect(window, nullptr);
            paint();
            if (window && controller.isAnimating()) invalidate();
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_MOUSEMOVE: {
            if (!isTrackingLeave) {
                TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, window, 0};
                TrackMouseEvent(&track);
                isTrackingLeave = true;
            }
            controller.pointerMove(localPoint(lParam), wParam & MK_SHIFT);
            invalidate();
            return 0;
        }
        case WM_MOUSELEAVE:
            isTrackingLeave = false;
            if (GetCapture() != window) {
                controller.pointerLeave();
                invalidate();
            }
            return 0;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            SetCapture(window);
            controller.pointerDown(localPoint(lParam), wParam & MK_SHIFT);
            controller.afterInput();
            return 0;
        case WM_LBUTTONUP:
            isReleasing = true;
            ReleaseCapture();
            isReleasing = false;
            controller.pointerUp(localPoint(lParam));
            controller.afterInput();
            return 0;
        case WM_CAPTURECHANGED:
            if (!isReleasing && reinterpret_cast<HWND>(lParam) != window) {
                controller.pointerCancel();
                invalidate();
            }
            return 0;
        case WM_RBUTTONUP:
            controller.rightClick(localPoint(lParam));
            controller.afterInput();
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (message == WM_SYSKEYDOWN && wParam == VK_F4) return 0;
            controller.keyDown(keyOf(wParam), modifiers());
            controller.afterInput();
            return 0;
        case WM_CHAR:
            controller.character(static_cast<wchar_t>(wParam), modifiers());
            controller.afterInput();
            return 0;
        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                POINT position;
                GetCursorPos(&position);
                ScreenToClient(window, &position);
                double ratio = controller.scale();
                SetCursor(LoadCursorW(nullptr, cursorOf(controller.cursorAt({position.x / ratio, position.y / ratio}))));
                return TRUE;
            }
            break;
        case WM_TIMER:
            if (wParam == CARET_TIMER) {
                bool visible = controller.caretVisible();
                if (visible != wasCaretVisible) {
                    wasCaretVisible = visible;
                    invalidate();
                }
            }
            return 0;
        case WM_IME_SETCONTEXT:
            lParam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
            return DefWindowProcW(window, message, wParam, lParam);
        case WM_IME_STARTCOMPOSITION:
            caretMoved();
            return 0;
        case WM_IME_COMPOSITION: {
            if (!controller.isEditingText()) break;
            HIMC context = ImmGetContext(window);
            if (context) {
                if (lParam & GCS_RESULTSTR) controller.commit(compositionString(context, GCS_RESULTSTR));
                if (lParam & GCS_COMPSTR) controller.composition(compositionString(context, GCS_COMPSTR));
                ImmReleaseContext(window, context);
            }
            caretMoved();
            invalidate();
            return 0;
        }
        case WM_IME_ENDCOMPOSITION:
            controller.composition(L"");
            invalidate();
            return 0;
        case WM_DPICHANGED:
            return 0;
        case WM_CLOSE:
            controller.close();
            return 0;
        case WM_MOUSEACTIVATE:
            return MA_ACTIVATE;
        default:
            break;
        }

        return DefWindowProcW(window, message, wParam, lParam);
    }

    OverlayController& controller;
    HWND window = nullptr;
    SwapSurface surface;
    bool isTrackingLeave = false;
    bool isReleasing = false;
    bool wasCaretVisible = false;
};

}

std::unique_ptr<OverlayWindow> createOverlayWindow(OverlayController& controller) {
    return std::make_unique<Win32OverlayWindow>(controller);
}

}

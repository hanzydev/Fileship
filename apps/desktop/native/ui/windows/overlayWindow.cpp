#include "d2d.h"

#include <ui/overlayWindow.h>

#include <shared/log.h>

#include <commdlg.h>
#include <imm.h>
#include <windowsx.h>

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

Key keyFrom(WPARAM code) {
    if ((code >= '0' && code <= '9') || (code >= 'A' && code <= 'Z')) return static_cast<Key>(code);
    if (code >= VK_NUMPAD0 && code <= VK_NUMPAD9)
        return static_cast<Key>(static_cast<uint32_t>(Key::Numpad0) + (code - VK_NUMPAD0));

    switch (code) {
    case VK_BACK:
        return Key::Backspace;
    case VK_TAB:
        return Key::Tab;
    case VK_RETURN:
        return Key::Enter;
    case VK_ESCAPE:
        return Key::Escape;
    case VK_SPACE:
        return Key::Space;
    case VK_DELETE:
        return Key::Delete;
    case VK_LEFT:
        return Key::Left;
    case VK_UP:
        return Key::Up;
    case VK_RIGHT:
        return Key::Right;
    case VK_DOWN:
        return Key::Down;
    case VK_HOME:
        return Key::Home;
    case VK_END:
        return Key::End;
    default:
        return Key::None;
    }
}

wchar_t const* cursorName(Cursor cursor) {
    switch (cursor) {
    case Cursor::Hand:
        return IDC_HAND;
    case Cursor::Cross:
        return IDC_CROSS;
    case Cursor::Text:
        return IDC_IBEAM;
    case Cursor::Move:
        return IDC_SIZEALL;
    case Cursor::ResizeNwse:
        return IDC_SIZENWSE;
    case Cursor::ResizeNesw:
        return IDC_SIZENESW;
    case Cursor::ResizeNs:
        return IDC_SIZENS;
    case Cursor::ResizeWe:
        return IDC_SIZEWE;
    default:
        return IDC_ARROW;
    }
}

Modifiers modifiers() {
    return {GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, GetKeyState(VK_MENU) < 0};
}

class Win32Overlay : public OverlayWindow {
public:
    Win32Overlay(D2DCanvas& canvas, double ratio, OverlayClient& client, Annotator& annotator)
        : canvas(canvas), ratio(ratio), client(client), annotator(annotator) {}

    ~Win32Overlay() override {
        close();
    }

    bool create(ScreenRect const& bounds) {
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

        window = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOREDIRECTIONBITMAP, L"FileshipOverlay",
                                 L"Fileship", WS_POPUP, bounds.x, bounds.y, bounds.width, bounds.height, nullptr,
                                 nullptr, GetModuleHandleW(nullptr), this);
        if (!window) return false;

        ImmAssociateContextEx(window, nullptr, 0);
        surface.create(canvas.gpu(), window, bounds.width, bounds.height, false);
        paint();
        canvas.gpu().composition->WaitForCommitCompletion();

        ShowWindow(window, SW_SHOW);
        forceForeground(window);
        return true;
    }

    void close() override {
        if (window) {
            KillTimer(window, CARET_TIMER);
            SetWindowLongPtrW(window, GWLP_USERDATA, 0);
            DestroyWindow(window);
            window = nullptr;
        }

        surface.release();
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

private:
    Point localPoint(LPARAM lParam) const {
        return {GET_X_LPARAM(lParam) / ratio, GET_Y_LPARAM(lParam) / ratio};
    }

    void invalidate() {
        if (window) InvalidateRect(window, nullptr, FALSE);
    }

    void paint() {
        if (!window) return;

        D2DPainter painter(surface.begin(canvas.gpu()), canvas.resources());
        client.paint(painter);
        surface.present(canvas.gpu());
    }

    void updateIme() {
        bool editing = annotator.isEditingText();
        if (editing == isImeEnabled) return;

        isImeEnabled = editing;
        ImmAssociateContextEx(window, nullptr, editing ? IACE_DEFAULT : 0);
        if (editing) {
            SetTimer(window, CARET_TIMER, 50, nullptr);
        } else {
            KillTimer(window, CARET_TIMER);
        }
    }

    void positionIme() {
        if (!annotator.isEditingText()) return;

        float height = 0;
        auto caret = annotator.editorCaret(height);
        HIMC context = ImmGetContext(window);
        if (!context) return;

        COMPOSITIONFORM composition{};
        composition.dwStyle = CFS_POINT;
        composition.ptCurrentPos = {static_cast<LONG>(caret.x * ratio), static_cast<LONG>(caret.y * ratio)};
        ImmSetCompositionWindow(context, &composition);

        CANDIDATEFORM candidate{};
        candidate.dwIndex = 0;
        candidate.dwStyle = CFS_CANDIDATEPOS;
        candidate.ptCurrentPos = {static_cast<LONG>(caret.x * ratio), static_cast<LONG>((caret.y + height) * ratio)};
        ImmSetCandidateWindow(context, &candidate);
        ImmReleaseContext(window, context);
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
            reinterpret_cast<Win32Overlay*>(create->lpCreateParams)->window = window;
        }

        auto self = reinterpret_cast<Win32Overlay*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (!self) return DefWindowProcW(window, message, wParam, lParam);

        try {
            return self->handle(message, wParam, lParam);
        } catch (winrt::hresult_error const& error) {
            logError(L"The capture overlay failed: " + std::wstring(error.message()));
            self->client.cancel();
            return 0;
        }
    }

    LRESULT handle(UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case WM_PAINT: {
            ValidateRect(window, nullptr);
            paint();
            if (window && client.isAnimating()) invalidate();
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
            client.pointerMove(localPoint(lParam), wParam & MK_SHIFT);
            invalidate();
            return 0;
        }
        case WM_MOUSELEAVE:
            isTrackingLeave = false;
            if (GetCapture() != window) {
                client.pointerLeave();
                invalidate();
            }
            return 0;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            SetCapture(window);
            client.pointerDown(localPoint(lParam), wParam & MK_SHIFT);
            updateIme();
            invalidate();
            return 0;
        case WM_LBUTTONUP:
            isReleasing = true;
            ReleaseCapture();
            isReleasing = false;
            client.pointerUp(localPoint(lParam));
            if (!window) return 0;
            updateIme();
            invalidate();
            return 0;
        case WM_CAPTURECHANGED:
            if (!isReleasing && reinterpret_cast<HWND>(lParam) != window) {
                client.pointerCancel();
                invalidate();
            }
            return 0;
        case WM_RBUTTONUP:
            client.rightClick(localPoint(lParam));
            if (!window) return 0;
            invalidate();
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (message == WM_SYSKEYDOWN && wParam == VK_F4) return 0;
            client.keyDown(keyFrom(wParam), modifiers());
            if (!window) return 0;
            updateIme();
            invalidate();
            return 0;
        case WM_CHAR:
            client.character(static_cast<wchar_t>(wParam), modifiers());
            if (!window) return 0;
            updateIme();
            invalidate();
            return 0;
        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                POINT position;
                GetCursorPos(&position);
                ScreenToClient(window, &position);
                SetCursor(LoadCursorW(nullptr, cursorName(client.cursorAt({position.x / ratio, position.y / ratio}))));
                return TRUE;
            }
            break;
        case WM_TIMER:
            if (wParam == CARET_TIMER) {
                bool visible = annotator.caretVisible();
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
            positionIme();
            return 0;
        case WM_IME_COMPOSITION: {
            if (!annotator.isEditingText()) break;
            HIMC context = ImmGetContext(window);
            if (context) {
                if (lParam & GCS_RESULTSTR) annotator.editorCommit(compositionString(context, GCS_RESULTSTR));
                if (lParam & GCS_COMPSTR) annotator.editorComposition(compositionString(context, GCS_COMPSTR));
                ImmReleaseContext(window, context);
            }
            positionIme();
            invalidate();
            return 0;
        }
        case WM_IME_ENDCOMPOSITION:
            annotator.editorComposition(L"");
            invalidate();
            return 0;
        case WM_DPICHANGED:
            return 0;
        case WM_CLOSE:
            client.cancel();
            return 0;
        case WM_MOUSEACTIVATE:
            return MA_ACTIVATE;
        default:
            break;
        }

        return DefWindowProcW(window, message, wParam, lParam);
    }

    D2DCanvas& canvas;
    double ratio;
    OverlayClient& client;
    Annotator& annotator;
    SwapSurface surface;
    HWND window = nullptr;
    bool isTrackingLeave = false;
    bool isReleasing = false;
    bool isImeEnabled = false;
    bool wasCaretVisible = false;
};

}

std::unique_ptr<OverlayWindow> OverlayWindow::open(Canvas& canvas, OverlayRequest const& request, double ratio,
                                                   OverlayClient& client, Annotator& annotator) {
    return guarded([&]() -> std::unique_ptr<OverlayWindow> {
        auto window = std::make_unique<Win32Overlay>(static_cast<D2DCanvas&>(canvas), ratio, client, annotator);
        if (!window->create(request.bounds)) return nullptr;
        return window;
    });
}

}

#include "core.h"

#include <dwmapi.h>

#include <algorithm>

namespace fileship::core {

namespace {

constexpr int MAX_CONTROLS = 3000;

std::wstring readTitle(HWND window) {
    wchar_t buffer[256]{};
    int length = GetWindowTextW(window, buffer, 256);
    return std::wstring(buffer, std::max(length, 0));
}

bool readFrame(HWND window, Rect& frame) {
    RECT rect{};

    if (FAILED(DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect))) &&
        !GetWindowRect(window, &rect))
        return false;

    frame = Rect{rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top};
    return frame.width > 0 && frame.height > 0;
}

bool isCloaked(HWND window) {
    DWORD cloaked = 0;
    return SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked != 0;
}

bool isIgnored(std::vector<uint64_t> const& ignored, HWND window) {
    auto handle = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(window));
    return std::find(ignored.begin(), ignored.end(), handle) != ignored.end();
}

struct Enumeration {
    std::vector<uint64_t> const* ignored;
    bool controls;
    int controlCount = 0;
    std::vector<WindowInfo> windows;
    WindowInfo* current = nullptr;
};

BOOL CALLBACK handleChild(HWND child, LPARAM parameter) {
    auto& state = *reinterpret_cast<Enumeration*>(parameter);
    RECT rect{};

    if (IsWindowVisible(child) && GetWindowRect(child, &rect)) {
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        if (width > 4 && height > 4 && state.controlCount < MAX_CONTROLS) {
            state.current->controls.push_back(Rect{rect.left, rect.top, width, height});
            state.controlCount++;
        }
    }

    return state.controlCount < MAX_CONTROLS;
}

BOOL CALLBACK handleWindow(HWND window, LPARAM parameter) {
    auto& state = *reinterpret_cast<Enumeration*>(parameter);

    if (isIgnored(*state.ignored, window)) return TRUE;
    if (!IsWindowVisible(window) || IsIconic(window) || isCloaked(window)) return TRUE;

    auto exStyle = GetWindowLongPtrW(window, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW || exStyle & WS_EX_NOACTIVATE) return TRUE;

    WindowInfo info;
    if (!readFrame(window, info.frame)) return TRUE;

    info.title = readTitle(window);

    if (state.controls && state.controlCount < MAX_CONTROLS) {
        state.current = &info;
        EnumChildWindows(window, handleChild, parameter);
        state.current = nullptr;
    }

    state.windows.push_back(std::move(info));
    return TRUE;
}

}

std::vector<WindowInfo> listWindows(std::vector<uint64_t> const& ignored, bool controls) {
    Enumeration state;
    state.ignored = &ignored;
    state.controls = controls;
    EnumWindows(handleWindow, reinterpret_cast<LPARAM>(&state));
    return std::move(state.windows);
}

std::optional<WindowInfo> foregroundWindow(std::vector<uint64_t> const& ignored) {
    HWND window = GetForegroundWindow();
    if (!window || isIgnored(ignored, window)) return std::nullopt;

    WindowInfo info;
    if (!readFrame(window, info.frame)) return std::nullopt;

    info.title = readTitle(window);
    return info;
}

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

}

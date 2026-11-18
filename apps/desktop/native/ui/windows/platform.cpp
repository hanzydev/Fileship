#include <ui/platform.h>

#include <windows.h>

#include <cstring>

namespace fileship::ui {

double caretBlinkMs() {
    double blink = GetCaretBlinkTime();
    return blink <= 0 || blink >= INFINITE ? 0 : blink;
}

std::optional<std::wstring> clipboardText() {
    std::optional<std::wstring> text;

    if (OpenClipboard(nullptr)) {
        if (HANDLE data = GetClipboardData(CF_UNICODETEXT)) {
            text = std::wstring(static_cast<wchar_t const*>(GlobalLock(data)));
            GlobalUnlock(data);
        }
        CloseClipboard();
    }

    return text;
}

void setClipboardText(std::wstring const& text) {
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

}

#include "app.h"

#include <shared/log.h>

#include <shellapi.h>

#include <atomic>
#include <cstring>

namespace fileship::host {

namespace {

constexpr int RETRY_DELAYS[] = {0, 100, 200};
constexpr int64_t HISTORY_GAP_MS = 1000;

std::atomic<int64_t> lastImageWriteAt = 0;

UINT pngFormat() {
    static UINT const format = RegisterClipboardFormatW(L"PNG");
    return format;
}

UINT fileNameFormat() {
    static UINT const format = RegisterClipboardFormatW(L"FileNameW");
    return format;
}

struct ClipboardLock {
    bool isOpen = false;

    ClipboardLock() {
        for (int attempt = 0; attempt < 10 && !isOpen; attempt++) {
            isOpen = OpenClipboard(mainWindow()) != FALSE;
            if (!isOpen) Sleep(20);
        }
    }

    ~ClipboardLock() {
        if (isOpen) CloseClipboard();
    }
};

HGLOBAL copyToGlobal(void const* data, size_t size) {
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!memory) return nullptr;

    std::memcpy(GlobalLock(memory), data, size);
    GlobalUnlock(memory);
    return memory;
}

bool setData(UINT format, void const* data, size_t size) {
    HGLOBAL memory = copyToGlobal(data, size);
    if (!memory) return false;

    if (!SetClipboardData(format, memory)) {
        GlobalFree(memory);
        return false;
    }

    return true;
}

std::vector<uint8_t> dibV5(core::Bitmap const& bitmap) {
    BITMAPV5HEADER header{};
    header.bV5Size = sizeof(header);
    header.bV5Width = static_cast<LONG>(bitmap.width);
    header.bV5Height = static_cast<LONG>(bitmap.height);
    header.bV5Planes = 1;
    header.bV5BitCount = 32;
    header.bV5Compression = BI_BITFIELDS;
    header.bV5SizeImage = bitmap.width * bitmap.height * 4;
    header.bV5RedMask = 0x00ff0000;
    header.bV5GreenMask = 0x0000ff00;
    header.bV5BlueMask = 0x000000ff;
    header.bV5AlphaMask = 0xff000000;
    header.bV5CSType = LCS_sRGB;
    header.bV5Intent = LCS_GM_IMAGES;

    std::vector<uint8_t> result(sizeof(header) + header.bV5SizeImage);
    std::memcpy(result.data(), &header, sizeof(header));

    size_t stride = static_cast<size_t>(bitmap.width) * 4;
    for (uint32_t row = 0; row < bitmap.height; row++) {
        std::memcpy(result.data() + sizeof(header) + (bitmap.height - 1 - row) * stride,
                    bitmap.pixels.data() + row * stride, stride);
    }

    return result;
}

bool withRetry(wchar_t const* label, std::function<bool()> const& write) {
    for (size_t attempt = 0; attempt < std::size(RETRY_DELAYS); attempt++) {
        if (RETRY_DELAYS[attempt]) Sleep(static_cast<DWORD>(RETRY_DELAYS[attempt]));
        if (write()) return true;
    }

    logWarn(std::wstring(L"Could not copy the ") + label);
    return false;
}

std::optional<std::vector<uint8_t>> readFormat(UINT format) {
    ClipboardLock lock;
    if (!lock.isOpen) return std::nullopt;

    HANDLE handle = GetClipboardData(format);
    if (!handle) return std::nullopt;

    auto data = static_cast<uint8_t const*>(GlobalLock(handle));
    if (!data) return std::nullopt;

    std::vector<uint8_t> result(data, data + GlobalSize(handle));
    GlobalUnlock(handle);
    return result;
}

std::optional<core::Bitmap> decodeDib(std::vector<uint8_t> const& dib) {
    if (dib.size() < sizeof(BITMAPINFOHEADER)) return std::nullopt;

    auto info = reinterpret_cast<BITMAPINFOHEADER const*>(dib.data());
    DWORD colors = info->biClrUsed ? info->biClrUsed : (info->biBitCount <= 8 ? 1u << info->biBitCount : 0);
    DWORD masks = info->biSize == sizeof(BITMAPINFOHEADER) && info->biCompression == BI_BITFIELDS ? 12 : 0;

    BITMAPFILEHEADER file{};
    file.bfType = 0x4d42;
    file.bfSize = static_cast<DWORD>(sizeof(file) + dib.size());
    file.bfOffBits = static_cast<DWORD>(sizeof(file) + info->biSize + masks + colors * sizeof(RGBQUAD));

    std::vector<uint8_t> bmp(sizeof(file));
    std::memcpy(bmp.data(), &file, sizeof(file));
    bmp.insert(bmp.end(), dib.begin(), dib.end());
    return core::decodeImage(bmp.data(), bmp.size());
}

}

void writeClipboardText(std::wstring const& text) {
    auto wait = lastImageWriteAt + HISTORY_GAP_MS - nowMs();
    if (wait > 0) Sleep(static_cast<DWORD>(wait));

    withRetry(L"text", [&] {
        ClipboardLock lock;
        if (!lock.isOpen || !EmptyClipboard()) return false;
        return setData(CF_UNICODETEXT, text.c_str(), (text.size() + 1) * sizeof(wchar_t));
    });
}

bool writeClipboardImage(std::vector<uint8_t> const& png, core::Bitmap const& bitmap) {
    auto dib = dibV5(bitmap);

    bool isCopied = withRetry(L"image", [&] {
        ClipboardLock lock;
        if (!lock.isOpen || !EmptyClipboard()) return false;
        return setData(pngFormat(), png.data(), png.size()) && setData(CF_DIBV5, dib.data(), dib.size());
    });

    if (isCopied) lastImageWriteAt = nowMs();
    return isCopied;
}

std::wstring readClipboardText() {
    auto data = readFormat(CF_UNICODETEXT);
    if (!data || data->size() < sizeof(wchar_t)) return {};

    std::wstring text(reinterpret_cast<wchar_t const*>(data->data()), data->size() / sizeof(wchar_t));
    return text.substr(0, text.find(L'\0'));
}

std::optional<core::Bitmap> readClipboardImage() {
    if (IsClipboardFormatAvailable(pngFormat())) {
        if (auto png = readFormat(pngFormat())) {
            if (auto image = core::decodeImage(png->data(), png->size())) return image;
        }
    }

    for (UINT format : {static_cast<UINT>(CF_DIBV5), static_cast<UINT>(CF_DIB)}) {
        if (!IsClipboardFormatAvailable(format)) continue;
        if (auto dib = readFormat(format)) return decodeDib(*dib);
    }

    return std::nullopt;
}

std::optional<std::filesystem::path> readClipboardFilePath() {
    if (IsClipboardFormatAvailable(fileNameFormat())) {
        if (auto data = readFormat(fileNameFormat()); data && data->size() >= sizeof(wchar_t)) {
            std::wstring path(reinterpret_cast<wchar_t const*>(data->data()), data->size() / sizeof(wchar_t));
            path = path.substr(0, path.find(L'\0'));
            if (!path.empty()) return std::filesystem::path(path);
        }
    }

    if (!IsClipboardFormatAvailable(CF_HDROP)) return std::nullopt;

    ClipboardLock lock;
    if (!lock.isOpen) return std::nullopt;

    auto drop = static_cast<HDROP>(GetClipboardData(CF_HDROP));
    if (!drop) return std::nullopt;

    UINT length = DragQueryFileW(drop, 0, nullptr, 0);
    if (!length) return std::nullopt;

    std::wstring path(length + 1, L'\0');
    DragQueryFileW(drop, 0, path.data(), length + 1);
    path.resize(length);
    return std::filesystem::path(path);
}

}

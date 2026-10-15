#include "common.h"

#include <thread>

namespace native {

namespace {

void drawCursor(HDC context, Rect const& origin) {
    CURSORINFO info{};
    info.cbSize = sizeof(info);
    if (!GetCursorInfo(&info) || !(info.flags & CURSOR_SHOWING) || !info.hCursor) return;

    ICONINFO icon{};
    bool hasIcon = GetIconInfo(info.hCursor, &icon) != FALSE;

    DrawIconEx(context, info.ptScreenPos.x - origin.x - (hasIcon ? static_cast<int>(icon.xHotspot) : 0),
               info.ptScreenPos.y - origin.y - (hasIcon ? static_cast<int>(icon.yHotspot) : 0), info.hCursor,
               0, 0, 0, nullptr, DI_NORMAL);

    if (hasIcon) {
        if (icon.hbmMask) DeleteObject(icon.hbmMask);
        if (icon.hbmColor) DeleteObject(icon.hbmColor);
    }
}

bool captureInto(Rect const& rect, bool cursor, uint8_t* pixels) {
    HDC screen = GetDC(nullptr);
    HDC memory = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateCompatibleBitmap(screen, rect.width, rect.height);
    HGDIOBJ previous = SelectObject(memory, bitmap);

    bool isCaptured = BitBlt(memory, 0, 0, rect.width, rect.height, screen, rect.x, rect.y,
                             SRCCOPY | CAPTUREBLT) != FALSE;

    if (isCaptured && cursor) drawCursor(memory, rect);

    SelectObject(memory, previous);

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = rect.width;
    info.bmiHeader.biHeight = -rect.height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    if (isCaptured)
        isCaptured = GetDIBits(memory, bitmap, 0, rect.height, pixels, &info, DIB_RGB_COLORS) == rect.height;

    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(nullptr, screen);

    if (!isCaptured) return false;

    size_t length = static_cast<size_t>(rect.width) * rect.height * 4;
    for (size_t index = 3; index < length; index += 4) pixels[index] = 255;

    return true;
}

class CaptureWorker : public Napi::AsyncWorker {
public:
    CaptureWorker(Napi::Env env, std::vector<Rect> rects, bool cursor)
        : Napi::AsyncWorker(env), deferred(Napi::Promise::Deferred::New(env)), rects(std::move(rects)),
          cursor(cursor) {
        auto buffers = Napi::Array::New(env, this->rects.size());

        for (size_t index = 0; index < this->rects.size(); index++) {
            auto const& rect = this->rects[index];
            auto buffer = Napi::Buffer<uint8_t>::New(env, static_cast<size_t>(rect.width) * rect.height * 4);
            buffers.Set(static_cast<uint32_t>(index), buffer);
            targets.push_back(buffer.Data());
        }

        output = Napi::Persistent(buffers);
    }

    Napi::Promise promise() {
        return deferred.Promise();
    }

protected:
    void Execute() override {
        std::vector<char> results(rects.size(), 0);
        std::vector<std::thread> threads;

        for (size_t index = 0; index < rects.size(); index++) {
            threads.emplace_back([&, index] { results[index] = captureInto(rects[index], cursor, targets[index]); });
        }

        for (auto& thread : threads) thread.join();

        for (char result : results) {
            if (!result) return SetError("Could not capture the screen");
        }
    }

    void OnOK() override {
        deferred.Resolve(output.Value());
    }

    void OnError(Napi::Error const& error) override {
        deferred.Reject(error.Value());
    }

private:
    Napi::Promise::Deferred deferred;
    std::vector<Rect> rects;
    bool cursor;
    std::vector<uint8_t*> targets;
    Napi::Reference<Napi::Array> output;
};

Napi::Value captureRects(Napi::CallbackInfo const& info) {
    auto env = info.Env();

    if (!info[0].IsArray()) throw Napi::TypeError::New(env, "Expected an array of rectangles");

    auto list = info[0].As<Napi::Array>();
    std::vector<Rect> rects;

    for (uint32_t index = 0; index < list.Length(); index++) {
        auto rect = readRect(list.Get(index).As<Napi::Object>());
        if (rect.width <= 0 || rect.height <= 0) throw Napi::RangeError::New(env, "Invalid capture area");
        rects.push_back(rect);
    }

    bool cursor = info[1].IsObject() && info[1].As<Napi::Object>().Get("cursor").ToBoolean().Value();

    auto worker = new CaptureWorker(env, std::move(rects), cursor);
    auto promise = worker->promise();
    worker->Queue();
    return promise;
}

}

void registerCapture(Napi::Env env, Napi::Object exports) {
    exports.Set("captureRects", Napi::Function::New(env, captureRects));
}

}

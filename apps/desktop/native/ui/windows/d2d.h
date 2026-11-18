#pragma once

#include <ui/gfx.h>

#include <windows.h>

#include <d2d1_3.h>
#include <d3d11.h>
#include <dcomp.h>
#include <dwrite_3.h>
#include <dxgi1_3.h>

#include <winrt/base.h>

#include <stdexcept>
#include <unordered_map>

namespace fileship::ui {

using winrt::com_ptr;

inline constexpr CLSID SHADOW_EFFECT{0xC67EA361, 0x1863, 0x4E69, {0x89, 0xDB, 0x69, 0x5D, 0x3E, 0x9A, 0x5B, 0x6B}};

template <typename Run> auto guarded(Run&& run) {
    try {
        return run();
    } catch (winrt::hresult_error const& error) {
        throw std::runtime_error(winrt::to_string(error.message()));
    }
}

inline D2D1_COLOR_F d2d(Color const& value) {
    return D2D1_COLOR_F{value.r, value.g, value.b, value.a};
}

inline D2D1_RECT_F d2d(RectF const& value) {
    return D2D1_RECT_F{value.left, value.top, value.right, value.bottom};
}

inline D2D1_POINT_2F d2d(PointF value) {
    return D2D1_POINT_2F{value.x, value.y};
}

inline D2D1_MATRIX_3X2_F d2d(Matrix const& value) {
    return D2D1_MATRIX_3X2_F{value.m11, value.m12, value.m21, value.m22, value.dx, value.dy};
}

class D2DTextLayout : public TextLayout {
public:
    explicit D2DTextLayout(com_ptr<IDWriteTextLayout4> layout) : layout(std::move(layout)) {}

    IDWriteTextLayout4* get() const {
        return layout.get();
    }

    TextMetrics metrics() override;
    float baseline() override;
    TextCaret caret(size_t position) override;
    size_t indexAt(float x, float y) override;
    std::vector<TextRange> ranges(size_t start, size_t length) override;

private:
    com_ptr<IDWriteTextLayout4> layout;
};

class D2DResources;

class D2DPath : public Path {
public:
    D2DPath(com_ptr<ID2D1Geometry> geometry, D2DResources& resources)
        : geometry(std::move(geometry)), resources(resources) {}

    ID2D1Geometry* get() const {
        return geometry.get();
    }

    RectF bounds() override;
    bool fillContains(PointF point) override;
    bool strokeContains(PointF point, float width, Stroke const& stroke) override;

private:
    com_ptr<ID2D1Geometry> geometry;
    D2DResources& resources;
};

class D2DImage : public Image {
public:
    explicit D2DImage(com_ptr<ID2D1Bitmap1> bitmap) : bitmap(std::move(bitmap)) {}

    ID2D1Bitmap1* get() const {
        return bitmap.get();
    }

private:
    com_ptr<ID2D1Bitmap1> bitmap;
};

class D2DResources : public Resources {
public:
    static std::shared_ptr<D2DResources> acquire();

    D2DResources() = default;
    ~D2DResources() override;
    D2DResources(D2DResources const&) = delete;
    D2DResources& operator=(D2DResources const&) = delete;

    ID2D1Factory7* factory() const {
        return d2d.get();
    }

    IDWriteFactory7* dwrite() const {
        return write.get();
    }

    ID2D1PathGeometry* icon(Icon const& icon);
    ID2D1StrokeStyle1* roundStroke();
    ID2D1StrokeStyle1* dashStroke(float dash, float gap, bool round = false);
    ID2D1StrokeStyle1* strokeStyle(Stroke const& stroke);

    std::shared_ptr<TextLayout> layout(std::wstring_view text, float size, FontWeight weight, FontKind kind,
                                       float lineHeight, bool tabular) override;
    FontMetrics metrics(FontKind kind) override;
    std::shared_ptr<Path> path(std::function<void(PathSink&)> const& build) override;
    std::shared_ptr<Path> roundedRect(RectF const& rect, float radius) override;
    std::shared_ptr<Path> ellipse(PointF center, float radiusX, float radiusY) override;

private:
    void loadFonts();
    com_ptr<IDWriteTextFormat3> format(float size, DWRITE_FONT_WEIGHT weight, FontKind kind);

    com_ptr<ID2D1Factory7> d2d;
    com_ptr<IDWriteFactory7> write;
    com_ptr<IDWriteFontCollection1> sans;
    com_ptr<IDWriteFontFallback> fallback;
    com_ptr<IDWriteInMemoryFontFileLoader> fontLoader;
    std::wstring sansFamily = L"Segoe UI";
    std::vector<com_ptr<IDWriteFontCollection1>> subsets;
    std::unordered_map<Icon const*, com_ptr<ID2D1PathGeometry>> icons;
    std::unordered_map<std::wstring, com_ptr<ID2D1StrokeStyle1>> strokes;
    std::unordered_map<std::wstring, com_ptr<IDWriteTextFormat3>> formats;
    FontMetrics sansMetrics;
    FontMetrics emojiMetrics;
    bool metricsReady = false;
};

class D2DPainter : public Painter {
public:
    D2DPainter(ID2D1DeviceContext* context, D2DResources& resources);

    ID2D1DeviceContext* context() const {
        return target;
    }

    ID2D1SolidColorBrush* brush(Color const& value);

    Resources& resources() override;

    void clear(Color const& value) override;
    void fillRect(RectF const& rect, Color const& value) override;
    void strokeRect(RectF const& rect, Color const& value, float width, Stroke const& stroke) override;
    void fillFrame(RectF const& outer, RectF const& inner, Color const& value) override;
    void fillRounded(RectF const& rect, float radius, Color const& value) override;
    void strokeRounded(RectF const& rect, float radius, Color const& value, float width, Stroke const& stroke) override;
    void fillEllipse(float cx, float cy, float rx, float ry, Color const& value) override;
    void strokeEllipse(float cx, float cy, float rx, float ry, Color const& value, float width) override;
    void line(PointF from, PointF to, Color const& value, float width) override;
    void fillPath(Path& path, Color const& value) override;
    void strokePath(Path& path, Color const& value, float width, Stroke const& stroke) override;
    void icon(Icon const& icon, float x, float y, float size, Color const& value, bool fill, float strokeWidth) override;
    void text(TextLayout& layout, float x, float y, Color const& value) override;
    void boxShadow(RectF const& rect, float radius, float offsetY, float blur, float spread, float alpha) override;
    void shadowed(float blur, float offset, float alpha, std::function<void()> const& draw) override;

    std::shared_ptr<Image> image(uint8_t const* bgra, int width, int height) override;
    void drawImage(Image& image, RectF const& target) override;
    void drawPixels(Image& image, RectF const& source, RectF const& target) override;

    void pushOpacity(float opacity) override;
    void popOpacity() override;
    void pushClip(Path& path) override;
    void popClip() override;

    Matrix transform() override;
    void setTransform(Matrix const& matrix) override;

private:
    ID2D1DeviceContext* target;
    D2DResources& res;
    com_ptr<ID2D1SolidColorBrush> solid;
};

struct Gpu {
    com_ptr<ID3D11Device> d3d;
    com_ptr<IDXGIDevice> dxgi;
    com_ptr<ID2D1Device6> device;
    com_ptr<ID2D1DeviceContext5> context;
    com_ptr<IDCompositionDevice> composition;

    static std::unique_ptr<Gpu> create(D2DResources& resources);
};

class SwapSurface {
public:
    bool create(Gpu& gpu, HWND window, int width, int height, bool transparent);
    ID2D1DeviceContext5* begin(Gpu& gpu);
    void present(Gpu& gpu);
    void release();

private:
    com_ptr<IDXGISwapChain1> chain;
    com_ptr<IDCompositionTarget> target;
    com_ptr<IDCompositionVisual> visual;
    com_ptr<ID2D1Bitmap1> buffer;
    bool committed = false;
};

class D2DCanvas : public Canvas {
public:
    explicit D2DCanvas(D2DResources& resources);

    Gpu& gpu() {
        return *device;
    }

    D2DResources& resources() {
        return res;
    }

    std::shared_ptr<Image> image(uint8_t const* bgra, int width, int height) override;
    std::vector<uint8_t> read(Image& image, PixelRect const& rect) override;
    std::vector<uint8_t> render(int width, int height, std::function<void(Painter&)> const& draw) override;

private:
    D2DResources& res;
    std::unique_ptr<Gpu> device;
};

std::vector<uint8_t> readBitmap(ID2D1DeviceContext* context, ID2D1Bitmap1* source, D2D1_RECT_U const& rect);

}

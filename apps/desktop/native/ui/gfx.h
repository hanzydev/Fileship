#pragma once

#include <windows.h>

#include <d2d1_3.h>
#include <d3d11.h>
#include <dcomp.h>
#include <dwrite_3.h>
#include <dxgi1_3.h>

#include <winrt/base.h>

#include <icons.h>
#include <theme.h>

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace fileship::ui {

using winrt::com_ptr;

inline constexpr CLSID SHADOW_EFFECT{0xC67EA361, 0x1863, 0x4E69, {0x89, 0xDB, 0x69, 0x5D, 0x3E, 0x9A, 0x5B, 0x6B}};

struct Box {
    double x = 0;
    double y = 0;
    double width = 0;
    double height = 0;

    double right() const {
        return x + width;
    }

    double bottom() const {
        return y + height;
    }

    bool contains(double px, double py) const {
        return px >= x && py >= y && px < x + width && py < y + height;
    }

    D2D1_RECT_F rect() const {
        return D2D1::RectF(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x + width),
                           static_cast<float>(y + height));
    }
};

enum class FontKind { Sans, Emoji };

struct FontMetrics {
    float ascent = 0.9f;
    float descent = 0.25f;
};

class Resources {
public:
    static std::shared_ptr<Resources> acquire();

    Resources() = default;
    ~Resources();
    Resources(Resources const&) = delete;
    Resources& operator=(Resources const&) = delete;

    ID2D1Factory7* factory() const {
        return d2d.get();
    }

    IDWriteFactory7* dwrite() const {
        return write.get();
    }

    ID2D1PathGeometry* icon(Icon const& icon);
    ID2D1StrokeStyle1* roundStroke();
    ID2D1StrokeStyle1* dashStroke(float dash, float gap, bool round = false);

    com_ptr<IDWriteTextLayout4> layout(std::wstring_view text, float size, DWRITE_FONT_WEIGHT weight,
                                       FontKind kind = FontKind::Sans, float lineHeight = 0, bool tabular = false);
    FontMetrics metrics(FontKind kind);
    float measure(std::wstring_view text, float size, DWRITE_FONT_WEIGHT weight, FontKind kind = FontKind::Sans,
                  bool tabular = false);

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

void setResourcesPath(std::filesystem::path const& path);
std::filesystem::path resourcesPath();

com_ptr<ID2D1PathGeometry> parseSvgPath(ID2D1Factory* factory, char const* data);

D2D1_COLOR_F color(ThemeColor const& value, float opacity = 1);
D2D1_COLOR_F withAlpha(D2D1_COLOR_F value, float opacity);
D2D1_COLOR_F rgba(int r, int g, int b, float a = 1);
D2D1_COLOR_F parseHex(std::string_view hex);
std::string toHex(D2D1_COLOR_F const& value);
D2D1_COLOR_F mix(D2D1_COLOR_F a, D2D1_COLOR_F b, float t);

double easeOutCubic(double t);
double easeOut(double t);
double nowMs();

class Painter {
public:
    Painter(ID2D1DeviceContext* context, Resources& resources);

    ID2D1DeviceContext* context() const {
        return target;
    }

    Resources& resources() const {
        return res;
    }

    ID2D1SolidColorBrush* brush(D2D1_COLOR_F const& value);

    void fillRect(D2D1_RECT_F const& rect, D2D1_COLOR_F const& value);
    void fillRounded(D2D1_RECT_F const& rect, float radius, D2D1_COLOR_F const& value);
    void strokeRounded(D2D1_RECT_F const& rect, float radius, D2D1_COLOR_F const& value, float width,
                       ID2D1StrokeStyle* style = nullptr);
    void fillEllipse(float cx, float cy, float rx, float ry, D2D1_COLOR_F const& value);
    void strokeEllipse(float cx, float cy, float rx, float ry, D2D1_COLOR_F const& value, float width);
    void icon(Icon const& icon, float x, float y, float size, D2D1_COLOR_F const& value, bool fill = false,
              float strokeWidth = 2);
    void text(IDWriteTextLayout* layout, float x, float y, D2D1_COLOR_F const& value);
    void textAt(std::wstring_view text, float x, float baseline, float size, DWRITE_FONT_WEIGHT weight,
                D2D1_COLOR_F const& value, FontKind kind = FontKind::Sans);
    void boxShadow(D2D1_RECT_F const& rect, float radius, float offsetY, float blur, float spread, float alpha);
    void ring(D2D1_RECT_F const& rect, float radius, float width, D2D1_COLOR_F const& value);
    float middleBaseline(float centerY, float size, FontKind kind = FontKind::Sans);

    void pushOpacity(float opacity);
    void popOpacity();

private:
    ID2D1DeviceContext* target;
    Resources& res;
    com_ptr<ID2D1SolidColorBrush> solid;
    std::vector<com_ptr<ID2D1Layer>> layers;
    int layerDepth = 0;
};

class Transform {
public:
    Transform(ID2D1DeviceContext* context, D2D1_MATRIX_3X2_F const& local) : target(context) {
        target->GetTransform(&saved);
        target->SetTransform(local * saved);
    }

    ~Transform() {
        target->SetTransform(saved);
    }

    Transform(Transform const&) = delete;
    Transform& operator=(Transform const&) = delete;

private:
    ID2D1DeviceContext* target;
    D2D1_MATRIX_3X2_F saved{};
};

struct Gpu {
    com_ptr<ID3D11Device> d3d;
    com_ptr<IDXGIDevice> dxgi;
    com_ptr<ID2D1Device6> device;
    com_ptr<ID2D1DeviceContext5> context;
    com_ptr<IDCompositionDevice> composition;

    static std::unique_ptr<Gpu> create(Resources& resources);
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

std::vector<uint8_t> readBitmap(ID2D1DeviceContext* context, ID2D1Bitmap1* source, D2D1_RECT_U const& rect);

}

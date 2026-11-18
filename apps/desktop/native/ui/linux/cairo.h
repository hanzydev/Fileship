#pragma once

#include <ui/gfx.h>

#include <cairo.h>
#include <pango/pangocairo.h>

#include <map>
#include <string>
#include <unordered_map>

namespace fileship::ui {

class CairoResources;

class PangoTextLayout : public TextLayout {
public:
    PangoTextLayout(PangoLayout* layout, std::string text) : layout(layout), text(std::move(text)) {}
    ~PangoTextLayout() override;

    PangoLayout* get() const {
        return layout;
    }

    TextMetrics metrics() override;
    float baseline() override;
    TextCaret caret(size_t position) override;
    size_t indexAt(float x, float y) override;
    std::vector<TextRange> ranges(size_t start, size_t length) override;

private:
    int byteOf(size_t position) const;

    PangoLayout* layout;
    std::string text;
};

class CairoPath : public Path {
public:
    CairoPath(cairo_path_t* path, CairoResources& resources) : path(path), resources(resources) {}
    ~CairoPath() override;

    cairo_path_t* get() const {
        return path;
    }

    RectF bounds() override;
    bool fillContains(PointF point) override;
    bool strokeContains(PointF point, float width, Stroke const& stroke) override;

private:
    cairo_path_t* path;
    CairoResources& resources;
};

class CairoImage : public Image {
public:
    explicit CairoImage(cairo_surface_t* surface) : surface(surface) {}
    ~CairoImage() override;

    cairo_surface_t* get() const {
        return surface;
    }

private:
    cairo_surface_t* surface;
};

class CairoResources : public Resources {
public:
    CairoResources();
    ~CairoResources() override;
    CairoResources(CairoResources const&) = delete;
    CairoResources& operator=(CairoResources const&) = delete;

    cairo_t* scratch() const {
        return measure;
    }

    cairo_path_t* icon(Icon const& icon);
    cairo_surface_t* shadow(int width, int height, float radius, float sigma);

    std::shared_ptr<TextLayout> layout(std::wstring_view text, float size, FontWeight weight, FontKind kind,
                                       float lineHeight, bool tabular) override;
    FontMetrics metrics(FontKind kind) override;
    std::shared_ptr<Path> path(std::function<void(PathSink&)> const& build) override;
    std::shared_ptr<Path> roundedRect(RectF const& rect, float radius) override;
    std::shared_ptr<Path> ellipse(PointF center, float radiusX, float radiusY) override;

private:
    PangoFontDescription* describe(float size, FontWeight weight, FontKind kind);

    cairo_surface_t* surface = nullptr;
    cairo_t* measure = nullptr;
    PangoContext* context = nullptr;
    std::string sansFamily = "sans-serif";
    std::unordered_map<Icon const*, cairo_path_t*> icons;
    std::map<std::string, cairo_surface_t*> shadows;
    FontMetrics sansMetrics;
    FontMetrics emojiMetrics;
    bool metricsReady = false;
};

class CairoPainter : public Painter {
public:
    CairoPainter(cairo_t* context, CairoResources& resources);

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
    struct Layer {
        cairo_t* previous;
        cairo_surface_t* recording;
        float opacity;
    };

    void source(Color const& value);
    void pixels();
    void stroke(float width, Stroke const& style);
    cairo_t* record(cairo_surface_t*& recording);
    double pixelScale();

    cairo_t* cr;
    CairoResources& res;
    double device = 1;
    std::vector<Layer> layers;
};

class CairoCanvas : public Canvas {
public:
    explicit CairoCanvas(CairoResources& resources) : res(resources) {}

    CairoResources& resources() {
        return res;
    }

    std::shared_ptr<Image> image(uint8_t const* bgra, int width, int height) override;
    std::vector<uint8_t> read(Image& image, PixelRect const& rect) override;
    std::vector<uint8_t> render(int width, int height, std::function<void(Painter&)> const& draw) override;

private:
    CairoResources& res;
};

}

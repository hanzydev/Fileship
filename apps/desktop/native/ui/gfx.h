#pragma once

#include <icons.h>
#include <theme.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace fileship::ui {

struct Color {
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 1;

    Color() = default;
    Color(float r, float g, float b, float a = 1) : r(r), g(g), b(b), a(a) {}
};

struct PointF {
    float x = 0;
    float y = 0;

    PointF() = default;
    PointF(float x, float y) : x(x), y(y) {}
};

struct SizeF {
    float width = 0;
    float height = 0;

    SizeF() = default;
    SizeF(float width, float height) : width(width), height(height) {}
};

struct RectF {
    float left = 0;
    float top = 0;
    float right = 0;
    float bottom = 0;

    RectF() = default;
    RectF(float left, float top, float right, float bottom) : left(left), top(top), right(right), bottom(bottom) {}
};

struct PixelRect {
    int32_t left = 0;
    int32_t top = 0;
    int32_t right = 0;
    int32_t bottom = 0;
};

struct Matrix {
    float m11 = 1;
    float m12 = 0;
    float m21 = 0;
    float m22 = 1;
    float dx = 0;
    float dy = 0;

    static Matrix scale(float x, float y, PointF center = {});
    static Matrix translation(float x, float y);
    static Matrix rotation(float degrees, PointF center = {});

    Matrix operator*(Matrix const& next) const;
};

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

    RectF rect() const {
        return RectF{static_cast<float>(x), static_cast<float>(y), static_cast<float>(x + width),
                     static_cast<float>(y + height)};
    }
};

enum class FontKind { Sans, Emoji };

enum class FontWeight { Normal = 400, Medium = 500, SemiBold = 600, Bold = 700 };

struct FontMetrics {
    float ascent = 0.9f;
    float descent = 0.25f;
};

struct Stroke {
    bool isRound = false;
    float dash = 0;
    float gap = 0;
};

struct TextMetrics {
    float width = 0;
    uint32_t lineCount = 0;
};

struct TextCaret {
    float x = 0;
    float y = 0;
    float height = 0;
};

struct TextRange {
    float left = 0;
    float top = 0;
    float width = 0;
    float height = 0;
};

class TextLayout {
public:
    virtual ~TextLayout() = default;

    virtual TextMetrics metrics() = 0;
    virtual float baseline() = 0;
    virtual TextCaret caret(size_t position) = 0;
    virtual size_t indexAt(float x, float y) = 0;
    virtual std::vector<TextRange> ranges(size_t start, size_t length) = 0;
};

class PathSink {
public:
    virtual ~PathSink() = default;

    virtual void begin(PointF point, bool isFilled) = 0;
    virtual void lineTo(PointF point) = 0;
    virtual void quadTo(PointF control, PointF end) = 0;
    virtual void cubicTo(PointF first, PointF second, PointF end) = 0;
    virtual void arcTo(PointF end, SizeF radius, float rotation, bool isClockwise, bool isLarge) = 0;
    virtual void end(bool isClosed) = 0;
};

class Path {
public:
    virtual ~Path() = default;

    virtual RectF bounds() = 0;
    virtual bool fillContains(PointF point) = 0;
    virtual bool strokeContains(PointF point, float width, Stroke const& stroke = {}) = 0;
};

class Image {
public:
    virtual ~Image() = default;
};

class Resources {
public:
    static std::shared_ptr<Resources> acquire();

    virtual ~Resources() = default;

    virtual std::shared_ptr<TextLayout> layout(std::wstring_view text, float size, FontWeight weight,
                                               FontKind kind = FontKind::Sans, float lineHeight = 0,
                                               bool tabular = false) = 0;
    virtual FontMetrics metrics(FontKind kind) = 0;
    virtual std::shared_ptr<Path> path(std::function<void(PathSink&)> const& build) = 0;
    virtual std::shared_ptr<Path> roundedRect(RectF const& rect, float radius) = 0;
    virtual std::shared_ptr<Path> ellipse(PointF center, float radiusX, float radiusY) = 0;

    float measure(std::wstring_view text, float size, FontWeight weight, FontKind kind = FontKind::Sans,
                  bool tabular = false);
};

void setResourcesPath(std::filesystem::path const& path);
std::filesystem::path resourcesPath();
std::filesystem::path executableDirectory();

void parseSvgPath(PathSink& sink, char const* data);

Color color(ThemeColor const& value, float opacity = 1);
Color withAlpha(Color value, float opacity);
Color rgba(int r, int g, int b, float a = 1);
Color parseHex(std::string_view hex);
std::string toHex(Color const& value);
Color mix(Color a, Color b, float t);

double easeOutCubic(double t);
double easeOut(double t);
double nowMs();

class Painter {
public:
    virtual ~Painter() = default;

    virtual Resources& resources() = 0;

    virtual void clear(Color const& value) = 0;
    virtual void fillRect(RectF const& rect, Color const& value) = 0;
    virtual void strokeRect(RectF const& rect, Color const& value, float width, Stroke const& stroke = {}) = 0;
    virtual void fillFrame(RectF const& outer, RectF const& inner, Color const& value) = 0;
    virtual void fillRounded(RectF const& rect, float radius, Color const& value) = 0;
    virtual void strokeRounded(RectF const& rect, float radius, Color const& value, float width,
                               Stroke const& stroke = {}) = 0;
    virtual void fillEllipse(float cx, float cy, float rx, float ry, Color const& value) = 0;
    virtual void strokeEllipse(float cx, float cy, float rx, float ry, Color const& value, float width) = 0;
    virtual void line(PointF from, PointF to, Color const& value, float width) = 0;
    virtual void fillPath(Path& path, Color const& value) = 0;
    virtual void strokePath(Path& path, Color const& value, float width, Stroke const& stroke = {}) = 0;
    virtual void icon(Icon const& icon, float x, float y, float size, Color const& value, bool fill = false,
                      float strokeWidth = 2) = 0;
    virtual void text(TextLayout& layout, float x, float y, Color const& value) = 0;
    virtual void boxShadow(RectF const& rect, float radius, float offsetY, float blur, float spread, float alpha) = 0;
    virtual void shadowed(float blur, float offset, float alpha, std::function<void()> const& draw) = 0;

    virtual std::shared_ptr<Image> image(uint8_t const* bgra, int width, int height) = 0;
    virtual void drawImage(Image& image, RectF const& target) = 0;
    virtual void drawPixels(Image& image, RectF const& source, RectF const& target) = 0;

    virtual void pushOpacity(float opacity) = 0;
    virtual void popOpacity() = 0;
    virtual void pushClip(Path& path) = 0;
    virtual void popClip() = 0;

    virtual Matrix transform() = 0;
    virtual void setTransform(Matrix const& matrix) = 0;

    void textAt(std::wstring_view text, float x, float baseline, float size, FontWeight weight, Color const& value,
                FontKind kind = FontKind::Sans);
    void ring(RectF const& rect, float radius, float width, Color const& value);
    float middleBaseline(float centerY, float size, FontKind kind = FontKind::Sans);
};

class Canvas {
public:
    static std::unique_ptr<Canvas> create(Resources& resources);

    virtual ~Canvas() = default;

    virtual std::shared_ptr<Image> image(uint8_t const* bgra, int width, int height) = 0;
    virtual std::vector<uint8_t> read(Image& image, PixelRect const& rect) = 0;
    virtual std::vector<uint8_t> render(int width, int height, std::function<void(Painter&)> const& draw) = 0;
};

class Transform {
public:
    Transform(Painter& painter, Matrix const& local) : painter(painter), saved(painter.transform()) {
        painter.setTransform(local * saved);
    }

    ~Transform() {
        painter.setTransform(saved);
    }

    Transform(Transform const&) = delete;
    Transform& operator=(Transform const&) = delete;

private:
    Painter& painter;
    Matrix saved;
};

}

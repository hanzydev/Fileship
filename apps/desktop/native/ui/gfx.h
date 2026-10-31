#pragma once

#include <icons.h>
#include <theme.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace fileship::ui {

struct PointF {
    float x = 0;
    float y = 0;

    constexpr PointF() = default;
    constexpr PointF(float x, float y) : x(x), y(y) {}
};

struct SizeF {
    float width = 0;
    float height = 0;

    constexpr SizeF() = default;
    constexpr SizeF(float width, float height) : width(width), height(height) {}
};

struct RectF {
    float left = 0;
    float top = 0;
    float right = 0;
    float bottom = 0;

    constexpr RectF() = default;
    constexpr RectF(float left, float top, float right, float bottom)
        : left(left), top(top), right(right), bottom(bottom) {}
};

struct PixelRect {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
};

struct Color {
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 1;

    constexpr Color() = default;
    constexpr Color(float r, float g, float b, float a = 1) : r(r), g(g), b(b), a(a) {}
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
    PointF apply(PointF point) const;
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
        return RectF(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x + width),
                     static_cast<float>(y + height));
    }
};

enum class FontKind { Sans, Emoji };

enum class FontWeight { Normal = 400, Medium = 500, Bold = 700 };

struct FontMetrics {
    float ascent = 0.9f;
    float descent = 0.25f;
};

struct TextMetrics {
    float width = 0;
    float widthIncludingTrailingWhitespace = 0;
    float height = 0;
    uint32_t lineCount = 1;
};

struct TextPosition {
    float x = 0;
    float y = 0;
    float height = 0;
};

class TextLayout {
public:
    virtual ~TextLayout() = default;

    virtual TextMetrics metrics() const = 0;
    virtual float baseline() const = 0;
    virtual TextPosition position(size_t index) const = 0;
    virtual std::vector<RectF> rangeRects(size_t start, size_t length) const = 0;
    virtual size_t indexAt(float x, float y) const = 0;
};

enum class FillMode { Winding, Alternate };

class Path {
public:
    enum class Verb { Move, Line, Quad, Cubic, Arc, Close };

    struct Segment {
        Verb verb;
        PointF points[3];
        float rx = 0;
        float ry = 0;
        float rotation = 0;
        bool large = false;
        bool sweep = false;
    };

    void moveTo(PointF point);
    void lineTo(PointF point);
    void quadTo(PointF control, PointF point);
    void cubicTo(PointF first, PointF second, PointF point);
    void arcTo(PointF point, float rx, float ry, float rotation, bool large, bool sweep);
    void close();
    void addRect(RectF const& rect);
    void addRoundedRect(RectF const& rect, float radius);
    void addEllipse(float cx, float cy, float rx, float ry);

    FillMode fill = FillMode::Winding;
    std::vector<Segment> segments;
    mutable std::shared_ptr<void> compiled;
};

enum class CapStyle { Flat, Round };

struct StrokeStyle {
    CapStyle cap = CapStyle::Flat;
    bool roundJoin = false;
    float dash = 0;
    float gap = 0;

    static StrokeStyle round();
    static StrokeStyle dashed(float dash, float gap, bool round = false);
};

enum class Interpolation { Nearest, Linear };

class Image {
public:
    virtual ~Image() = default;

    virtual int width() const = 0;
    virtual int height() const = 0;
};

class Resources;

class Painter {
public:
    explicit Painter(Resources& resources) : res(resources) {}
    virtual ~Painter() = default;
    Painter(Painter const&) = delete;
    Painter& operator=(Painter const&) = delete;

    Resources& resources() const {
        return res;
    }

    virtual void clear(Color const& value) = 0;
    virtual void fillRect(RectF const& rect, Color const& value) = 0;
    virtual void strokeRect(RectF const& rect, Color const& value, float width,
                            StrokeStyle const* style = nullptr) = 0;
    virtual void fillRounded(RectF const& rect, float radius, Color const& value) = 0;
    virtual void strokeRounded(RectF const& rect, float radius, Color const& value, float width,
                               StrokeStyle const* style = nullptr) = 0;
    virtual void fillEllipse(float cx, float cy, float rx, float ry, Color const& value) = 0;
    virtual void strokeEllipse(float cx, float cy, float rx, float ry, Color const& value, float width) = 0;
    virtual void line(PointF from, PointF to, Color const& value, float width) = 0;
    virtual void fillPath(Path const& path, Color const& value) = 0;
    virtual void strokePath(Path const& path, Color const& value, float width, StrokeStyle const& style) = 0;
    virtual void text(TextLayout const& layout, float x, float y, Color const& value) = 0;
    virtual void image(Image const& source, RectF const& destination, std::optional<RectF> const& cut,
                       Interpolation interpolation) = 0;
    virtual std::shared_ptr<Image> createImage(uint8_t const* bgra, int width, int height, bool isOpaque) = 0;
    virtual void boxShadow(RectF const& rect, float radius, float offsetY, float blur, float spread,
                           float alpha) = 0;

    virtual void pushOpacity(float opacity) = 0;
    virtual void popOpacity() = 0;
    virtual void pushClip(RectF const& rect, float radius) = 0;
    virtual void popClip() = 0;
    virtual void pushTransform(Matrix const& local) = 0;
    virtual void popTransform() = 0;
    virtual Matrix transform() const = 0;
    virtual void beginShadow() = 0;
    virtual void endShadow(float offsetX, float offsetY, float blur, float alpha) = 0;

    void icon(Icon const& icon, float x, float y, float size, Color const& value, bool fill = false,
              float strokeWidth = 2);
    void textAt(std::wstring_view text, float x, float baseline, float size, FontWeight weight, Color const& value,
                FontKind kind = FontKind::Sans);
    void ring(RectF const& rect, float radius, float width, Color const& value);
    float middleBaseline(float centerY, float size, FontKind kind = FontKind::Sans);

private:
    Resources& res;
};

class Transform {
public:
    Transform(Painter& painter, Matrix const& local) : painter(painter) {
        painter.pushTransform(local);
    }

    ~Transform() {
        painter.popTransform();
    }

    Transform(Transform const&) = delete;
    Transform& operator=(Transform const&) = delete;

private:
    Painter& painter;
};

class Canvas {
public:
    virtual ~Canvas() = default;

    virtual Painter& painter() = 0;
    virtual std::vector<uint8_t> finish() = 0;
};

class Device {
public:
    static std::unique_ptr<Device> create(Resources& resources);

    virtual ~Device() = default;

    virtual std::shared_ptr<Image> createImage(uint8_t const* bgra, int width, int height, bool isOpaque) = 0;
    virtual std::vector<uint8_t> readImage(Image const& image, PixelRect const& rect) = 0;
    virtual std::unique_ptr<Canvas> createCanvas(int width, int height) = 0;
};

class Resources {
public:
    static std::shared_ptr<Resources> acquire();

    Resources();
    ~Resources();
    Resources(Resources const&) = delete;
    Resources& operator=(Resources const&) = delete;

    std::shared_ptr<TextLayout> layout(std::wstring_view text, float size, FontWeight weight,
                                       FontKind kind = FontKind::Sans, float lineHeight = 0, bool tabular = false);
    FontMetrics metrics(FontKind kind);
    float measure(std::wstring_view text, float size, FontWeight weight, FontKind kind = FontKind::Sans,
                  bool tabular = false);
    Path const& icon(Icon const& icon);
    RectF bounds(Path const& path);
    bool fillContains(Path const& path, PointF point);
    bool strokeContains(Path const& path, PointF point, float width, StrokeStyle const* style = nullptr);

    struct Backend;
    Backend& backend() const {
        return *state;
    }

private:
    std::unique_ptr<Backend> state;
    std::unordered_map<Icon const*, Path> icons;
};

void setResourcesPath(std::filesystem::path const& path);
std::filesystem::path resourcesPath();
std::filesystem::path executableDirectory();
double caretBlinkMs();

Path parseSvgPath(char const* data);

Color color(ThemeColor const& value, float opacity = 1);
Color withAlpha(Color value, float opacity);
Color rgba(int r, int g, int b, float a = 1);
Color parseHex(std::string_view hex);
std::string toHex(Color const& value);
Color mix(Color a, Color b, float t);

double easeOutCubic(double t);
double easeOut(double t);
double nowMs();

}

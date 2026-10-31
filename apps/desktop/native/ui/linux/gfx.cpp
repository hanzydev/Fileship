#include "cairo.h"

#include <ui/effects.h>
#include <shared/utf8.h>

#include <fontconfig/fontconfig.h>
#include <gtk/gtk.h>
#include <pango/pangocairo.h>

#include <fonts.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>

namespace fileship::ui {

namespace {

constexpr char const* SANS_FAMILY = "Geist, sans-serif";
constexpr char const* EMOJI_FAMILY = "Noto Color Emoji, emoji";

size_t byteOffset(std::string const& text, size_t index) {
    auto end = g_utf8_offset_to_pointer(text.c_str(), static_cast<glong>(std::min<size_t>(
                                                           index, g_utf8_strlen(text.c_str(), static_cast<gssize>(text.size())))));
    return static_cast<size_t>(end - text.c_str());
}

size_t characterOffset(std::string const& text, size_t bytes) {
    return static_cast<size_t>(g_utf8_pointer_to_offset(text.c_str(), text.c_str() + std::min(bytes, text.size())));
}

void setColor(cairo_t* context, Color const& value) {
    cairo_set_source_rgba(context, value.r, value.g, value.b, value.a);
}

void roundedRect(cairo_t* context, RectF const& rect, float radius) {
    float limit = std::min(rect.right - rect.left, rect.bottom - rect.top) / 2;
    radius = std::max(0.0f, std::min(radius, limit));
    if (radius <= 0) {
        cairo_rectangle(context, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
        return;
    }

    constexpr double QUARTER = std::numbers::pi / 2;
    cairo_new_sub_path(context);
    cairo_arc(context, rect.right - radius, rect.top + radius, radius, -QUARTER, 0);
    cairo_arc(context, rect.right - radius, rect.bottom - radius, radius, 0, QUARTER);
    cairo_arc(context, rect.left + radius, rect.bottom - radius, radius, QUARTER, 2 * QUARTER);
    cairo_arc(context, rect.left + radius, rect.top + radius, radius, 2 * QUARTER, 3 * QUARTER);
    cairo_close_path(context);
}

void ellipse(cairo_t* context, float cx, float cy, float rx, float ry) {
    cairo_save(context);
    cairo_translate(context, cx, cy);
    cairo_scale(context, std::max(rx, 0.001f), std::max(ry, 0.001f));
    cairo_new_sub_path(context);
    cairo_arc(context, 0, 0, 1, 0, 2 * std::numbers::pi);
    cairo_close_path(context);
    cairo_restore(context);
}

void svgArc(cairo_t* context, PointF from, Path::Segment const& segment) {
    auto to = segment.points[0];
    double rx = std::abs(segment.rx);
    double ry = std::abs(segment.ry);
    double phi = segment.rotation * std::numbers::pi / 180;
    double cosine = std::cos(phi);
    double sine = std::sin(phi);

    double dx = (from.x - to.x) / 2;
    double dy = (from.y - to.y) / 2;
    double x1 = cosine * dx + sine * dy;
    double y1 = -sine * dx + cosine * dy;

    double lambda = (x1 * x1) / (rx * rx) + (y1 * y1) / (ry * ry);
    if (lambda > 1) {
        rx *= std::sqrt(lambda);
        ry *= std::sqrt(lambda);
    }

    double numerator = rx * rx * ry * ry - rx * rx * y1 * y1 - ry * ry * x1 * x1;
    double denominator = rx * rx * y1 * y1 + ry * ry * x1 * x1;
    double factor = std::sqrt(std::max(0.0, numerator / denominator)) * (segment.large == segment.sweep ? -1 : 1);
    double cx1 = factor * rx * y1 / ry;
    double cy1 = -factor * ry * x1 / rx;

    double cx = cosine * cx1 - sine * cy1 + (from.x + to.x) / 2;
    double cy = sine * cx1 + cosine * cy1 + (from.y + to.y) / 2;

    auto angle = [](double ux, double uy, double vx, double vy) {
        double value = std::atan2(ux * vy - uy * vx, ux * vx + uy * vy);
        return value;
    };

    double start = angle(1, 0, (x1 - cx1) / rx, (y1 - cy1) / ry);
    double delta = angle((x1 - cx1) / rx, (y1 - cy1) / ry, (-x1 - cx1) / rx, (-y1 - cy1) / ry);
    if (!segment.sweep && delta > 0) delta -= 2 * std::numbers::pi;
    if (segment.sweep && delta < 0) delta += 2 * std::numbers::pi;

    cairo_save(context);
    cairo_translate(context, cx, cy);
    cairo_rotate(context, phi);
    cairo_scale(context, rx, ry);
    if (segment.sweep) cairo_arc(context, 0, 0, 1, start, start + delta);
    else cairo_arc_negative(context, 0, 0, 1, start, start + delta);
    cairo_restore(context);
}

void tracePath(cairo_t* context, Path const& path) {
    cairo_new_path(context);
    PointF current{};
    PointF start{};

    for (auto const& segment : path.segments) {
        auto const& points = segment.points;
        switch (segment.verb) {
        case Path::Verb::Move:
            cairo_move_to(context, points[0].x, points[0].y);
            current = start = points[0];
            break;
        case Path::Verb::Line:
            cairo_line_to(context, points[0].x, points[0].y);
            current = points[0];
            break;
        case Path::Verb::Quad: {
            double x1 = current.x + 2.0 / 3 * (points[0].x - current.x);
            double y1 = current.y + 2.0 / 3 * (points[0].y - current.y);
            double x2 = points[1].x + 2.0 / 3 * (points[0].x - points[1].x);
            double y2 = points[1].y + 2.0 / 3 * (points[0].y - points[1].y);
            cairo_curve_to(context, x1, y1, x2, y2, points[1].x, points[1].y);
            current = points[1];
            break;
        }
        case Path::Verb::Cubic:
            cairo_curve_to(context, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
            current = points[2];
            break;
        case Path::Verb::Arc:
            svgArc(context, current, segment);
            current = points[0];
            break;
        case Path::Verb::Close:
            cairo_close_path(context);
            current = start;
            break;
        }
    }

    cairo_set_fill_rule(context,
                        path.fill == FillMode::Alternate ? CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);
}

void applyStroke(cairo_t* context, float width, StrokeStyle const* style) {
    cairo_set_line_width(context, width);
    cairo_set_line_cap(context, style && style->cap == CapStyle::Round ? CAIRO_LINE_CAP_ROUND : CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join(context, style && style->roundJoin ? CAIRO_LINE_JOIN_ROUND : CAIRO_LINE_JOIN_MITER);
    cairo_set_miter_limit(context, 10);

    if (style && style->dash > 0) {
        double dashes[] = {style->dash * width, style->gap * width};
        cairo_set_dash(context, dashes, 2, 0);
    } else {
        cairo_set_dash(context, nullptr, 0, 0);
    }
}

void strokeCurrent(cairo_t* context, float width, StrokeStyle const* style) {
    cairo_save(context);
    if (style && style->dash > 0) cairo_identity_matrix(context);
    applyStroke(context, width, style);
    cairo_stroke(context);
    cairo_restore(context);
}

std::vector<uint8_t> blurredAlpha(cairo_surface_t* surface, int& left, int& top, int& width, int& height,
                                  int radius) {
    cairo_surface_flush(surface);
    int surfaceWidth = cairo_image_surface_get_width(surface);
    int surfaceHeight = cairo_image_surface_get_height(surface);
    int stride = cairo_image_surface_get_stride(surface);
    auto data = cairo_image_surface_get_data(surface);

    int minX = surfaceWidth;
    int minY = surfaceHeight;
    int maxX = -1;
    int maxY = -1;
    for (int y = 0; y < surfaceHeight; y++) {
        auto row = reinterpret_cast<uint32_t const*>(data + static_cast<size_t>(y) * stride);
        for (int x = 0; x < surfaceWidth; x++) {
            if (!(row[x] >> 24)) continue;
            minX = std::min(minX, x);
            minY = std::min(minY, y);
            maxX = std::max(maxX, x);
            maxY = std::max(maxY, y);
        }
    }

    if (maxX < 0) {
        width = height = 0;
        return {};
    }

    left = minX - radius * 2;
    top = minY - radius * 2;
    width = maxX - minX + 1 + radius * 4;
    height = maxY - minY + 1 + radius * 4;

    std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4, 0);
    for (int y = 0; y < height; y++) {
        int sourceY = top + y;
        if (sourceY < 0 || sourceY >= surfaceHeight) continue;
        auto row = reinterpret_cast<uint32_t const*>(data + static_cast<size_t>(sourceY) * stride);
        for (int x = 0; x < width; x++) {
            int sourceX = left + x;
            if (sourceX < 0 || sourceX >= surfaceWidth) continue;
            pixels[(static_cast<size_t>(y) * width + x) * 4 + 3] = static_cast<uint8_t>(row[sourceX] >> 24);
        }
    }

    if (radius > 0) stackBlur(pixels.data(), width, height, radius);
    return pixels;
}

void paintShadow(cairo_t* context, cairo_surface_t* group, double offsetX, double offsetY, double sigma,
                 float alpha) {
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;
    auto pixels = blurredAlpha(group, left, top, width, height, std::max(0, static_cast<int>(std::lround(sigma * 2))));
    if (pixels.empty()) return;

    auto mask = cairo_image_surface_create(CAIRO_FORMAT_A8, width, height);
    auto stride = cairo_image_surface_get_stride(mask);
    auto data = cairo_image_surface_get_data(mask);
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++) data[static_cast<size_t>(y) * stride + x] = pixels[(static_cast<size_t>(y) * width + x) * 4 + 3];
    cairo_surface_mark_dirty(mask);

    double deviceX = 0;
    double deviceY = 0;
    cairo_surface_get_device_offset(group, &deviceX, &deviceY);

    cairo_save(context);
    cairo_identity_matrix(context);
    cairo_set_source_rgba(context, 0, 0, 0, alpha);
    cairo_mask_surface(context, mask, left - deviceX + offsetX, top - deviceY + offsetY);
    cairo_restore(context);
    cairo_surface_destroy(mask);
}

class CairoTextLayout : public TextLayout {
public:
    CairoTextLayout(PangoLayout* layout, std::string text) : layout(layout), text(std::move(text)) {}

    ~CairoTextLayout() override {
        g_object_unref(layout);
    }

    TextMetrics metrics() const override {
        PangoRectangle logical{};
        pango_layout_get_extents(layout, nullptr, &logical);
        float width = static_cast<float>(logical.width) / PANGO_SCALE;
        return {width, width, static_cast<float>(logical.height) / PANGO_SCALE,
                static_cast<uint32_t>(pango_layout_get_line_count(layout))};
    }

    float baseline() const override {
        return static_cast<float>(pango_layout_get_baseline(layout)) / PANGO_SCALE;
    }

    TextPosition position(size_t index) const override {
        PangoRectangle rect{};
        pango_layout_index_to_pos(layout, static_cast<int>(byteOffset(text, index)), &rect);
        return {static_cast<float>(rect.x) / PANGO_SCALE, static_cast<float>(rect.y) / PANGO_SCALE,
                static_cast<float>(rect.height) / PANGO_SCALE};
    }

    std::vector<RectF> rangeRects(size_t start, size_t length) const override {
        int begin = static_cast<int>(byteOffset(text, start));
        int end = static_cast<int>(byteOffset(text, start + length));
        std::vector<RectF> result;

        auto iterator = pango_layout_get_iter(layout);
        do {
            auto line = pango_layout_iter_get_line_readonly(iterator);
            PangoRectangle extents{};
            pango_layout_iter_get_line_extents(iterator, nullptr, &extents);

            int* ranges = nullptr;
            int count = 0;
            pango_layout_line_get_x_ranges(line, begin, end, &ranges, &count);
            for (int index = 0; index < count; index++) {
                result.push_back({static_cast<float>(ranges[index * 2]) / PANGO_SCALE,
                                  static_cast<float>(extents.y) / PANGO_SCALE,
                                  static_cast<float>(ranges[index * 2 + 1]) / PANGO_SCALE,
                                  static_cast<float>(extents.y + extents.height) / PANGO_SCALE});
            }
            g_free(ranges);
        } while (pango_layout_iter_next_line(iterator));
        pango_layout_iter_free(iterator);

        return result;
    }

    size_t indexAt(float x, float y) const override {
        int index = 0;
        int trailing = 0;
        pango_layout_xy_to_index(layout, static_cast<int>(x * PANGO_SCALE), static_cast<int>(y * PANGO_SCALE), &index,
                                 &trailing);
        return characterOffset(text, static_cast<size_t>(index)) + static_cast<size_t>(trailing);
    }

    PangoLayout* layout;
    std::string text;
};

class CairoImage : public Image {
public:
    explicit CairoImage(cairo_surface_t* surface) : surface(surface) {}

    ~CairoImage() override {
        cairo_surface_destroy(surface);
    }

    int width() const override {
        return cairo_image_surface_get_width(surface);
    }

    int height() const override {
        return cairo_image_surface_get_height(surface);
    }

    cairo_surface_t* surface;
};

std::shared_ptr<Image> createSurface(uint8_t const* bgra, int width, int height, bool isOpaque) {
    auto surface = cairo_image_surface_create(isOpaque ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_ARGB32, width, height);
    auto stride = cairo_image_surface_get_stride(surface);
    auto data = cairo_image_surface_get_data(surface);
    for (int row = 0; row < height; row++)
        std::memcpy(data + static_cast<size_t>(row) * stride, bgra + static_cast<size_t>(row) * width * 4,
                    static_cast<size_t>(width) * 4);
    cairo_surface_mark_dirty(surface);
    return std::make_shared<CairoImage>(surface);
}

std::vector<uint8_t> readSurface(cairo_surface_t* surface, PixelRect const& rect) {
    cairo_surface_flush(surface);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    std::vector<uint8_t> pixels(static_cast<size_t>(std::max(0, width)) * std::max(0, height) * 4);
    auto stride = cairo_image_surface_get_stride(surface);
    auto data = cairo_image_surface_get_data(surface);

    for (int row = 0; row < height; row++)
        std::memcpy(pixels.data() + static_cast<size_t>(row) * width * 4,
                    data + static_cast<size_t>(rect.top + row) * stride + static_cast<size_t>(rect.left) * 4,
                    static_cast<size_t>(width) * 4);
    return pixels;
}

}

struct Resources::Backend {
    PangoFontMap* fonts = nullptr;
    PangoContext* context = nullptr;
    FontMetrics sansMetrics;
    FontMetrics emojiMetrics;
    bool metricsReady = false;

    Backend() {
        auto config = FcConfigGetCurrent();
        auto directory = resourcesPath() / "fonts";
        for (auto const& subset : GEIST_SUBSETS) {
            auto path = (directory / std::filesystem::path(subset.file)).string();
            FcConfigAppFontAddFile(config, reinterpret_cast<FcChar8 const*>(path.c_str()));
        }

        fonts = pango_cairo_font_map_new();
        context = pango_font_map_create_context(fonts);
        pango_context_set_round_glyph_positions(context, FALSE);
        cairo_font_options_t* options = cairo_font_options_create();
        cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_GRAY);
        cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_NONE);
        cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_OFF);
        pango_cairo_context_set_font_options(context, options);
        cairo_font_options_destroy(options);
    }

    ~Backend() {
        g_object_unref(context);
        g_object_unref(fonts);
    }

    PangoFontDescription* description(float size, FontWeight weight, FontKind kind) {
        auto value = pango_font_description_new();
        pango_font_description_set_family(value, kind == FontKind::Emoji ? EMOJI_FAMILY : SANS_FAMILY);
        pango_font_description_set_weight(value, static_cast<PangoWeight>(weight));
        pango_font_description_set_absolute_size(value, size * PANGO_SCALE);
        return value;
    }

    FontMetrics readMetrics(FontKind kind) {
        auto value = description(100, FontWeight::Normal, kind);
        auto metrics = pango_context_get_metrics(context, value, nullptr);
        FontMetrics result{static_cast<float>(pango_font_metrics_get_ascent(metrics)) / PANGO_SCALE / 100,
                           static_cast<float>(pango_font_metrics_get_descent(metrics)) / PANGO_SCALE / 100};
        pango_font_metrics_unref(metrics);
        pango_font_description_free(value);
        return result;
    }
};

namespace {

class CairoPainter : public Painter {
public:
    CairoPainter(cairo_t* context, Resources& resources) : Painter(resources), cr(context) {}

    void clear(Color const& value) override {
        cairo_save(cr);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        setColor(cr, value);
        cairo_paint(cr);
        cairo_restore(cr);
    }

    void fillRect(RectF const& rect, Color const& value) override {
        cairo_new_path(cr);
        cairo_rectangle(cr, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
        setColor(cr, value);
        cairo_fill(cr);
    }

    void strokeRect(RectF const& rect, Color const& value, float width, StrokeStyle const* style) override {
        cairo_new_path(cr);
        cairo_rectangle(cr, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
        setColor(cr, value);
        strokeCurrent(cr, width, style);
    }

    void fillRounded(RectF const& rect, float radius, Color const& value) override {
        cairo_new_path(cr);
        roundedRect(cr, rect, radius);
        setColor(cr, value);
        cairo_fill(cr);
    }

    void strokeRounded(RectF const& rect, float radius, Color const& value, float width,
                       StrokeStyle const* style) override {
        cairo_new_path(cr);
        roundedRect(cr, rect, radius);
        setColor(cr, value);
        strokeCurrent(cr, width, style);
    }

    void fillEllipse(float cx, float cy, float rx, float ry, Color const& value) override {
        cairo_new_path(cr);
        ellipse(cr, cx, cy, rx, ry);
        setColor(cr, value);
        cairo_fill(cr);
    }

    void strokeEllipse(float cx, float cy, float rx, float ry, Color const& value, float width) override {
        cairo_new_path(cr);
        ellipse(cr, cx, cy, rx, ry);
        setColor(cr, value);
        strokeCurrent(cr, width, nullptr);
    }

    void line(PointF from, PointF to, Color const& value, float width) override {
        cairo_new_path(cr);
        cairo_move_to(cr, from.x, from.y);
        cairo_line_to(cr, to.x, to.y);
        setColor(cr, value);
        strokeCurrent(cr, width, nullptr);
    }

    void fillPath(Path const& path, Color const& value) override {
        tracePath(cr, path);
        setColor(cr, value);
        cairo_fill(cr);
    }

    void strokePath(Path const& path, Color const& value, float width, StrokeStyle const& style) override {
        tracePath(cr, path);
        setColor(cr, value);
        strokeCurrent(cr, width, &style);
    }

    void text(TextLayout const& layout, float x, float y, Color const& value) override {
        cairo_save(cr);
        cairo_move_to(cr, x, y);
        setColor(cr, value);
        pango_cairo_show_layout(cr, static_cast<CairoTextLayout const&>(layout).layout);
        cairo_restore(cr);
    }

    void image(Image const& source, RectF const& destination, std::optional<RectF> const& cut,
               Interpolation interpolation) override {
        auto surface = static_cast<CairoImage const&>(source).surface;
        RectF from = cut ? *cut : RectF(0, 0, static_cast<float>(source.width()), static_cast<float>(source.height()));
        double scaleX = (destination.right - destination.left) / std::max(from.right - from.left, 0.0001f);
        double scaleY = (destination.bottom - destination.top) / std::max(from.bottom - from.top, 0.0001f);

        cairo_save(cr);
        cairo_new_path(cr);
        cairo_rectangle(cr, destination.left, destination.top, destination.right - destination.left,
                        destination.bottom - destination.top);
        cairo_clip(cr);
        cairo_translate(cr, destination.left, destination.top);
        cairo_scale(cr, scaleX, scaleY);
        cairo_set_source_surface(cr, surface, -from.left, -from.top);

        auto pattern = cairo_get_source(cr);
        cairo_pattern_set_filter(pattern, interpolation == Interpolation::Nearest ? CAIRO_FILTER_NEAREST
                                                                                  : CAIRO_FILTER_BILINEAR);
        cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD);
        cairo_paint(cr);
        cairo_restore(cr);
    }

    std::shared_ptr<Image> createImage(uint8_t const* bgra, int width, int height, bool isOpaque) override {
        return createSurface(bgra, width, height, isOpaque);
    }

    void boxShadow(RectF const& rect, float radius, float offsetY, float blur, float spread, float alpha) override {
        cairo_matrix_t matrix{};
        cairo_get_matrix(cr, &matrix);
        double scale = std::sqrt(std::abs(matrix.xx * matrix.yy - matrix.xy * matrix.yx));

        cairo_push_group(cr);
        fillRounded(RectF(rect.left - spread, rect.top - spread, rect.right + spread, rect.bottom + spread),
                    std::max(0.0f, radius + spread), Color(0, 0, 0, 1));
        auto pattern = cairo_pop_group(cr);

        cairo_surface_t* group = nullptr;
        cairo_pattern_get_surface(pattern, &group);
        paintShadow(cr, group, matrix.xy * offsetY, matrix.yy * offsetY, blur / 2 * scale, alpha);
        cairo_pattern_destroy(pattern);
    }

    void pushOpacity(float opacity) override {
        opacities.push_back(opacity);
        cairo_push_group(cr);
    }

    void popOpacity() override {
        cairo_pop_group_to_source(cr);
        cairo_paint_with_alpha(cr, opacities.back());
        opacities.pop_back();
    }

    void pushClip(RectF const& rect, float radius) override {
        cairo_save(cr);
        cairo_new_path(cr);
        roundedRect(cr, rect, radius);
        cairo_clip(cr);
    }

    void popClip() override {
        cairo_restore(cr);
    }

    void pushTransform(Matrix const& local) override {
        cairo_save(cr);
        cairo_matrix_t matrix{local.m11, local.m12, local.m21, local.m22, local.dx, local.dy};
        cairo_transform(cr, &matrix);
    }

    void popTransform() override {
        cairo_restore(cr);
    }

    Matrix transform() const override {
        cairo_matrix_t matrix{};
        cairo_get_matrix(cr, &matrix);
        return {static_cast<float>(matrix.xx), static_cast<float>(matrix.yx), static_cast<float>(matrix.xy),
                static_cast<float>(matrix.yy), static_cast<float>(matrix.x0), static_cast<float>(matrix.y0)};
    }

    void beginShadow() override {
        cairo_push_group(cr);
    }

    void endShadow(float offsetX, float offsetY, float blur, float alpha) override {
        auto pattern = cairo_pop_group(cr);

        cairo_matrix_t matrix{};
        cairo_get_matrix(cr, &matrix);
        double scaleX = std::abs(matrix.xx);
        double scaleY = std::abs(matrix.yy);

        cairo_surface_t* group = nullptr;
        cairo_pattern_get_surface(pattern, &group);
        paintShadow(cr, group, offsetX * scaleX, offsetY * scaleY, blur * std::min(scaleX, scaleY), alpha);

        cairo_set_source(cr, pattern);
        cairo_paint(cr);
        cairo_pattern_destroy(pattern);
    }

private:
    cairo_t* cr;
    std::vector<float> opacities;
};

class CairoCanvas : public Canvas {
public:
    CairoCanvas(Resources& resources, int width, int height)
        : surface(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height)), context(cairo_create(surface)),
          drawing(std::make_unique<CairoPainter>(context, resources)), width(width), height(height) {}

    ~CairoCanvas() override {
        drawing.reset();
        cairo_destroy(context);
        cairo_surface_destroy(surface);
    }

    Painter& painter() override {
        return *drawing;
    }

    std::vector<uint8_t> finish() override {
        drawing.reset();
        return readSurface(surface, {0, 0, width, height});
    }

private:
    cairo_surface_t* surface;
    cairo_t* context;
    std::unique_ptr<CairoPainter> drawing;
    int width;
    int height;
};

class CairoDevice : public Device {
public:
    explicit CairoDevice(Resources& resources) : resources(resources) {}

    std::shared_ptr<Image> createImage(uint8_t const* bgra, int width, int height, bool isOpaque) override {
        return createSurface(bgra, width, height, isOpaque);
    }

    std::vector<uint8_t> readImage(Image const& image, PixelRect const& rect) override {
        return readSurface(static_cast<CairoImage const&>(image).surface, rect);
    }

    std::unique_ptr<Canvas> createCanvas(int width, int height) override {
        return std::make_unique<CairoCanvas>(resources, width, height);
    }

private:
    Resources& resources;
};

cairo_t* scratch() {
    static cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_A8, 1, 1);
    static cairo_t* context = cairo_create(surface);
    return context;
}

}

Resources::Resources() : state(std::make_unique<Backend>()) {}

Resources::~Resources() {
    icons.clear();
}

FontMetrics Resources::metrics(FontKind kind) {
    auto& backend = *state;
    if (!backend.metricsReady) {
        backend.metricsReady = true;
        backend.sansMetrics = backend.readMetrics(FontKind::Sans);
        backend.emojiMetrics = backend.readMetrics(FontKind::Emoji);
    }

    return kind == FontKind::Emoji ? backend.emojiMetrics : backend.sansMetrics;
}

std::shared_ptr<TextLayout> Resources::layout(std::wstring_view text, float size, FontWeight weight, FontKind kind,
                                              float lineHeight, bool tabular) {
    auto& backend = *state;
    auto utf8 = toUtf8(text);
    auto layout = pango_layout_new(backend.context);
    auto description = backend.description(size, weight, kind);
    pango_layout_set_font_description(layout, description);
    pango_font_description_free(description);
    pango_layout_set_text(layout, utf8.c_str(), static_cast<int>(utf8.size()));

    auto attributes = pango_attr_list_new();
    if (tabular) pango_attr_list_insert(attributes, pango_attr_font_features_new("tnum 1"));
    if (lineHeight > 0)
        pango_attr_list_insert(attributes, pango_attr_line_height_new_absolute(static_cast<int>(lineHeight * PANGO_SCALE)));
    pango_layout_set_attributes(layout, attributes);
    pango_attr_list_unref(attributes);

    return std::make_shared<CairoTextLayout>(layout, std::move(utf8));
}

RectF Resources::bounds(Path const& path) {
    auto context = scratch();
    tracePath(context, path);
    double x1 = 0;
    double y1 = 0;
    double x2 = 0;
    double y2 = 0;
    cairo_path_extents(context, &x1, &y1, &x2, &y2);
    return {static_cast<float>(x1), static_cast<float>(y1), static_cast<float>(x2), static_cast<float>(y2)};
}

bool Resources::fillContains(Path const& path, PointF point) {
    auto context = scratch();
    tracePath(context, path);
    return cairo_in_fill(context, point.x, point.y);
}

bool Resources::strokeContains(Path const& path, PointF point, float width, StrokeStyle const* style) {
    auto context = scratch();
    tracePath(context, path);
    applyStroke(context, width, style);
    return cairo_in_stroke(context, point.x, point.y);
}

std::unique_ptr<Device> Device::create(Resources& resources) {
    return std::make_unique<CairoDevice>(resources);
}

std::filesystem::path executableDirectory() {
    std::error_code error;
    return std::filesystem::read_symlink("/proc/self/exe", error).parent_path();
}

double caretBlinkMs() {
    auto settings = gtk_settings_get_default();
    if (!settings) return 600;

    gboolean blink = TRUE;
    gint time = 1200;
    g_object_get(settings, "gtk-cursor-blink", &blink, "gtk-cursor-blink-time", &time, nullptr);
    return blink ? time / 2.0 : 0;
}

std::unique_ptr<Painter> createPainter(cairo_t* context, Resources& resources) {
    return std::make_unique<CairoPainter>(context, resources);
}

cairo_surface_t* surfaceOf(Image const& image) {
    return static_cast<CairoImage const&>(image).surface;
}

}

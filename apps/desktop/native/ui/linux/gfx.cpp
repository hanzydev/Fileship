#include "cairo.h"

#include <fonts.h>

#include <shared/utf8.h>

#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include <pango/pangofc-fontmap.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <numbers>

namespace fileship::ui {

namespace {

constexpr double PI = std::numbers::pi;

std::weak_ptr<CairoResources> sharedResources;

float units(int value) {
    return static_cast<float>(value) / PANGO_SCALE;
}

void roundedPath(cairo_t* cr, RectF const& rect, float radius) {
    double width = rect.right - rect.left;
    double height = rect.bottom - rect.top;
    double r = std::max(0.0, std::min<double>(radius, std::min(width, height) / 2));

    if (r == 0) {
        cairo_rectangle(cr, rect.left, rect.top, width, height);
        return;
    }

    cairo_new_sub_path(cr);
    cairo_arc(cr, rect.right - r, rect.top + r, r, -PI / 2, 0);
    cairo_arc(cr, rect.right - r, rect.bottom - r, r, 0, PI / 2);
    cairo_arc(cr, rect.left + r, rect.bottom - r, r, PI / 2, PI);
    cairo_arc(cr, rect.left + r, rect.top + r, r, PI, PI * 3 / 2);
    cairo_close_path(cr);
}

void ellipsePath(cairo_t* cr, double cx, double cy, double rx, double ry) {
    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_scale(cr, rx, ry);
    cairo_new_sub_path(cr);
    cairo_arc(cr, 0, 0, 1, 0, PI * 2);
    cairo_close_path(cr);
    cairo_restore(cr);
}

class CairoSink : public PathSink {
public:
    explicit CairoSink(cairo_t* cr) : cr(cr) {}

    void begin(PointF point, bool) override {
        cairo_move_to(cr, point.x, point.y);
    }

    void lineTo(PointF point) override {
        cairo_line_to(cr, point.x, point.y);
    }

    void quadTo(PointF control, PointF end) override {
        double x = 0;
        double y = 0;
        cairo_get_current_point(cr, &x, &y);
        cairo_curve_to(cr, x + (control.x - x) * 2 / 3, y + (control.y - y) * 2 / 3,
                       end.x + (control.x - end.x) * 2 / 3, end.y + (control.y - end.y) * 2 / 3, end.x, end.y);
    }

    void cubicTo(PointF first, PointF second, PointF end) override {
        cairo_curve_to(cr, first.x, first.y, second.x, second.y, end.x, end.y);
    }

    void arcTo(PointF end, SizeF radius, float rotation, bool isClockwise, bool isLarge) override {
        double x0 = 0;
        double y0 = 0;
        cairo_get_current_point(cr, &x0, &y0);

        double angle = rotation * PI / 180;
        double cosine = std::cos(angle);
        double sine = std::sin(angle);
        double dx = (x0 - end.x) / 2;
        double dy = (y0 - end.y) / 2;
        double x1 = cosine * dx + sine * dy;
        double y1 = -sine * dx + cosine * dy;
        double rx = radius.width;
        double ry = radius.height;

        double lambda = x1 * x1 / (rx * rx) + y1 * y1 / (ry * ry);
        if (lambda > 1) {
            rx *= std::sqrt(lambda);
            ry *= std::sqrt(lambda);
        }

        double numerator = rx * rx * ry * ry - rx * rx * y1 * y1 - ry * ry * x1 * x1;
        double denominator = rx * rx * y1 * y1 + ry * ry * x1 * x1;
        double factor = (isLarge != isClockwise ? 1 : -1) * std::sqrt(std::max(0.0, numerator / denominator));
        double cx1 = factor * rx * y1 / ry;
        double cy1 = -factor * ry * x1 / rx;
        double cx = cosine * cx1 - sine * cy1 + (x0 + end.x) / 2;
        double cy = sine * cx1 + cosine * cy1 + (y0 + end.y) / 2;
        double start = std::atan2((y1 - cy1) / ry, (x1 - cx1) / rx);
        double stop = std::atan2((-y1 - cy1) / ry, (-x1 - cx1) / rx);

        cairo_save(cr);
        cairo_translate(cr, cx, cy);
        cairo_rotate(cr, angle);
        cairo_scale(cr, rx, ry);
        if (isClockwise) {
            cairo_arc(cr, 0, 0, 1, start, stop);
        } else {
            cairo_arc_negative(cr, 0, 0, 1, start, stop);
        }
        cairo_restore(cr);
    }

    void end(bool isClosed) override {
        if (isClosed) cairo_close_path(cr);
    }

private:
    cairo_t* cr;
};

std::array<int, 3> boxRadii(float sigma) {
    double ideal = std::sqrt(12.0 * sigma * sigma / 3 + 1);
    int lower = static_cast<int>(std::floor(ideal));
    if (lower % 2 == 0) lower--;
    int upper = lower + 2;
    int count = static_cast<int>(
        std::lround((12.0 * sigma * sigma - 3.0 * lower * lower - 12.0 * lower - 9) / (-4.0 * lower - 4)));

    std::array<int, 3> radii{};
    for (int index = 0; index < 3; index++) radii[index] = ((index < count ? lower : upper) - 1) / 2;
    return radii;
}

void boxBlur(uint8_t const* in, uint8_t* out, int count, int step, int radius) {
    int window = radius * 2 + 1;
    int sum = 0;
    for (int index = 0; index <= radius && index < count; index++) sum += in[index * step];

    for (int index = 0; index < count; index++) {
        out[index * step] = static_cast<uint8_t>((sum + window / 2) / window);
        int entering = index + radius + 1;
        int leaving = index - radius;
        if (entering < count) sum += in[entering * step];
        if (leaving >= 0) sum -= in[leaving * step];
    }
}

void blurAlpha(cairo_surface_t* surface, float sigma) {
    if (sigma < 0.5f) return;

    cairo_surface_flush(surface);
    auto data = cairo_image_surface_get_data(surface);
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    int stride = cairo_image_surface_get_stride(surface);
    std::vector<uint8_t> buffer(static_cast<size_t>(stride) * height);

    for (int radius : boxRadii(sigma)) {
        if (radius < 1) continue;
        for (int y = 0; y < height; y++) boxBlur(data + y * stride, buffer.data() + y * stride, width, 1, radius);
        for (int x = 0; x < width; x++) boxBlur(buffer.data() + x, data + x, height, stride, radius);
    }

    cairo_surface_mark_dirty(surface);
}

void unpackFont(FT_Library library, std::filesystem::path const& packed, std::filesystem::path const& unpacked) {
    FT_Face face = nullptr;
    FT_ULong length = 0;
    FT_New_Face(library, packed.c_str(), 0, &face);
    FT_Load_Sfnt_Table(face, 0, 0, nullptr, &length);

    std::vector<FT_Byte> bytes(length);
    FT_Load_Sfnt_Table(face, 0, 0, bytes.data(), &length);
    FT_Done_Face(face);

    std::ofstream(unpacked, std::ios::binary).write(reinterpret_cast<char const*>(bytes.data()), bytes.size());
}

void loadFonts() {
    static bool isLoaded = false;
    if (isLoaded) return;
    isLoaded = true;

    auto fonts = resourcesPath() / L"fonts";
    auto cache = std::filesystem::path(g_get_user_cache_dir()) / "fileship" / "fonts";
    std::filesystem::create_directories(cache);

    FT_Library library = nullptr;
    FT_Init_FreeType(&library);

    for (auto const& subset : GEIST_SUBSETS) {
        auto packed = fonts / subset.file;
        auto unpacked = cache / packed.filename().replace_extension(".ttf");
        if (!std::filesystem::exists(unpacked) ||
            std::filesystem::last_write_time(packed) > std::filesystem::last_write_time(unpacked))
            unpackFont(library, packed, unpacked);

        FcConfigAppFontAddFile(nullptr, reinterpret_cast<FcChar8 const*>(unpacked.c_str()));
    }

    FT_Done_FreeType(library);
    pango_fc_font_map_config_changed(PANGO_FC_FONT_MAP(pango_cairo_font_map_get_default()));
}

std::string applicationFamily() {
    auto fonts = FcConfigGetFonts(nullptr, FcSetApplication);
    FcChar8* family = nullptr;
    if (!fonts || fonts->nfont == 0 || FcPatternGetString(fonts->fonts[0], FC_FAMILY, 0, &family) != FcResultMatch)
        return "sans-serif";
    return reinterpret_cast<char const*>(family);
}

FontMetrics readMetrics(PangoContext* context, PangoFontDescription* description) {
    auto metrics = pango_context_get_metrics(context, description, nullptr);
    float size = units(pango_font_description_get_size(description));
    FontMetrics result{units(pango_font_metrics_get_ascent(metrics)) / size,
                       units(pango_font_metrics_get_descent(metrics)) / size};
    pango_font_metrics_unref(metrics);
    return result;
}

}

std::filesystem::path executableDirectory() {
    return std::filesystem::read_symlink("/proc/self/exe").parent_path();
}

Matrix Matrix::rotation(float degrees, PointF center) {
    float angle = degrees * static_cast<float>(PI) / 180;
    float sine = std::sin(angle);
    float cosine = std::cos(angle);
    return Matrix{cosine, sine, -sine, cosine, center.x - center.x * cosine + center.y * sine,
                  center.y - center.x * sine - center.y * cosine};
}

std::shared_ptr<Resources> Resources::acquire() {
    if (auto existing = sharedResources.lock()) return existing;

    auto resources = std::make_shared<CairoResources>();
    sharedResources = resources;
    return resources;
}

std::unique_ptr<Canvas> Canvas::create(Resources& resources) {
    return std::make_unique<CairoCanvas>(static_cast<CairoResources&>(resources));
}

CairoResources::CairoResources() {
    loadFonts();
    sansFamily = applicationFamily();

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    measure = cairo_create(surface);
    context = pango_font_map_create_context(pango_cairo_font_map_get_default());

    auto options = cairo_font_options_create();
    cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_GRAY);
    cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_NONE);
    cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_OFF);
    pango_cairo_context_set_font_options(context, options);
    pango_cairo_context_set_resolution(context, 96);
    cairo_font_options_destroy(options);
}

CairoResources::~CairoResources() {
    for (auto& [key, path] : icons) cairo_path_destroy(path);
    for (auto& [key, mask] : shadows) cairo_surface_destroy(mask);
    g_object_unref(context);
    cairo_destroy(measure);
    cairo_surface_destroy(surface);
}

PangoFontDescription* CairoResources::describe(float size, FontWeight weight, FontKind kind) {
    auto description = pango_font_description_new();

    if (kind == FontKind::Emoji) {
        pango_font_description_set_family(description, "Noto Color Emoji, emoji");
    } else {
        pango_font_description_set_family(description, sansFamily.c_str());
        pango_font_description_set_weight(description, static_cast<PangoWeight>(weight));
        pango_font_description_set_variations(description,
                                              ("wght=" + std::to_string(static_cast<int>(weight))).c_str());
    }

    pango_font_description_set_absolute_size(description, size * PANGO_SCALE);
    return description;
}

std::shared_ptr<TextLayout> CairoResources::layout(std::wstring_view text, float size, FontWeight weight,
                                                   FontKind kind, float lineHeight, bool tabular) {
    auto utf8 = toUtf8(text);
    auto layout = pango_layout_new(context);
    auto description = describe(size, weight, kind);
    pango_layout_set_font_description(layout, description);
    pango_font_description_free(description);
    pango_layout_set_text(layout, utf8.data(), static_cast<int>(utf8.size()));

    auto attributes = pango_attr_list_new();
    if (tabular) pango_attr_list_insert(attributes, pango_attr_font_features_new("tnum=1"));
    if (lineHeight > 0)
        pango_attr_list_insert(
            attributes, pango_attr_line_height_new_absolute(static_cast<int>(std::lround(lineHeight * PANGO_SCALE))));
    pango_layout_set_attributes(layout, attributes);
    pango_attr_list_unref(attributes);

    return std::make_shared<PangoTextLayout>(layout, std::move(utf8));
}

FontMetrics CairoResources::metrics(FontKind kind) {
    if (!metricsReady) {
        metricsReady = true;

        auto sans = describe(100, FontWeight::Normal, FontKind::Sans);
        sansMetrics = readMetrics(context, sans);
        pango_font_description_free(sans);

        auto emoji = describe(100, FontWeight::Normal, FontKind::Emoji);
        emojiMetrics = readMetrics(context, emoji);
        pango_font_description_free(emoji);
    }

    return kind == FontKind::Emoji ? emojiMetrics : sansMetrics;
}

std::shared_ptr<Path> CairoResources::path(std::function<void(PathSink&)> const& build) {
    cairo_new_path(measure);
    CairoSink sink(measure);
    build(sink);

    auto result = std::make_shared<CairoPath>(cairo_copy_path(measure), *this);
    cairo_new_path(measure);
    return result;
}

std::shared_ptr<Path> CairoResources::roundedRect(RectF const& rect, float radius) {
    cairo_new_path(measure);
    roundedPath(measure, rect, radius);

    auto result = std::make_shared<CairoPath>(cairo_copy_path(measure), *this);
    cairo_new_path(measure);
    return result;
}

std::shared_ptr<Path> CairoResources::ellipse(PointF center, float radiusX, float radiusY) {
    cairo_new_path(measure);
    ellipsePath(measure, center.x, center.y, radiusX, radiusY);

    auto result = std::make_shared<CairoPath>(cairo_copy_path(measure), *this);
    cairo_new_path(measure);
    return result;
}

cairo_path_t* CairoResources::icon(Icon const& value) {
    if (auto found = icons.find(&value); found != icons.end()) return found->second;

    cairo_new_path(measure);
    CairoSink sink(measure);
    for (size_t index = 0; index < value.count; index++) parseSvgPath(sink, value.paths[index]);

    auto path = cairo_copy_path(measure);
    cairo_new_path(measure);
    icons[&value] = path;
    return path;
}

cairo_surface_t* CairoResources::shadow(int width, int height, float radius, float sigma) {
    char key[96];
    snprintf(key, sizeof(key), "%d:%d:%.2f:%.2f", width, height, radius, sigma);
    if (auto found = shadows.find(key); found != shadows.end()) return found->second;

    if (shadows.size() >= 64) {
        for (auto& [name, mask] : shadows) cairo_surface_destroy(mask);
        shadows.clear();
    }

    int pad = static_cast<int>(std::ceil(sigma * 3)) + 1;
    auto mask = cairo_image_surface_create(CAIRO_FORMAT_A8, width + pad * 2, height + pad * 2);
    auto cr = cairo_create(mask);
    roundedPath(cr, RectF(static_cast<float>(pad), static_cast<float>(pad), static_cast<float>(pad + width),
                          static_cast<float>(pad + height)),
                radius);
    cairo_set_source_rgba(cr, 0, 0, 0, 1);
    cairo_fill(cr);
    cairo_destroy(cr);

    blurAlpha(mask, sigma);
    shadows[key] = mask;
    return mask;
}

PangoTextLayout::~PangoTextLayout() {
    g_object_unref(layout);
}

int PangoTextLayout::byteOf(size_t position) const {
    auto start = text.c_str();
    return static_cast<int>(g_utf8_offset_to_pointer(start, static_cast<glong>(position)) - start);
}

TextMetrics PangoTextLayout::metrics() {
    PangoRectangle logical{};
    pango_layout_get_extents(layout, nullptr, &logical);
    return TextMetrics{units(logical.width), static_cast<uint32_t>(pango_layout_get_line_count(layout))};
}

float PangoTextLayout::baseline() {
    return units(pango_layout_get_baseline(layout));
}

TextCaret PangoTextLayout::caret(size_t position) {
    PangoRectangle strong{};
    pango_layout_get_cursor_pos(layout, byteOf(position), &strong, nullptr);
    return TextCaret{units(strong.x), units(strong.y), units(strong.height)};
}

size_t PangoTextLayout::indexAt(float x, float y) {
    int index = 0;
    int trailing = 0;
    pango_layout_xy_to_index(layout, static_cast<int>(std::lround(x * PANGO_SCALE)),
                             static_cast<int>(std::lround(y * PANGO_SCALE)), &index, &trailing);
    return static_cast<size_t>(g_utf8_pointer_to_offset(text.c_str(), text.c_str() + index)) + trailing;
}

std::vector<TextRange> PangoTextLayout::ranges(size_t start, size_t length) {
    std::vector<TextRange> result;
    int first = byteOf(start);
    int last = byteOf(start + length);

    auto iterator = pango_layout_get_iter(layout);
    do {
        auto line = pango_layout_iter_get_line_readonly(iterator);
        int lineStart = pango_layout_line_get_start_index(line);
        int lineEnd = lineStart + pango_layout_line_get_length(line);
        if (last <= lineStart || first > lineEnd) continue;

        PangoRectangle logical{};
        pango_layout_iter_get_line_extents(iterator, nullptr, &logical);

        int* spans = nullptr;
        int count = 0;
        pango_layout_line_get_x_ranges(line, std::max(first, lineStart), std::min(last, lineEnd), &spans, &count);
        for (int index = 0; index < count; index++)
            result.push_back(TextRange{units(spans[index * 2]), units(logical.y),
                                       units(spans[index * 2 + 1] - spans[index * 2]), units(logical.height)});
        g_free(spans);
    } while (pango_layout_iter_next_line(iterator));
    pango_layout_iter_free(iterator);

    return result;
}

CairoPath::~CairoPath() {
    cairo_path_destroy(path);
}

RectF CairoPath::bounds() {
    auto cr = resources.scratch();
    double left = 0;
    double top = 0;
    double right = 0;
    double bottom = 0;

    cairo_new_path(cr);
    cairo_append_path(cr, path);
    cairo_path_extents(cr, &left, &top, &right, &bottom);
    cairo_new_path(cr);
    return RectF(static_cast<float>(left), static_cast<float>(top), static_cast<float>(right),
                 static_cast<float>(bottom));
}

bool CairoPath::fillContains(PointF point) {
    auto cr = resources.scratch();
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_append_path(cr, path);
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
    bool contains = cairo_in_fill(cr, point.x, point.y);
    cairo_new_path(cr);
    cairo_restore(cr);
    return contains;
}

bool CairoPath::strokeContains(PointF point, float width, Stroke const& stroke) {
    auto cr = resources.scratch();
    cairo_save(cr);
    cairo_new_path(cr);
    cairo_append_path(cr, path);
    cairo_set_line_width(cr, width);
    cairo_set_line_cap(cr, stroke.isRound ? CAIRO_LINE_CAP_ROUND : CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join(cr, stroke.isRound ? CAIRO_LINE_JOIN_ROUND : CAIRO_LINE_JOIN_MITER);
    bool contains = cairo_in_stroke(cr, point.x, point.y);
    cairo_new_path(cr);
    cairo_restore(cr);
    return contains;
}

CairoImage::~CairoImage() {
    cairo_surface_destroy(surface);
}

CairoPainter::CairoPainter(cairo_t* context, CairoResources& resources) : cr(context), res(resources) {
    double vertical = 1;
    cairo_surface_get_device_scale(cairo_get_target(context), &device, &vertical);
}

Resources& CairoPainter::resources() {
    return res;
}

void CairoPainter::source(Color const& value) {
    cairo_set_source_rgba(cr, value.r, value.g, value.b, value.a);
}

void CairoPainter::pixels() {
    cairo_matrix_t matrix;
    cairo_matrix_init_scale(&matrix, 1 / device, 1 / device);
    cairo_set_matrix(cr, &matrix);
}

void CairoPainter::stroke(float width, Stroke const& style) {
    cairo_save(cr);
    cairo_set_line_cap(cr, style.isRound ? CAIRO_LINE_CAP_ROUND : CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join(cr, style.isRound && style.dash == 0 ? CAIRO_LINE_JOIN_ROUND : CAIRO_LINE_JOIN_MITER);
    cairo_set_miter_limit(cr, 10);

    if (style.dash > 0) {
        double dashes[] = {style.dash * width, style.gap * width};
        pixels();
        cairo_set_dash(cr, dashes, 2, 0);
    }

    cairo_set_line_width(cr, width);
    cairo_stroke(cr);
    cairo_restore(cr);
}

cairo_t* CairoPainter::record(cairo_surface_t*& recording) {
    cairo_matrix_t matrix;
    cairo_get_matrix(cr, &matrix);

    recording = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, nullptr);
    auto context = cairo_create(recording);
    cairo_set_matrix(context, &matrix);
    return context;
}

void CairoPainter::clear(Color const& value) {
    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    source(value);
    cairo_paint(cr);
    cairo_restore(cr);
}

void CairoPainter::fillRect(RectF const& rect, Color const& value) {
    source(value);
    cairo_rectangle(cr, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
    cairo_fill(cr);
}

void CairoPainter::strokeRect(RectF const& rect, Color const& value, float width, Stroke const& style) {
    source(value);
    cairo_rectangle(cr, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
    stroke(width, style);
}

void CairoPainter::fillFrame(RectF const& outer, RectF const& inner, Color const& value) {
    cairo_save(cr);
    source(value);
    cairo_rectangle(cr, outer.left, outer.top, outer.right - outer.left, outer.bottom - outer.top);
    cairo_rectangle(cr, inner.left, inner.top, inner.right - inner.left, inner.bottom - inner.top);
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_fill(cr);
    cairo_restore(cr);
}

void CairoPainter::fillRounded(RectF const& rect, float radius, Color const& value) {
    source(value);
    roundedPath(cr, rect, radius);
    cairo_fill(cr);
}

void CairoPainter::strokeRounded(RectF const& rect, float radius, Color const& value, float width,
                                 Stroke const& style) {
    source(value);
    roundedPath(cr, rect, radius);
    stroke(width, style);
}

void CairoPainter::fillEllipse(float cx, float cy, float rx, float ry, Color const& value) {
    source(value);
    ellipsePath(cr, cx, cy, rx, ry);
    cairo_fill(cr);
}

void CairoPainter::strokeEllipse(float cx, float cy, float rx, float ry, Color const& value, float width) {
    source(value);
    ellipsePath(cr, cx, cy, rx, ry);
    stroke(width, {});
}

void CairoPainter::line(PointF from, PointF to, Color const& value, float width) {
    source(value);
    cairo_move_to(cr, from.x, from.y);
    cairo_line_to(cr, to.x, to.y);
    stroke(width, {});
}

void CairoPainter::fillPath(Path& path, Color const& value) {
    cairo_save(cr);
    source(value);
    cairo_append_path(cr, static_cast<CairoPath&>(path).get());
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_fill(cr);
    cairo_restore(cr);
}

void CairoPainter::strokePath(Path& path, Color const& value, float width, Stroke const& style) {
    source(value);
    cairo_append_path(cr, static_cast<CairoPath&>(path).get());
    stroke(width, style);
}

void CairoPainter::icon(Icon const& value, float x, float y, float size, Color const& tint, bool fill,
                        float strokeWidth) {
    cairo_save(cr);
    cairo_translate(cr, x, y);
    cairo_scale(cr, size / 24.0, size / 24.0);
    cairo_append_path(cr, res.icon(value));
    source(tint);
    if (fill) cairo_fill_preserve(cr);

    cairo_set_line_width(cr, strokeWidth);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    cairo_stroke(cr);
    cairo_restore(cr);
}

void CairoPainter::text(TextLayout& layout, float x, float y, Color const& value) {
    auto pango = static_cast<PangoTextLayout&>(layout).get();
    source(value);
    cairo_move_to(cr, x, y);
    pango_cairo_update_layout(cr, pango);
    pango_cairo_show_layout(cr, pango);
    cairo_new_path(cr);
}

double CairoPainter::pixelScale() {
    cairo_matrix_t matrix;
    cairo_get_matrix(cr, &matrix);
    return std::sqrt(std::abs(matrix.xx * matrix.yy - matrix.xy * matrix.yx)) * device;
}

void CairoPainter::boxShadow(RectF const& rect, float radius, float offsetY, float blur, float spread, float alpha) {
    cairo_matrix_t matrix;
    cairo_get_matrix(cr, &matrix);
    double scale = pixelScale();

    double left = ((rect.left - spread) * matrix.xx + matrix.x0) * device;
    double top = ((rect.top - spread) * matrix.yy + matrix.y0) * device;
    int width = static_cast<int>(std::lround((rect.right - rect.left + spread * 2) * matrix.xx * device));
    int height = static_cast<int>(std::lround((rect.bottom - rect.top + spread * 2) * matrix.yy * device));
    if (width <= 0 || height <= 0) return;

    auto mask = res.shadow(width, height, static_cast<float>(std::max(0.0f, radius + spread) * scale),
                           static_cast<float>(std::max(0.01, blur / 2 * scale)));
    int pad = (cairo_image_surface_get_width(mask) - width) / 2;

    cairo_save(cr);
    pixels();
    cairo_set_source_rgba(cr, 0, 0, 0, alpha);
    cairo_mask_surface(cr, mask, left - pad + matrix.xy * offsetY * device, top - pad + matrix.yy * offsetY * device);
    cairo_restore(cr);
}

void CairoPainter::shadowed(float blur, float offset, float alpha, std::function<void()> const& draw) {
    cairo_matrix_t matrix;
    cairo_get_matrix(cr, &matrix);

    cairo_surface_t* recording = nullptr;
    auto previous = cr;
    cr = record(recording);
    draw();
    cairo_destroy(cr);
    cr = previous;

    double x = 0;
    double y = 0;
    double width = 0;
    double height = 0;
    cairo_recording_surface_ink_extents(recording, &x, &y, &width, &height);

    if (width > 0 && height > 0) {
        double scaleX = std::abs(matrix.xx) * device;
        double scaleY = std::abs(matrix.yy) * device;
        float sigma = static_cast<float>(blur * std::min(scaleX, scaleY));
        int pad = static_cast<int>(std::ceil(sigma * 3)) + 1;
        int left = static_cast<int>(std::floor(x * device)) - pad;
        int top = static_cast<int>(std::floor(y * device)) - pad;
        int right = static_cast<int>(std::ceil((x + width) * device)) + pad;
        int bottom = static_cast<int>(std::ceil((y + height) * device)) + pad;

        auto mask = cairo_image_surface_create(CAIRO_FORMAT_A8, right - left, bottom - top);
        auto target = cairo_create(mask);
        cairo_translate(target, -left, -top);
        cairo_scale(target, device, device);
        cairo_set_source_surface(target, recording, 0, 0);
        cairo_paint(target);
        cairo_destroy(target);
        blurAlpha(mask, sigma);

        cairo_save(cr);
        pixels();
        cairo_set_source_rgba(cr, 0, 0, 0, alpha);
        cairo_mask_surface(cr, mask, left + offset * scaleX, top + offset * scaleY);
        cairo_restore(cr);
        cairo_surface_destroy(mask);
    }

    cairo_surface_destroy(recording);
    draw();
}

std::shared_ptr<Image> CairoPainter::image(uint8_t const* bgra, int width, int height) {
    auto surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    auto data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    for (int row = 0; row < height; row++)
        memcpy(data + static_cast<size_t>(row) * stride, bgra + static_cast<size_t>(row) * width * 4,
               static_cast<size_t>(width) * 4);
    cairo_surface_mark_dirty(surface);
    return std::make_shared<CairoImage>(surface);
}

void CairoPainter::drawImage(Image& image, RectF const& target) {
    auto surface = static_cast<CairoImage&>(image).get();
    double width = cairo_image_surface_get_width(surface);
    double height = cairo_image_surface_get_height(surface);

    cairo_save(cr);
    cairo_translate(cr, target.left, target.top);
    cairo_scale(cr, (target.right - target.left) / width, (target.bottom - target.top) / height);
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_BILINEAR);
    cairo_pattern_set_extend(cairo_get_source(cr), CAIRO_EXTEND_PAD);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    cairo_restore(cr);
}

void CairoPainter::drawPixels(Image& image, RectF const& from, RectF const& target) {
    double width = from.right - from.left;
    double height = from.bottom - from.top;

    cairo_save(cr);
    cairo_translate(cr, target.left, target.top);
    cairo_scale(cr, (target.right - target.left) / width, (target.bottom - target.top) / height);
    cairo_set_source_surface(cr, static_cast<CairoImage&>(image).get(), -from.left, -from.top);
    cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    cairo_restore(cr);
}

void CairoPainter::pushOpacity(float opacity) {
    cairo_surface_t* recording = nullptr;
    auto context = record(recording);
    layers.push_back({cr, recording, opacity});
    cr = context;
}

void CairoPainter::popOpacity() {
    auto layer = layers.back();
    layers.pop_back();
    cairo_destroy(cr);
    cr = layer.previous;

    double x = 0;
    double y = 0;
    double width = 0;
    double height = 0;
    cairo_recording_surface_ink_extents(layer.recording, &x, &y, &width, &height);

    if (width > 0 && height > 0) {
        cairo_save(cr);
        cairo_identity_matrix(cr);
        cairo_rectangle(cr, std::floor(x) - 1, std::floor(y) - 1, std::ceil(width) + 3, std::ceil(height) + 3);
        cairo_clip(cr);
        cairo_set_source_surface(cr, layer.recording, 0, 0);
        cairo_paint_with_alpha(cr, layer.opacity);
        cairo_restore(cr);
    }

    cairo_surface_destroy(layer.recording);
}

void CairoPainter::pushClip(Path& path) {
    cairo_save(cr);
    cairo_append_path(cr, static_cast<CairoPath&>(path).get());
    cairo_clip(cr);
}

void CairoPainter::popClip() {
    cairo_matrix_t matrix;
    cairo_get_matrix(cr, &matrix);
    cairo_restore(cr);
    cairo_set_matrix(cr, &matrix);
}

Matrix CairoPainter::transform() {
    cairo_matrix_t matrix;
    cairo_get_matrix(cr, &matrix);
    return Matrix{static_cast<float>(matrix.xx), static_cast<float>(matrix.yx), static_cast<float>(matrix.xy),
                  static_cast<float>(matrix.yy), static_cast<float>(matrix.x0), static_cast<float>(matrix.y0)};
}

void CairoPainter::setTransform(Matrix const& value) {
    cairo_matrix_t matrix;
    cairo_matrix_init(&matrix, value.m11, value.m12, value.m21, value.m22, value.dx, value.dy);
    cairo_set_matrix(cr, &matrix);
}

std::shared_ptr<Image> CairoCanvas::image(uint8_t const* bgra, int width, int height) {
    auto surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
    auto data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    for (int row = 0; row < height; row++)
        memcpy(data + static_cast<size_t>(row) * stride, bgra + static_cast<size_t>(row) * width * 4,
               static_cast<size_t>(width) * 4);
    cairo_surface_mark_dirty(surface);
    return std::make_shared<CairoImage>(surface);
}

std::vector<uint8_t> CairoCanvas::read(Image& image, PixelRect const& rect) {
    auto surface = static_cast<CairoImage&>(image).get();
    auto data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
    for (int row = 0; row < height; row++)
        memcpy(pixels.data() + static_cast<size_t>(row) * width * 4,
               data + static_cast<size_t>(rect.top + row) * stride + static_cast<size_t>(rect.left) * 4,
               static_cast<size_t>(width) * 4);
    return pixels;
}

std::vector<uint8_t> CairoCanvas::render(int width, int height, std::function<void(Painter&)> const& draw) {
    auto surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    auto cr = cairo_create(surface);
    {
        CairoPainter painter(cr, res);
        draw(painter);
    }
    cairo_destroy(cr);
    cairo_surface_flush(surface);

    auto data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
    for (int row = 0; row < height; row++)
        memcpy(pixels.data() + static_cast<size_t>(row) * width * 4, data + static_cast<size_t>(row) * stride,
               static_cast<size_t>(width) * 4);
    cairo_surface_destroy(surface);
    return pixels;
}

}

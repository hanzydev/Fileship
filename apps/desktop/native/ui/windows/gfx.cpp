#include "d2d.h"

#include <d2d1_3helper.h>
#include <d2d1effects_2.h>

#include <fonts.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iterator>

namespace fileship::ui {

namespace {

std::weak_ptr<D2DResources> sharedResources;

std::vector<char> readFile(std::filesystem::path const& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

std::wstring familyName(IDWriteFontCollection1* collection) {
    com_ptr<IDWriteFontFamily1> family;
    if (!collection || collection->GetFontFamilyCount() == 0 || FAILED(collection->GetFontFamily(0, family.put())))
        return {};

    com_ptr<IDWriteLocalizedStrings> names;
    if (FAILED(family->GetFamilyNames(names.put()))) return {};

    UINT32 index = 0;
    BOOL exists = FALSE;
    names->FindLocaleName(L"en-us", &index, &exists);
    if (!exists) index = 0;

    UINT32 length = 0;
    names->GetStringLength(index, &length);
    std::wstring name(length, L'\0');
    names->GetString(index, name.data(), length + 1);
    return name;
}

class GeometrySink : public PathSink {
public:
    explicit GeometrySink(ID2D1GeometrySink* sink) : sink(sink) {}

    void begin(PointF point, bool isFilled) override {
        sink->BeginFigure(d2d(point), isFilled ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
    }

    void lineTo(PointF point) override {
        sink->AddLine(d2d(point));
    }

    void quadTo(PointF control, PointF end) override {
        sink->AddQuadraticBezier(D2D1::QuadraticBezierSegment(d2d(control), d2d(end)));
    }

    void cubicTo(PointF first, PointF second, PointF end) override {
        sink->AddBezier(D2D1::BezierSegment(d2d(first), d2d(second), d2d(end)));
    }

    void arcTo(PointF end, SizeF radius, float rotation, bool isClockwise, bool isLarge) override {
        sink->AddArc(D2D1::ArcSegment(d2d(end), D2D1::SizeF(radius.width, radius.height), rotation,
                                      isClockwise ? D2D1_SWEEP_DIRECTION_CLOCKWISE
                                                  : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
                                      isLarge ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL));
    }

    void end(bool isClosed) override {
        sink->EndFigure(isClosed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
    }

private:
    ID2D1GeometrySink* sink;
};

FontMetrics readMetrics(IDWriteFontCollection* collection, wchar_t const* family) {
    FontMetrics result;
    UINT32 index = 0;
    BOOL exists = FALSE;

    if (!collection || FAILED(collection->FindFamilyName(family, &index, &exists)) || !exists) return result;

    com_ptr<IDWriteFontFamily> fontFamily;
    com_ptr<IDWriteFont> font;
    if (FAILED(collection->GetFontFamily(index, fontFamily.put()))) return result;
    if (FAILED(fontFamily->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                                DWRITE_FONT_STYLE_NORMAL, font.put())))
        return result;

    DWRITE_FONT_METRICS metrics{};
    font->GetMetrics(&metrics);
    result.ascent = static_cast<float>(metrics.ascent) / metrics.designUnitsPerEm;
    result.descent = static_cast<float>(metrics.descent) / metrics.designUnitsPerEm;
    return result;
}

}

std::filesystem::path executableDirectory() {
    wchar_t buffer[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
}

Matrix Matrix::rotation(float degrees, PointF center) {
    D2D1_MATRIX_3X2_F matrix{};
    D2D1MakeRotateMatrix(degrees, d2d(center), &matrix);
    return Matrix{matrix._11, matrix._12, matrix._21, matrix._22, matrix._31, matrix._32};
}

std::unique_ptr<Canvas> Canvas::create(Resources& resources) {
    return guarded([&] { return std::make_unique<D2DCanvas>(static_cast<D2DResources&>(resources)); });
}

D2DCanvas::D2DCanvas(D2DResources& resources) : res(resources), device(Gpu::create(resources)) {}

std::shared_ptr<Image> D2DCanvas::image(uint8_t const* bgra, int width, int height) {
    return guarded([&] {
        auto properties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_NONE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));
        com_ptr<ID2D1Bitmap1> bitmap;
        winrt::check_hresult(
            device->context->CreateBitmap(D2D1::SizeU(width, height), bgra, width * 4, properties, bitmap.put()));
        return std::make_shared<D2DImage>(std::move(bitmap));
    });
}

std::vector<uint8_t> D2DCanvas::read(Image& image, PixelRect const& rect) {
    return guarded([&] {
        return readBitmap(device->context.get(), static_cast<D2DImage&>(image).get(),
                          D2D1::RectU(static_cast<UINT32>(rect.left), static_cast<UINT32>(rect.top),
                                      static_cast<UINT32>(rect.right), static_cast<UINT32>(rect.bottom)));
    });
}

std::vector<uint8_t> D2DCanvas::render(int width, int height, std::function<void(Painter&)> const& draw) {
    return guarded([&] {
        auto context = device->context.get();
        auto properties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        com_ptr<ID2D1Bitmap1> target;
        winrt::check_hresult(context->CreateBitmap(D2D1::SizeU(width, height), nullptr, 0, properties, target.put()));

        context->SetTarget(target.get());
        context->BeginDraw();
        context->SetTransform(D2D1::Matrix3x2F::Identity());
        {
            D2DPainter painter(context, res);
            draw(painter);
        }
        winrt::check_hresult(context->EndDraw());
        context->SetTarget(nullptr);

        return readBitmap(context, target.get(),
                          D2D1::RectU(0, 0, static_cast<UINT32>(width), static_cast<UINT32>(height)));
    });
}

std::shared_ptr<Resources> Resources::acquire() {
    return guarded([]() -> std::shared_ptr<Resources> { return D2DResources::acquire(); });
}

std::shared_ptr<D2DResources> D2DResources::acquire() {
    if (auto existing = sharedResources.lock()) return existing;

    auto resources = std::make_shared<D2DResources>();
    winrt::check_hresult(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory7), nullptr,
                                           resources->d2d.put_void()));
    winrt::check_hresult(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory7),
                                             reinterpret_cast<IUnknown**>(resources->write.put())));
    resources->loadFonts();
    sharedResources = resources;
    return resources;
}

D2DResources::~D2DResources() {
    if (fontLoader && write) write->UnregisterFontFileLoader(fontLoader.get());
}

void D2DResources::loadFonts() {
    if (!fontLoader) {
        if (FAILED(write->CreateInMemoryFontFileLoader(fontLoader.put()))) return;
        write->RegisterFontFileLoader(fontLoader.get());
    }

    com_ptr<IDWriteFontFallbackBuilder> builder;
    if (FAILED(write->CreateFontFallbackBuilder(builder.put()))) return;

    auto directory = resourcesPath() / L"fonts";
    size_t count = std::size(GEIST_SUBSETS);

    for (size_t index = count; index-- > 0;) {
        auto const& subset = GEIST_SUBSETS[index];
        auto bytes = readFile(directory / subset.file);
        if (bytes.empty()) continue;

        com_ptr<IDWriteFontFileStream> stream;
        if (FAILED(write->UnpackFontFile(DWRITE_CONTAINER_TYPE_WOFF2, bytes.data(), static_cast<UINT32>(bytes.size()),
                                         stream.put())))
            continue;

        UINT64 size = 0;
        void const* fragment = nullptr;
        void* context = nullptr;
        stream->GetFileSize(&size);
        if (FAILED(stream->ReadFileFragment(&fragment, 0, size, &context))) continue;

        com_ptr<IDWriteFontFile> file;
        HRESULT created = fontLoader->CreateInMemoryFontFileReference(write.get(), fragment, static_cast<UINT32>(size),
                                                                      nullptr, file.put());
        stream->ReleaseFileFragment(context);
        if (FAILED(created)) continue;

        com_ptr<IDWriteFontSetBuilder2> setBuilder;
        com_ptr<IDWriteFontSet> set;
        com_ptr<IDWriteFontCollection2> collection;
        if (FAILED(write->CreateFontSetBuilder(setBuilder.put())) || FAILED(setBuilder->AddFontFile(file.get())) ||
            FAILED(setBuilder->CreateFontSet(set.put())) ||
            FAILED(write->CreateFontCollectionFromFontSet(set.get(), DWRITE_FONT_FAMILY_MODEL_WEIGHT_STRETCH_STYLE,
                                                           collection.put())))
            continue;

        auto name = familyName(collection.get());
        if (name.empty()) continue;

        std::vector<DWRITE_UNICODE_RANGE> ranges;
        for (size_t range = 0; range < subset.count; range++)
            ranges.push_back({subset.ranges[range].first, subset.ranges[range].last});

        wchar_t const* families[] = {name.c_str()};
        builder->AddMapping(ranges.data(), static_cast<UINT32>(ranges.size()), families, 1, collection.get(), nullptr,
                            nullptr, 1.0f);

        if (!sans) {
            sans = collection;
            sansFamily = name;
        }
        subsets.push_back(collection);
    }

    com_ptr<IDWriteFontFallback> system;
    if (SUCCEEDED(write->GetSystemFontFallback(system.put()))) builder->AddMappings(system.get());
    builder->CreateFontFallback(fallback.put());
}

com_ptr<IDWriteTextFormat3> D2DResources::format(float size, DWRITE_FONT_WEIGHT weight, FontKind kind) {
    auto key = std::to_wstring(size) + L":" + std::to_wstring(static_cast<int>(weight)) + L":" +
               std::to_wstring(static_cast<int>(kind));
    if (auto found = formats.find(key); found != formats.end()) return found->second;

    com_ptr<IDWriteTextFormat> created;
    bool isEmoji = kind == FontKind::Emoji;
    winrt::check_hresult(write->CreateTextFormat(isEmoji ? L"Segoe UI Emoji" : sansFamily.c_str(),
                                                 isEmoji ? nullptr : sans.get(), weight, DWRITE_FONT_STYLE_NORMAL,
                                                 DWRITE_FONT_STRETCH_NORMAL, size, L"en-us", created.put()));

    auto result = created.as<IDWriteTextFormat3>();
    result->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    if (!isEmoji && fallback) result->SetFontFallback(fallback.get());

    if (!isEmoji) {
        DWRITE_FONT_AXIS_VALUE axis{DWRITE_FONT_AXIS_TAG_WEIGHT, static_cast<float>(weight)};
        result->SetFontAxisValues(&axis, 1);
    }

    formats[key] = result;
    return result;
}

FontMetrics D2DResources::metrics(FontKind kind) {
    if (!metricsReady) {
        metricsReady = true;
        sansMetrics = readMetrics(sans.get(), sansFamily.c_str());

        com_ptr<IDWriteFontCollection> system;
        if (SUCCEEDED(write->GetSystemFontCollection(system.put())))
            emojiMetrics = readMetrics(system.get(), L"Segoe UI Emoji");
    }

    return kind == FontKind::Emoji ? emojiMetrics : sansMetrics;
}

std::shared_ptr<TextLayout> D2DResources::layout(std::wstring_view text, float size, FontWeight weight, FontKind kind,
                                                 float lineHeight, bool tabular) {
    auto textFormat = format(size, static_cast<DWRITE_FONT_WEIGHT>(weight), kind);
    com_ptr<IDWriteTextLayout> created;
    winrt::check_hresult(write->CreateTextLayout(text.data(), static_cast<UINT32>(text.size()), textFormat.get(),
                                                 100000.0f, 100000.0f, created.put()));

    auto result = created.as<IDWriteTextLayout4>();
    if (tabular) {
        com_ptr<IDWriteTypography> typography;
        if (SUCCEEDED(write->CreateTypography(typography.put()))) {
            typography->AddFontFeature({DWRITE_FONT_FEATURE_TAG_TABULAR_FIGURES, 1});
            result->SetTypography(typography.get(), {0, static_cast<UINT32>(text.size())});
        }
    }

    if (lineHeight > 0) {
        auto fontMetrics = metrics(kind);
        DWRITE_LINE_SPACING spacing{};
        spacing.method = DWRITE_LINE_SPACING_METHOD_UNIFORM;
        spacing.height = lineHeight;
        spacing.baseline = lineHeight / 2 + (fontMetrics.ascent - fontMetrics.descent) / 2 * size;
        spacing.leadingBefore = 0;
        spacing.fontLineGapUsage = DWRITE_FONT_LINE_GAP_USAGE_DISABLED;
        result->SetLineSpacing(&spacing);
    }

    return std::make_shared<D2DTextLayout>(std::move(result));
}

ID2D1PathGeometry* D2DResources::icon(Icon const& value) {
    if (auto found = icons.find(&value); found != icons.end()) return found->second.get();

    std::vector<com_ptr<ID2D1PathGeometry>> parts;
    for (size_t index = 0; index < value.count; index++) {
        com_ptr<ID2D1PathGeometry> part;
        winrt::check_hresult(d2d->CreatePathGeometry(part.put()));

        com_ptr<ID2D1GeometrySink> partSink;
        winrt::check_hresult(part->Open(partSink.put()));
        partSink->SetFillMode(D2D1_FILL_MODE_WINDING);

        GeometrySink sink(partSink.get());
        parseSvgPath(sink, value.paths[index]);
        winrt::check_hresult(partSink->Close());
        parts.push_back(std::move(part));
    }

    com_ptr<ID2D1PathGeometry> combined;
    winrt::check_hresult(d2d->CreatePathGeometry(combined.put()));
    com_ptr<ID2D1GeometrySink> combinedSink;
    winrt::check_hresult(combined->Open(combinedSink.put()));
    for (auto& part : parts) part->Stream(combinedSink.get());
    combinedSink->Close();

    icons[&value] = combined;
    return combined.get();
}

ID2D1StrokeStyle1* D2DResources::roundStroke() {
    auto& stroke = strokes[L"round"];
    if (!stroke) {
        auto properties = D2D1::StrokeStyleProperties1(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND,
                                                       D2D1_LINE_JOIN_ROUND, 10.0f, D2D1_DASH_STYLE_SOLID, 0.0f,
                                                       D2D1_STROKE_TRANSFORM_TYPE_NORMAL);
        winrt::check_hresult(d2d->CreateStrokeStyle(properties, nullptr, 0, stroke.put()));
    }
    return stroke.get();
}

ID2D1StrokeStyle1* D2DResources::dashStroke(float dash, float gap, bool round) {
    auto key = std::to_wstring(dash) + L"/" + std::to_wstring(gap) + (round ? L"r" : L"");
    auto& stroke = strokes[key];
    if (!stroke) {
        auto cap = round ? D2D1_CAP_STYLE_ROUND : D2D1_CAP_STYLE_FLAT;
        auto properties =
            D2D1::StrokeStyleProperties1(cap, cap, cap, D2D1_LINE_JOIN_MITER, 10.0f, D2D1_DASH_STYLE_CUSTOM, 0.0f,
                                         D2D1_STROKE_TRANSFORM_TYPE_FIXED);
        float dashes[] = {dash, gap};
        winrt::check_hresult(d2d->CreateStrokeStyle(properties, dashes, 2, stroke.put()));
    }
    return stroke.get();
}

ID2D1StrokeStyle1* D2DResources::strokeStyle(Stroke const& stroke) {
    if (stroke.dash > 0) return dashStroke(stroke.dash, stroke.gap, stroke.isRound);
    return stroke.isRound ? roundStroke() : nullptr;
}

std::shared_ptr<Path> D2DResources::path(std::function<void(PathSink&)> const& build) {
    com_ptr<ID2D1PathGeometry> geometry;
    winrt::check_hresult(d2d->CreatePathGeometry(geometry.put()));

    com_ptr<ID2D1GeometrySink> target;
    winrt::check_hresult(geometry->Open(target.put()));

    GeometrySink sink(target.get());
    build(sink);

    winrt::check_hresult(target->Close());
    return std::make_shared<D2DPath>(std::move(geometry), *this);
}

std::shared_ptr<Path> D2DResources::roundedRect(RectF const& rect, float radius) {
    com_ptr<ID2D1RoundedRectangleGeometry> geometry;
    winrt::check_hresult(
        d2d->CreateRoundedRectangleGeometry(D2D1::RoundedRect(ui::d2d(rect), radius, radius), geometry.put()));
    return std::make_shared<D2DPath>(std::move(geometry), *this);
}

std::shared_ptr<Path> D2DResources::ellipse(PointF center, float radiusX, float radiusY) {
    com_ptr<ID2D1EllipseGeometry> geometry;
    winrt::check_hresult(
        d2d->CreateEllipseGeometry(D2D1::Ellipse(ui::d2d(center), radiusX, radiusY), geometry.put()));
    return std::make_shared<D2DPath>(std::move(geometry), *this);
}

TextMetrics D2DTextLayout::metrics() {
    DWRITE_TEXT_METRICS value{};
    layout->GetMetrics(&value);
    return TextMetrics{value.widthIncludingTrailingWhitespace, value.lineCount};
}

float D2DTextLayout::baseline() {
    DWRITE_LINE_METRICS line{};
    UINT32 count = 0;
    layout->GetLineMetrics(&line, 1, &count);
    return line.baseline;
}

TextCaret D2DTextLayout::caret(size_t position) {
    DWRITE_HIT_TEST_METRICS hit{};
    float x = 0;
    float y = 0;
    layout->HitTestTextPosition(static_cast<UINT32>(position), FALSE, &x, &y, &hit);
    return TextCaret{x, y, hit.height};
}

size_t D2DTextLayout::indexAt(float x, float y) {
    BOOL trailing = FALSE;
    BOOL inside = FALSE;
    DWRITE_HIT_TEST_METRICS hit{};
    layout->HitTestPoint(x, y, &trailing, &inside, &hit);
    return hit.textPosition + (trailing ? hit.length : 0);
}

std::vector<TextRange> D2DTextLayout::ranges(size_t start, size_t length) {
    UINT32 count = 0;
    layout->HitTestTextRange(static_cast<UINT32>(start), static_cast<UINT32>(length), 0, 0, nullptr, 0, &count);
    std::vector<DWRITE_HIT_TEST_METRICS> hits(count);
    layout->HitTestTextRange(static_cast<UINT32>(start), static_cast<UINT32>(length), 0, 0, hits.data(), count, &count);

    std::vector<TextRange> result;
    for (auto const& hit : hits) result.push_back(TextRange{hit.left, hit.top, hit.width, hit.height});
    return result;
}

RectF D2DPath::bounds() {
    D2D1_RECT_F value{};
    geometry->GetBounds(nullptr, &value);
    return RectF{value.left, value.top, value.right, value.bottom};
}

bool D2DPath::fillContains(PointF point) {
    BOOL contains = FALSE;
    geometry->FillContainsPoint(d2d(point), nullptr, 0.25f, &contains);
    return contains;
}

bool D2DPath::strokeContains(PointF point, float width, Stroke const& stroke) {
    BOOL contains = FALSE;
    geometry->StrokeContainsPoint(d2d(point), width, resources.strokeStyle(stroke), nullptr, 0.25f, &contains);
    return contains;
}

D2DPainter::D2DPainter(ID2D1DeviceContext* context, D2DResources& resources) : target(context), res(resources) {}

Resources& D2DPainter::resources() {
    return res;
}

ID2D1SolidColorBrush* D2DPainter::brush(Color const& value) {
    if (!solid) winrt::check_hresult(target->CreateSolidColorBrush(d2d(value), solid.put()));
    solid->SetColor(d2d(value));
    return solid.get();
}

void D2DPainter::clear(Color const& value) {
    target->Clear(d2d(value));
}

void D2DPainter::fillFrame(RectF const& outer, RectF const& inner, Color const& value) {
    com_ptr<ID2D1RectangleGeometry> first;
    com_ptr<ID2D1RectangleGeometry> second;
    res.factory()->CreateRectangleGeometry(d2d(outer), first.put());
    res.factory()->CreateRectangleGeometry(d2d(inner), second.put());

    ID2D1Geometry* parts[] = {first.get(), second.get()};
    com_ptr<ID2D1GeometryGroup> group;
    res.factory()->CreateGeometryGroup(D2D1_FILL_MODE_ALTERNATE, parts, 2, group.put());
    target->FillGeometry(group.get(), brush(value));
}

void D2DPainter::fillRect(RectF const& rect, Color const& value) {
    target->FillRectangle(d2d(rect), brush(value));
}

void D2DPainter::strokeRect(RectF const& rect, Color const& value, float width, Stroke const& stroke) {
    target->DrawRectangle(d2d(rect), brush(value), width, res.strokeStyle(stroke));
}

void D2DPainter::fillRounded(RectF const& rect, float radius, Color const& value) {
    float limit = std::min(rect.right - rect.left, rect.bottom - rect.top) / 2;
    radius = std::max(0.0f, std::min(radius, limit));
    target->FillRoundedRectangle(D2D1::RoundedRect(d2d(rect), radius, radius), brush(value));
}

void D2DPainter::strokeRounded(RectF const& rect, float radius, Color const& value, float width,
                               Stroke const& stroke) {
    float limit = std::min(rect.right - rect.left, rect.bottom - rect.top) / 2;
    radius = std::max(0.0f, std::min(radius, limit));
    target->DrawRoundedRectangle(D2D1::RoundedRect(d2d(rect), radius, radius), brush(value), width,
                                 res.strokeStyle(stroke));
}

void D2DPainter::fillEllipse(float cx, float cy, float rx, float ry, Color const& value) {
    target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), rx, ry), brush(value));
}

void D2DPainter::strokeEllipse(float cx, float cy, float rx, float ry, Color const& value, float width) {
    target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), rx, ry), brush(value), width);
}

void D2DPainter::line(PointF from, PointF to, Color const& value, float width) {
    target->DrawLine(d2d(from), d2d(to), brush(value), width);
}

void D2DPainter::fillPath(Path& path, Color const& value) {
    target->FillGeometry(static_cast<D2DPath&>(path).get(), brush(value));
}

void D2DPainter::strokePath(Path& path, Color const& value, float width, Stroke const& stroke) {
    target->DrawGeometry(static_cast<D2DPath&>(path).get(), brush(value), width, res.strokeStyle(stroke));
}

void D2DPainter::icon(Icon const& value, float x, float y, float size, Color const& tint, bool fill,
                      float strokeWidth) {
    auto geometry = res.icon(value);
    float scale = size / 24.0f;
    Transform transform(*this, Matrix::scale(scale, scale) * Matrix::translation(x, y));

    if (fill) target->FillGeometry(geometry, brush(tint));
    target->DrawGeometry(geometry, brush(tint), strokeWidth, res.roundStroke());
}

void D2DPainter::text(TextLayout& layout, float x, float y, Color const& value) {
    target->DrawTextLayout(D2D1::Point2F(x, y), static_cast<D2DTextLayout&>(layout).get(), brush(value),
                           D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
}

void D2DPainter::boxShadow(RectF const& rect, float radius, float offsetY, float blur, float spread, float alpha) {
    D2D1_MATRIX_3X2_F matrix{};
    target->GetTransform(&matrix);
    float scale = std::sqrt(std::abs(matrix._11 * matrix._22 - matrix._12 * matrix._21));

    com_ptr<ID2D1CommandList> list;
    if (FAILED(target->CreateCommandList(list.put()))) return;

    com_ptr<ID2D1Image> previous;
    target->GetTarget(previous.put());
    target->SetTarget(list.get());

    auto inflated = RectF{rect.left - spread, rect.top - spread, rect.right + spread, rect.bottom + spread};
    fillRounded(inflated, std::max(0.0f, radius + spread), Color{0, 0, 0, 1});

    target->SetTarget(previous.get());
    list->Close();

    com_ptr<ID2D1Effect> shadow;
    if (FAILED(target->CreateEffect(SHADOW_EFFECT, shadow.put()))) return;

    shadow->SetInput(0, list.get());
    shadow->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, std::max(0.01f, blur / 2 * scale));
    shadow->SetValue(D2D1_SHADOW_PROP_COLOR, D2D1::Vector4F(0, 0, 0, alpha));

    target->SetTransform(D2D1::Matrix3x2F::Identity());
    target->DrawImage(shadow.get(), D2D1::Point2F(matrix._21 * offsetY, matrix._22 * offsetY));
    target->SetTransform(matrix);
}

void D2DPainter::shadowed(float blur, float offset, float alpha, std::function<void()> const& draw) {
    com_ptr<ID2D1CommandList> list;
    winrt::check_hresult(target->CreateCommandList(list.put()));

    com_ptr<ID2D1Image> previous;
    target->GetTarget(previous.put());
    target->SetTarget(list.get());
    draw();
    target->SetTarget(previous.get());
    list->Close();

    D2D1_MATRIX_3X2_F matrix{};
    target->GetTransform(&matrix);
    float scaleX = std::abs(matrix._11);
    float scaleY = std::abs(matrix._22);

    com_ptr<ID2D1Effect> shadow;
    winrt::check_hresult(target->CreateEffect(SHADOW_EFFECT, shadow.put()));
    shadow->SetInput(0, list.get());
    shadow->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, blur * std::min(scaleX, scaleY));
    shadow->SetValue(D2D1_SHADOW_PROP_COLOR, D2D1::Vector4F(0, 0, 0, alpha));

    target->SetTransform(D2D1::Matrix3x2F::Identity());
    target->DrawImage(shadow.get(), D2D1::Point2F(offset * scaleX, offset * scaleY));
    target->DrawImage(list.get());
    target->SetTransform(matrix);
}

std::shared_ptr<Image> D2DPainter::image(uint8_t const* bgra, int width, int height) {
    auto properties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
    com_ptr<ID2D1Bitmap1> bitmap;
    winrt::check_hresult(target->CreateBitmap(D2D1::SizeU(width, height), bgra, width * 4, properties, bitmap.put()));
    return std::make_shared<D2DImage>(std::move(bitmap));
}

void D2DPainter::drawImage(Image& image, RectF const& rect) {
    target->DrawBitmap(static_cast<D2DImage&>(image).get(), d2d(rect), 1.0f, D2D1_INTERPOLATION_MODE_LINEAR, nullptr,
                       nullptr);
}

void D2DPainter::drawPixels(Image& image, RectF const& source, RectF const& rect) {
    auto cut = d2d(source);
    target->DrawBitmap(static_cast<D2DImage&>(image).get(), d2d(rect), 1.0f, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                       &cut, nullptr);
}

void D2DPainter::pushOpacity(float opacity) {
    target->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                            D2D1::IdentityMatrix(), opacity),
                      nullptr);
}

void D2DPainter::popOpacity() {
    target->PopLayer();
}

void D2DPainter::pushClip(Path& path) {
    target->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), static_cast<D2DPath&>(path).get()), nullptr);
}

void D2DPainter::popClip() {
    target->PopLayer();
}

Matrix D2DPainter::transform() {
    D2D1_MATRIX_3X2_F matrix{};
    target->GetTransform(&matrix);
    return Matrix{matrix._11, matrix._12, matrix._21, matrix._22, matrix._31, matrix._32};
}

void D2DPainter::setTransform(Matrix const& matrix) {
    target->SetTransform(d2d(matrix));
}

std::unique_ptr<Gpu> Gpu::create(D2DResources& resources) {
    auto gpu = std::make_unique<Gpu>();
    D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
                                  D3D_FEATURE_LEVEL_10_0};
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels,
                                       static_cast<UINT>(std::size(levels)), D3D11_SDK_VERSION, gpu->d3d.put(),
                                       nullptr, nullptr);
    if (FAILED(result)) {
        winrt::check_hresult(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags, levels,
                                               static_cast<UINT>(std::size(levels)), D3D11_SDK_VERSION,
                                               gpu->d3d.put(), nullptr, nullptr));
    }

    gpu->dxgi = gpu->d3d.as<IDXGIDevice>();
    winrt::check_hresult(resources.factory()->CreateDevice(gpu->dxgi.get(), gpu->device.put()));

    com_ptr<ID2D1DeviceContext6> context;
    winrt::check_hresult(gpu->device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, context.put()));
    gpu->context = context.as<ID2D1DeviceContext5>();
    gpu->context->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

    winrt::check_hresult(DCompositionCreateDevice(gpu->dxgi.get(), __uuidof(IDCompositionDevice),
                                                  gpu->composition.put_void()));
    return gpu;
}

bool SwapSurface::create(Gpu& gpu, HWND window, int width, int height, bool transparent) {
    com_ptr<IDXGIAdapter> adapter;
    com_ptr<IDXGIFactory2> factory;
    winrt::check_hresult(gpu.dxgi->GetAdapter(adapter.put()));
    winrt::check_hresult(adapter->GetParent(__uuidof(IDXGIFactory2), factory.put_void()));

    DXGI_SWAP_CHAIN_DESC1 description{};
    description.Width = static_cast<UINT>(width);
    description.Height = static_cast<UINT>(height);
    description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.BufferCount = 2;
    description.Scaling = DXGI_SCALING_STRETCH;
    description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    description.AlphaMode = transparent ? DXGI_ALPHA_MODE_PREMULTIPLIED : DXGI_ALPHA_MODE_IGNORE;

    winrt::check_hresult(factory->CreateSwapChainForComposition(gpu.d3d.get(), &description, nullptr, chain.put()));
    winrt::check_hresult(gpu.composition->CreateTargetForHwnd(window, TRUE, target.put()));
    winrt::check_hresult(gpu.composition->CreateVisual(visual.put()));
    winrt::check_hresult(visual->SetContent(chain.get()));
    winrt::check_hresult(target->SetRoot(visual.get()));
    return true;
}

ID2D1DeviceContext5* SwapSurface::begin(Gpu& gpu) {
    com_ptr<IDXGISurface> surface;
    winrt::check_hresult(chain->GetBuffer(0, __uuidof(IDXGISurface), surface.put_void()));

    DXGI_SWAP_CHAIN_DESC1 description{};
    chain->GetDesc1(&description);
    auto alpha = description.AlphaMode == DXGI_ALPHA_MODE_PREMULTIPLIED ? D2D1_ALPHA_MODE_PREMULTIPLIED
                                                                        : D2D1_ALPHA_MODE_IGNORE;
    auto properties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                                              D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, alpha));
    winrt::check_hresult(gpu.context->CreateBitmapFromDxgiSurface(surface.get(), properties, buffer.put()));

    gpu.context->SetTarget(buffer.get());
    gpu.context->BeginDraw();
    gpu.context->SetTransform(D2D1::Matrix3x2F::Identity());
    return gpu.context.get();
}

void SwapSurface::present(Gpu& gpu) {
    gpu.context->EndDraw();
    gpu.context->SetTarget(nullptr);
    buffer = nullptr;
    chain->Present(1, 0);

    if (!committed) {
        gpu.composition->Commit();
        committed = true;
    }
}

void SwapSurface::release() {
    buffer = nullptr;
    visual = nullptr;
    target = nullptr;
    chain = nullptr;
    committed = false;
}

std::vector<uint8_t> readBitmap(ID2D1DeviceContext* context, ID2D1Bitmap1* source, D2D1_RECT_U const& rect) {
    UINT32 width = rect.right - rect.left;
    UINT32 height = rect.bottom - rect.top;
    std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
    if (!width || !height) return pixels;

    auto properties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                                              source->GetPixelFormat());
    com_ptr<ID2D1Bitmap1> staging;
    winrt::check_hresult(context->CreateBitmap(D2D1::SizeU(width, height), nullptr, 0, properties, staging.put()));

    auto origin = D2D1::Point2U(0, 0);
    winrt::check_hresult(staging->CopyFromBitmap(&origin, source, &rect));

    D2D1_MAPPED_RECT mapped{};
    winrt::check_hresult(staging->Map(D2D1_MAP_OPTIONS_READ, &mapped));
    for (UINT32 row = 0; row < height; row++)
        memcpy(pixels.data() + static_cast<size_t>(row) * width * 4, mapped.bits + static_cast<size_t>(row) * mapped.pitch,
               static_cast<size_t>(width) * 4);
    staging->Unmap();
    return pixels;
}

}

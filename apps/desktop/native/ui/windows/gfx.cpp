#include "direct2d.h"

#include <d2d1_3helper.h>
#include <d2d1effects_2.h>

#include <fonts.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>

namespace fileship::ui {

namespace {

constexpr CLSID SHADOW_EFFECT{0xC67EA361, 0x1863, 0x4E69, {0x89, 0xDB, 0x69, 0x5D, 0x3E, 0x9A, 0x5B, 0x6B}};

D2D1_RECT_F native(RectF const& rect) {
    return D2D1::RectF(rect.left, rect.top, rect.right, rect.bottom);
}

D2D1_COLOR_F native(Color const& value) {
    return D2D1::ColorF(value.r, value.g, value.b, value.a);
}

D2D1_POINT_2F native(PointF const& point) {
    return D2D1::Point2F(point.x, point.y);
}

D2D1_MATRIX_3X2_F native(Matrix const& matrix) {
    return D2D1::Matrix3x2F(matrix.m11, matrix.m12, matrix.m21, matrix.m22, matrix.dx, matrix.dy);
}

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

class DirectTextLayout : public TextLayout {
public:
    explicit DirectTextLayout(com_ptr<IDWriteTextLayout4> layout) : layout(std::move(layout)) {}

    TextMetrics metrics() const override {
        DWRITE_TEXT_METRICS metrics{};
        layout->GetMetrics(&metrics);
        return {metrics.width, metrics.widthIncludingTrailingWhitespace, metrics.height, metrics.lineCount};
    }

    float baseline() const override {
        DWRITE_LINE_METRICS line{};
        UINT32 count = 0;
        layout->GetLineMetrics(&line, 1, &count);
        return line.baseline;
    }

    TextPosition position(size_t index) const override {
        float x = 0;
        float y = 0;
        DWRITE_HIT_TEST_METRICS hit{};
        layout->HitTestTextPosition(static_cast<UINT32>(index), FALSE, &x, &y, &hit);
        return {x, y, hit.height};
    }

    std::vector<RectF> rangeRects(size_t start, size_t length) const override {
        UINT32 count = 0;
        layout->HitTestTextRange(static_cast<UINT32>(start), static_cast<UINT32>(length), 0, 0, nullptr, 0, &count);
        std::vector<DWRITE_HIT_TEST_METRICS> ranges(count);
        layout->HitTestTextRange(static_cast<UINT32>(start), static_cast<UINT32>(length), 0, 0, ranges.data(), count,
                                 &count);

        std::vector<RectF> result;
        for (auto const& range : ranges)
            result.push_back({range.left, range.top, range.left + range.width, range.top + range.height});
        return result;
    }

    size_t indexAt(float x, float y) const override {
        BOOL trailing = FALSE;
        BOOL inside = FALSE;
        DWRITE_HIT_TEST_METRICS hit{};
        layout->HitTestPoint(x, y, &trailing, &inside, &hit);
        return hit.textPosition + (trailing ? hit.length : 0);
    }

    com_ptr<IDWriteTextLayout4> layout;
};

class DirectImage : public Image {
public:
    explicit DirectImage(com_ptr<ID2D1Bitmap1> bitmap) : bitmap(std::move(bitmap)) {}

    int width() const override {
        return static_cast<int>(bitmap->GetPixelSize().width);
    }

    int height() const override {
        return static_cast<int>(bitmap->GetPixelSize().height);
    }

    com_ptr<ID2D1Bitmap1> bitmap;
};

std::shared_ptr<Image> createBitmap(ID2D1DeviceContext* context, uint8_t const* bgra, int width, int height,
                                    bool isOpaque) {
    auto properties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, isOpaque ? D2D1_ALPHA_MODE_IGNORE : D2D1_ALPHA_MODE_PREMULTIPLIED));
    com_ptr<ID2D1Bitmap1> bitmap;
    winrt::check_hresult(context->CreateBitmap(D2D1::SizeU(width, height), bgra, width * 4, properties, bitmap.put()));
    return std::make_shared<DirectImage>(std::move(bitmap));
}

}

struct Resources::Backend {
    com_ptr<ID2D1Factory7> d2d;
    com_ptr<IDWriteFactory7> write;
    com_ptr<IDWriteFontCollection1> sans;
    com_ptr<IDWriteFontFallback> fallback;
    com_ptr<IDWriteInMemoryFontFileLoader> fontLoader;
    std::wstring sansFamily = L"Segoe UI";
    std::vector<com_ptr<IDWriteFontCollection1>> subsets;
    std::unordered_map<std::wstring, com_ptr<ID2D1StrokeStyle1>> strokes;
    std::unordered_map<std::wstring, com_ptr<IDWriteTextFormat3>> formats;
    FontMetrics sansMetrics;
    FontMetrics emojiMetrics;
    bool metricsReady = false;

    void loadFonts() {
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
            if (FAILED(write->UnpackFontFile(DWRITE_CONTAINER_TYPE_WOFF2, bytes.data(),
                                             static_cast<UINT32>(bytes.size()), stream.put())))
                continue;

            UINT64 size = 0;
            void const* fragment = nullptr;
            void* context = nullptr;
            stream->GetFileSize(&size);
            if (FAILED(stream->ReadFileFragment(&fragment, 0, size, &context))) continue;

            com_ptr<IDWriteFontFile> file;
            HRESULT created = fontLoader->CreateInMemoryFontFileReference(write.get(), fragment,
                                                                          static_cast<UINT32>(size), nullptr, file.put());
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
            builder->AddMapping(ranges.data(), static_cast<UINT32>(ranges.size()), families, 1, collection.get(),
                                nullptr, nullptr, 1.0f);

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

    com_ptr<IDWriteTextFormat3> format(float size, FontWeight weight, FontKind kind) {
        auto key = std::to_wstring(size) + L":" + std::to_wstring(static_cast<int>(weight)) + L":" +
                   std::to_wstring(static_cast<int>(kind));
        if (auto found = formats.find(key); found != formats.end()) return found->second;

        com_ptr<IDWriteTextFormat> created;
        bool isEmoji = kind == FontKind::Emoji;
        auto fontWeight = static_cast<DWRITE_FONT_WEIGHT>(weight);
        winrt::check_hresult(write->CreateTextFormat(isEmoji ? L"Segoe UI Emoji" : sansFamily.c_str(),
                                                     isEmoji ? nullptr : sans.get(), fontWeight,
                                                     DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size,
                                                     L"en-us", created.put()));

        auto result = created.as<IDWriteTextFormat3>();
        result->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        if (!isEmoji && fallback) result->SetFontFallback(fallback.get());

        if (!isEmoji) {
            DWRITE_FONT_AXIS_VALUE axis{DWRITE_FONT_AXIS_TAG_WEIGHT, static_cast<float>(fontWeight)};
            result->SetFontAxisValues(&axis, 1);
        }

        formats[key] = result;
        return result;
    }

    ID2D1StrokeStyle1* stroke(StrokeStyle const& style) {
        auto key = std::to_wstring(static_cast<int>(style.cap)) + L":" + std::to_wstring(style.roundJoin) + L":" +
                   std::to_wstring(style.dash) + L"/" + std::to_wstring(style.gap);
        auto& stroke = strokes[key];
        if (stroke) return stroke.get();

        auto cap = style.cap == CapStyle::Round ? D2D1_CAP_STYLE_ROUND : D2D1_CAP_STYLE_FLAT;
        if (style.dash > 0) {
            auto properties =
                D2D1::StrokeStyleProperties1(cap, cap, cap, D2D1_LINE_JOIN_MITER, 10.0f, D2D1_DASH_STYLE_CUSTOM, 0.0f,
                                             D2D1_STROKE_TRANSFORM_TYPE_FIXED);
            float dashes[] = {style.dash, style.gap};
            winrt::check_hresult(d2d->CreateStrokeStyle(properties, dashes, 2, stroke.put()));
        } else {
            auto properties = D2D1::StrokeStyleProperties1(
                cap, cap, cap, style.roundJoin ? D2D1_LINE_JOIN_ROUND : D2D1_LINE_JOIN_MITER, 10.0f,
                D2D1_DASH_STYLE_SOLID, 0.0f, D2D1_STROKE_TRANSFORM_TYPE_NORMAL);
            winrt::check_hresult(d2d->CreateStrokeStyle(properties, nullptr, 0, stroke.put()));
        }

        return stroke.get();
    }

    ID2D1PathGeometry* geometry(Path const& path) {
        if (path.compiled) return static_cast<ID2D1PathGeometry*>(path.compiled.get());

        com_ptr<ID2D1PathGeometry> geometry;
        winrt::check_hresult(d2d->CreatePathGeometry(geometry.put()));

        com_ptr<ID2D1GeometrySink> sink;
        winrt::check_hresult(geometry->Open(sink.put()));
        sink->SetFillMode(path.fill == FillMode::Alternate ? D2D1_FILL_MODE_ALTERNATE : D2D1_FILL_MODE_WINDING);

        bool isOpen = false;
        for (auto const& segment : path.segments) {
            auto const& points = segment.points;
            switch (segment.verb) {
            case Path::Verb::Move:
                if (isOpen) sink->EndFigure(D2D1_FIGURE_END_OPEN);
                sink->BeginFigure(native(points[0]), D2D1_FIGURE_BEGIN_FILLED);
                isOpen = true;
                break;
            case Path::Verb::Line:
                sink->AddLine(native(points[0]));
                break;
            case Path::Verb::Quad:
                sink->AddQuadraticBezier(D2D1::QuadraticBezierSegment(native(points[0]), native(points[1])));
                break;
            case Path::Verb::Cubic:
                sink->AddBezier(D2D1::BezierSegment(native(points[0]), native(points[1]), native(points[2])));
                break;
            case Path::Verb::Arc:
                sink->AddArc(D2D1::ArcSegment(native(points[0]), D2D1::SizeF(segment.rx, segment.ry), segment.rotation,
                                              segment.sweep ? D2D1_SWEEP_DIRECTION_CLOCKWISE
                                                            : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
                                              segment.large ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL));
                break;
            case Path::Verb::Close:
                if (isOpen) sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                isOpen = false;
                break;
            }
        }

        if (isOpen) sink->EndFigure(D2D1_FIGURE_END_OPEN);
        winrt::check_hresult(sink->Close());

        auto raw = geometry.detach();
        path.compiled = std::shared_ptr<void>(raw, [](void* value) { static_cast<ID2D1PathGeometry*>(value)->Release(); });
        return raw;
    }
};

namespace {

class DirectPainter : public Painter {
public:
    DirectPainter(ID2D1DeviceContext* context, Resources& resources)
        : Painter(resources), target(context), backend(resources.backend()) {}

    void clear(Color const& value) override {
        target->Clear(native(value));
    }

    void fillRect(RectF const& rect, Color const& value) override {
        target->FillRectangle(native(rect), brush(value));
    }

    void strokeRect(RectF const& rect, Color const& value, float width, StrokeStyle const* style) override {
        target->DrawRectangle(native(rect), brush(value), width, style ? backend.stroke(*style) : nullptr);
    }

    void fillRounded(RectF const& rect, float radius, Color const& value) override {
        float limit = std::min(rect.right - rect.left, rect.bottom - rect.top) / 2;
        radius = std::max(0.0f, std::min(radius, limit));
        target->FillRoundedRectangle(D2D1::RoundedRect(native(rect), radius, radius), brush(value));
    }

    void strokeRounded(RectF const& rect, float radius, Color const& value, float width,
                       StrokeStyle const* style) override {
        float limit = std::min(rect.right - rect.left, rect.bottom - rect.top) / 2;
        radius = std::max(0.0f, std::min(radius, limit));
        target->DrawRoundedRectangle(D2D1::RoundedRect(native(rect), radius, radius), brush(value), width,
                                     style ? backend.stroke(*style) : nullptr);
    }

    void fillEllipse(float cx, float cy, float rx, float ry, Color const& value) override {
        target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), rx, ry), brush(value));
    }

    void strokeEllipse(float cx, float cy, float rx, float ry, Color const& value, float width) override {
        target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), rx, ry), brush(value), width);
    }

    void line(PointF from, PointF to, Color const& value, float width) override {
        target->DrawLine(native(from), native(to), brush(value), width);
    }

    void fillPath(Path const& path, Color const& value) override {
        target->FillGeometry(backend.geometry(path), brush(value));
    }

    void strokePath(Path const& path, Color const& value, float width, StrokeStyle const& style) override {
        target->DrawGeometry(backend.geometry(path), brush(value), width, backend.stroke(style));
    }

    void text(TextLayout const& layout, float x, float y, Color const& value) override {
        target->DrawTextLayout(D2D1::Point2F(x, y), static_cast<DirectTextLayout const&>(layout).layout.get(),
                               brush(value), D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
    }

    void image(Image const& source, RectF const& destination, std::optional<RectF> const& cut,
               Interpolation interpolation) override {
        auto rect = cut ? native(*cut) : D2D1_RECT_F{};
        target->DrawBitmap(static_cast<DirectImage const&>(source).bitmap.get(), native(destination), 1.0f,
                           interpolation == Interpolation::Nearest ? D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR
                                                                   : D2D1_INTERPOLATION_MODE_LINEAR,
                           cut ? &rect : nullptr, nullptr);
    }

    std::shared_ptr<Image> createImage(uint8_t const* bgra, int width, int height, bool isOpaque) override {
        return createBitmap(target, bgra, width, height, isOpaque);
    }

    void boxShadow(RectF const& rect, float radius, float offsetY, float blur, float spread, float alpha) override {
        D2D1_MATRIX_3X2_F matrix{};
        target->GetTransform(&matrix);
        float scale = std::sqrt(std::abs(matrix._11 * matrix._22 - matrix._12 * matrix._21));

        com_ptr<ID2D1CommandList> list;
        if (FAILED(target->CreateCommandList(list.put()))) return;

        com_ptr<ID2D1Image> previous;
        target->GetTarget(previous.put());
        target->SetTarget(list.get());

        auto inflated = RectF(rect.left - spread, rect.top - spread, rect.right + spread, rect.bottom + spread);
        fillRounded(inflated, std::max(0.0f, radius + spread), Color(0, 0, 0, 1));

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

    void pushOpacity(float opacity) override {
        target->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                                D2D1::IdentityMatrix(), opacity),
                          nullptr);
    }

    void popOpacity() override {
        target->PopLayer();
    }

    void pushClip(RectF const& rect, float radius) override {
        com_ptr<ID2D1RoundedRectangleGeometry> clip;
        backend.d2d->CreateRoundedRectangleGeometry(D2D1::RoundedRect(native(rect), radius, radius), clip.put());
        target->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), clip.get()), nullptr);
    }

    void popClip() override {
        target->PopLayer();
    }

    void pushTransform(Matrix const& local) override {
        D2D1_MATRIX_3X2_F saved{};
        target->GetTransform(&saved);
        transforms.push_back(saved);
        target->SetTransform(native(local) * saved);
    }

    void popTransform() override {
        target->SetTransform(transforms.back());
        transforms.pop_back();
    }

    Matrix transform() const override {
        D2D1_MATRIX_3X2_F matrix{};
        target->GetTransform(&matrix);
        return {matrix._11, matrix._12, matrix._21, matrix._22, matrix._31, matrix._32};
    }

    void beginShadow() override {
        com_ptr<ID2D1CommandList> list;
        winrt::check_hresult(target->CreateCommandList(list.put()));

        com_ptr<ID2D1Image> previous;
        target->GetTarget(previous.put());
        target->SetTarget(list.get());
        shadows.push_back({std::move(list), std::move(previous)});
    }

    void endShadow(float offsetX, float offsetY, float blur, float alpha) override {
        auto [list, previous] = std::move(shadows.back());
        shadows.pop_back();
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
        target->DrawImage(shadow.get(), D2D1::Point2F(offsetX * scaleX, offsetY * scaleY));
        target->DrawImage(list.get());
        target->SetTransform(matrix);
    }

private:
    ID2D1SolidColorBrush* brush(Color const& value) {
        if (!solid) winrt::check_hresult(target->CreateSolidColorBrush(native(value), solid.put()));
        solid->SetColor(native(value));
        return solid.get();
    }

    ID2D1DeviceContext* target;
    Resources::Backend& backend;
    com_ptr<ID2D1SolidColorBrush> solid;
    std::vector<D2D1_MATRIX_3X2_F> transforms;
    std::vector<std::pair<com_ptr<ID2D1CommandList>, com_ptr<ID2D1Image>>> shadows;
};

class DirectCanvas : public Canvas {
public:
    DirectCanvas(Gpu& gpu, Resources& resources, int width, int height) : gpu(gpu), width(width), height(height) {
        auto context = gpu.context.get();
        auto properties =
            D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,
                                    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        winrt::check_hresult(context->CreateBitmap(D2D1::SizeU(width, height), nullptr, 0, properties, bitmap.put()));

        context->SetTarget(bitmap.get());
        context->BeginDraw();
        context->SetTransform(D2D1::Matrix3x2F::Identity());
        drawing = std::make_unique<DirectPainter>(context, resources);
    }

    Painter& painter() override {
        return *drawing;
    }

    std::vector<uint8_t> finish() override {
        auto context = gpu.context.get();
        drawing.reset();
        winrt::check_hresult(context->EndDraw());
        context->SetTarget(nullptr);
        return readBitmap(context, bitmap.get(),
                          D2D1::RectU(0, 0, static_cast<UINT32>(width), static_cast<UINT32>(height)));
    }

private:
    Gpu& gpu;
    int width;
    int height;
    com_ptr<ID2D1Bitmap1> bitmap;
    std::unique_ptr<DirectPainter> drawing;
};

class DirectDevice : public Device {
public:
    explicit DirectDevice(Resources& resources) : resources(resources), gpu(Gpu::create(resources)) {}

    std::shared_ptr<Image> createImage(uint8_t const* bgra, int width, int height, bool isOpaque) override {
        return createBitmap(gpu->context.get(), bgra, width, height, isOpaque);
    }

    std::vector<uint8_t> readImage(Image const& image, PixelRect const& rect) override {
        return readBitmap(gpu->context.get(), static_cast<DirectImage const&>(image).bitmap.get(),
                          D2D1::RectU(static_cast<UINT32>(rect.left), static_cast<UINT32>(rect.top),
                                      static_cast<UINT32>(rect.right), static_cast<UINT32>(rect.bottom)));
    }

    std::unique_ptr<Canvas> createCanvas(int width, int height) override {
        return std::make_unique<DirectCanvas>(*gpu, resources, width, height);
    }

    Resources& resources;
    std::unique_ptr<Gpu> gpu;
};

}

Resources::Resources() : state(std::make_unique<Backend>()) {
    winrt::check_hresult(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory7), nullptr,
                                           state->d2d.put_void()));
    winrt::check_hresult(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory7),
                                             reinterpret_cast<IUnknown**>(state->write.put())));
    state->loadFonts();
}

Resources::~Resources() {
    icons.clear();
    if (state->fontLoader && state->write) state->write->UnregisterFontFileLoader(state->fontLoader.get());
}

FontMetrics Resources::metrics(FontKind kind) {
    auto& backend = *state;
    if (!backend.metricsReady) {
        backend.metricsReady = true;
        backend.sansMetrics = readMetrics(backend.sans.get(), backend.sansFamily.c_str());

        com_ptr<IDWriteFontCollection> system;
        if (SUCCEEDED(backend.write->GetSystemFontCollection(system.put())))
            backend.emojiMetrics = readMetrics(system.get(), L"Segoe UI Emoji");
    }

    return kind == FontKind::Emoji ? backend.emojiMetrics : backend.sansMetrics;
}

std::shared_ptr<TextLayout> Resources::layout(std::wstring_view text, float size, FontWeight weight, FontKind kind,
                                              float lineHeight, bool tabular) {
    auto& backend = *state;
    auto textFormat = backend.format(size, weight, kind);
    com_ptr<IDWriteTextLayout> created;
    winrt::check_hresult(backend.write->CreateTextLayout(text.data(), static_cast<UINT32>(text.size()),
                                                         textFormat.get(), 100000.0f, 100000.0f, created.put()));

    auto result = created.as<IDWriteTextLayout4>();
    if (tabular) {
        com_ptr<IDWriteTypography> typography;
        if (SUCCEEDED(backend.write->CreateTypography(typography.put()))) {
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

    return std::make_shared<DirectTextLayout>(std::move(result));
}

RectF Resources::bounds(Path const& path) {
    D2D1_RECT_F bounds{};
    state->geometry(path)->GetBounds(nullptr, &bounds);
    return {bounds.left, bounds.top, bounds.right, bounds.bottom};
}

bool Resources::fillContains(Path const& path, PointF point) {
    BOOL contains = FALSE;
    state->geometry(path)->FillContainsPoint(native(point), nullptr, 0.25f, &contains);
    return contains;
}

bool Resources::strokeContains(Path const& path, PointF point, float width, StrokeStyle const* style) {
    BOOL contains = FALSE;
    state->geometry(path)->StrokeContainsPoint(native(point), width, style ? state->stroke(*style) : nullptr, nullptr,
                                               0.25f, &contains);
    return contains;
}

std::unique_ptr<Device> Device::create(Resources& resources) {
    return std::make_unique<DirectDevice>(resources);
}

std::filesystem::path executableDirectory() {
    wchar_t buffer[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
}

double caretBlinkMs() {
    double blink = GetCaretBlinkTime();
    return blink >= INFINITE ? 0 : blink;
}

std::unique_ptr<Gpu> Gpu::create(Resources& resources) {
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
    winrt::check_hresult(factoryOf(resources)->CreateDevice(gpu->dxgi.get(), gpu->device.put()));

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

std::unique_ptr<Painter> createPainter(ID2D1DeviceContext* context, Resources& resources) {
    return std::make_unique<DirectPainter>(context, resources);
}

Gpu& gpuOf(Device& device) {
    return *static_cast<DirectDevice&>(device).gpu;
}

ID2D1Factory7* factoryOf(Resources& resources) {
    return resources.backend().d2d.get();
}

}

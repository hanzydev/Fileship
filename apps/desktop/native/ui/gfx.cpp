#include "gfx.h"

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

std::weak_ptr<Resources> sharedResources;
std::filesystem::path customResources;

std::filesystem::path executableDirectory() {
    wchar_t buffer[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
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

class PathReader {
public:
    explicit PathReader(char const* data) : cursor(data) {}

    void skip() {
        while (*cursor == ' ' || *cursor == ',' || *cursor == '\t' || *cursor == '\n' || *cursor == '\r') cursor++;
    }

    bool hasNumber() {
        skip();
        return *cursor == '-' || *cursor == '+' || *cursor == '.' || (*cursor >= '0' && *cursor <= '9');
    }

    char command() {
        skip();
        return *cursor ? *cursor++ : 0;
    }

    float number() {
        skip();
        char const* start = cursor;

        if (*cursor == '-' || *cursor == '+') cursor++;
        while (*cursor >= '0' && *cursor <= '9') cursor++;
        if (*cursor == '.') {
            cursor++;
            while (*cursor >= '0' && *cursor <= '9') cursor++;
        }
        if (*cursor == 'e' || *cursor == 'E') {
            cursor++;
            if (*cursor == '-' || *cursor == '+') cursor++;
            while (*cursor >= '0' && *cursor <= '9') cursor++;
        }

        return std::strtof(std::string(start, cursor).c_str(), nullptr);
    }

    bool flag() {
        skip();
        return *cursor ? *cursor++ == '1' : false;
    }

    bool done() {
        skip();
        return !*cursor;
    }

private:
    char const* cursor;
};

}

void setResourcesPath(std::filesystem::path const& path) {
    customResources = path;
}

std::filesystem::path resourcesPath() {
    return customResources.empty() ? executableDirectory() : customResources;
}

std::shared_ptr<Resources> Resources::acquire() {
    if (auto existing = sharedResources.lock()) return existing;

    auto resources = std::make_shared<Resources>();
    winrt::check_hresult(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory7), nullptr,
                                           resources->d2d.put_void()));
    winrt::check_hresult(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory7),
                                             reinterpret_cast<IUnknown**>(resources->write.put())));
    resources->loadFonts();
    sharedResources = resources;
    return resources;
}

Resources::~Resources() {
    if (fontLoader && write) write->UnregisterFontFileLoader(fontLoader.get());
}

void Resources::loadFonts() {
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

com_ptr<IDWriteTextFormat3> Resources::format(float size, DWRITE_FONT_WEIGHT weight, FontKind kind) {
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

FontMetrics Resources::metrics(FontKind kind) {
    if (!metricsReady) {
        metricsReady = true;
        sansMetrics = readMetrics(sans.get(), sansFamily.c_str());

        com_ptr<IDWriteFontCollection> system;
        if (SUCCEEDED(write->GetSystemFontCollection(system.put())))
            emojiMetrics = readMetrics(system.get(), L"Segoe UI Emoji");
    }

    return kind == FontKind::Emoji ? emojiMetrics : sansMetrics;
}

com_ptr<IDWriteTextLayout4> Resources::layout(std::wstring_view text, float size, DWRITE_FONT_WEIGHT weight,
                                              FontKind kind, float lineHeight, bool tabular) {
    auto textFormat = format(size, weight, kind);
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

    return result;
}

float Resources::measure(std::wstring_view text, float size, DWRITE_FONT_WEIGHT weight, FontKind kind, bool tabular) {
    if (text.empty()) return 0;

    DWRITE_TEXT_METRICS textMetrics{};
    layout(text, size, weight, kind, 0, tabular)->GetMetrics(&textMetrics);
    return textMetrics.widthIncludingTrailingWhitespace;
}

ID2D1PathGeometry* Resources::icon(Icon const& value) {
    if (auto found = icons.find(&value); found != icons.end()) return found->second.get();

    std::vector<com_ptr<ID2D1PathGeometry>> parts;
    for (size_t index = 0; index < value.count; index++) parts.push_back(parseSvgPath(d2d.get(), value.paths[index]));

    com_ptr<ID2D1PathGeometry> combined;
    winrt::check_hresult(d2d->CreatePathGeometry(combined.put()));
    com_ptr<ID2D1GeometrySink> combinedSink;
    winrt::check_hresult(combined->Open(combinedSink.put()));
    for (auto& part : parts) part->Stream(combinedSink.get());
    combinedSink->Close();

    icons[&value] = combined;
    return combined.get();
}

ID2D1StrokeStyle1* Resources::roundStroke() {
    auto& stroke = strokes[L"round"];
    if (!stroke) {
        auto properties = D2D1::StrokeStyleProperties1(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND,
                                                       D2D1_LINE_JOIN_ROUND, 10.0f, D2D1_DASH_STYLE_SOLID, 0.0f,
                                                       D2D1_STROKE_TRANSFORM_TYPE_NORMAL);
        winrt::check_hresult(d2d->CreateStrokeStyle(properties, nullptr, 0, stroke.put()));
    }
    return stroke.get();
}

ID2D1StrokeStyle1* Resources::dashStroke(float dash, float gap, bool round) {
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

com_ptr<ID2D1PathGeometry> parseSvgPath(ID2D1Factory* factory, char const* data) {
    com_ptr<ID2D1PathGeometry> geometry;
    winrt::check_hresult(factory->CreatePathGeometry(geometry.put()));

    com_ptr<ID2D1GeometrySink> sink;
    winrt::check_hresult(geometry->Open(sink.put()));
    sink->SetFillMode(D2D1_FILL_MODE_WINDING);

    PathReader reader(data);
    D2D1_POINT_2F current{};
    D2D1_POINT_2F start{};
    D2D1_POINT_2F control{};
    char previous = 0;
    char command = 0;
    bool isOpen = false;

    auto begin = [&](D2D1_POINT_2F point) {
        if (isOpen) sink->EndFigure(D2D1_FIGURE_END_OPEN);
        sink->BeginFigure(point, D2D1_FIGURE_BEGIN_FILLED);
        isOpen = true;
        start = point;
    };

    auto ensureOpen = [&]() {
        if (!isOpen) begin(current);
    };

    while (!reader.done()) {
        if (!reader.hasNumber()) command = reader.command();

        bool relative = command >= 'a' && command <= 'z';
        char upper = static_cast<char>(relative ? command - 32 : command);
        auto point = [&](float x, float y) {
            return relative ? D2D1::Point2F(current.x + x, current.y + y) : D2D1::Point2F(x, y);
        };

        switch (upper) {
        case 'M': {
            float x = reader.number();
            float y = reader.number();
            current = point(x, y);
            begin(current);
            command = relative ? 'l' : 'L';
            break;
        }
        case 'L': {
            float x = reader.number();
            float y = reader.number();
            ensureOpen();
            current = point(x, y);
            sink->AddLine(current);
            break;
        }
        case 'H': {
            float x = reader.number();
            ensureOpen();
            current.x = relative ? current.x + x : x;
            sink->AddLine(current);
            break;
        }
        case 'V': {
            float y = reader.number();
            ensureOpen();
            current.y = relative ? current.y + y : y;
            sink->AddLine(current);
            break;
        }
        case 'C': {
            float values[6];
            for (auto& value : values) value = reader.number();
            ensureOpen();
            auto first = point(values[0], values[1]);
            control = point(values[2], values[3]);
            auto end = point(values[4], values[5]);
            sink->AddBezier(D2D1::BezierSegment(first, control, end));
            current = end;
            break;
        }
        case 'S': {
            float values[4];
            for (auto& value : values) value = reader.number();
            ensureOpen();
            bool smooth = previous == 'C' || previous == 'S';
            auto first = smooth ? D2D1::Point2F(current.x * 2 - control.x, current.y * 2 - control.y) : current;
            control = point(values[0], values[1]);
            auto end = point(values[2], values[3]);
            sink->AddBezier(D2D1::BezierSegment(first, control, end));
            current = end;
            break;
        }
        case 'Q': {
            float values[4];
            for (auto& value : values) value = reader.number();
            ensureOpen();
            control = point(values[0], values[1]);
            auto end = point(values[2], values[3]);
            sink->AddQuadraticBezier(D2D1::QuadraticBezierSegment(control, end));
            current = end;
            break;
        }
        case 'T': {
            float values[2];
            for (auto& value : values) value = reader.number();
            ensureOpen();
            bool smooth = previous == 'Q' || previous == 'T';
            control = smooth ? D2D1::Point2F(current.x * 2 - control.x, current.y * 2 - control.y) : current;
            auto end = point(values[0], values[1]);
            sink->AddQuadraticBezier(D2D1::QuadraticBezierSegment(control, end));
            current = end;
            break;
        }
        case 'A': {
            float rx = reader.number();
            float ry = reader.number();
            float rotation = reader.number();
            bool large = reader.flag();
            bool sweep = reader.flag();
            float x = reader.number();
            float y = reader.number();
            ensureOpen();
            auto end = point(x, y);
            if (rx == 0 || ry == 0) {
                sink->AddLine(end);
            } else {
                sink->AddArc(D2D1::ArcSegment(end, D2D1::SizeF(std::abs(rx), std::abs(ry)), rotation,
                                              sweep ? D2D1_SWEEP_DIRECTION_CLOCKWISE
                                                    : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
                                              large ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL));
            }
            current = end;
            break;
        }
        case 'Z':
            if (isOpen) sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            isOpen = false;
            current = start;
            break;
        default:
            reader.number();
            break;
        }

        previous = upper;
    }

    if (isOpen) sink->EndFigure(D2D1_FIGURE_END_OPEN);
    winrt::check_hresult(sink->Close());
    return geometry;
}

D2D1_COLOR_F color(ThemeColor const& value, float opacity) {
    return D2D1::ColorF(value.r, value.g, value.b, value.a * opacity);
}

D2D1_COLOR_F withAlpha(D2D1_COLOR_F value, float opacity) {
    value.a *= opacity;
    return value;
}

D2D1_COLOR_F rgba(int r, int g, int b, float a) {
    return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f, a);
}

D2D1_COLOR_F parseHex(std::string_view hex) {
    if (!hex.empty() && hex.front() == '#') hex.remove_prefix(1);
    if (hex.size() < 6) return rgba(255, 255, 255);

    auto value = std::strtoul(std::string(hex.substr(0, 6)).c_str(), nullptr, 16);
    return rgba((value >> 16) & 255, (value >> 8) & 255, value & 255);
}

std::string toHex(D2D1_COLOR_F const& value) {
    char buffer[8];
    auto channel = [](float component) { return static_cast<int>(std::lround(std::clamp(component, 0.0f, 1.0f) * 255)); };
    snprintf(buffer, sizeof(buffer), "#%02x%02x%02x", channel(value.r), channel(value.g), channel(value.b));
    return buffer;
}

D2D1_COLOR_F mix(D2D1_COLOR_F a, D2D1_COLOR_F b, float t) {
    return D2D1::ColorF(a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t);
}

double easeOutCubic(double t) {
    t = std::clamp(t, 0.0, 1.0);
    return 1 - std::pow(1 - t, 3);
}

double easeOut(double t) {
    t = std::clamp(t, 0.0, 1.0);
    return 1 - (1 - t) * (1 - t);
}

double nowMs() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

Painter::Painter(ID2D1DeviceContext* context, Resources& resources) : target(context), res(resources) {}

ID2D1SolidColorBrush* Painter::brush(D2D1_COLOR_F const& value) {
    if (!solid) winrt::check_hresult(target->CreateSolidColorBrush(value, solid.put()));
    solid->SetColor(value);
    return solid.get();
}

void Painter::fillRect(D2D1_RECT_F const& rect, D2D1_COLOR_F const& value) {
    target->FillRectangle(rect, brush(value));
}

void Painter::fillRounded(D2D1_RECT_F const& rect, float radius, D2D1_COLOR_F const& value) {
    float limit = std::min(rect.right - rect.left, rect.bottom - rect.top) / 2;
    radius = std::max(0.0f, std::min(radius, limit));
    target->FillRoundedRectangle(D2D1::RoundedRect(rect, radius, radius), brush(value));
}

void Painter::strokeRounded(D2D1_RECT_F const& rect, float radius, D2D1_COLOR_F const& value, float width,
                            ID2D1StrokeStyle* style) {
    float limit = std::min(rect.right - rect.left, rect.bottom - rect.top) / 2;
    radius = std::max(0.0f, std::min(radius, limit));
    target->DrawRoundedRectangle(D2D1::RoundedRect(rect, radius, radius), brush(value), width, style);
}

void Painter::fillEllipse(float cx, float cy, float rx, float ry, D2D1_COLOR_F const& value) {
    target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), rx, ry), brush(value));
}

void Painter::strokeEllipse(float cx, float cy, float rx, float ry, D2D1_COLOR_F const& value, float width) {
    target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), rx, ry), brush(value), width);
}

void Painter::icon(Icon const& value, float x, float y, float size, D2D1_COLOR_F const& tint, bool fill,
                   float strokeWidth) {
    auto geometry = res.icon(value);
    float scale = size / 24.0f;
    Transform transform(target, D2D1::Matrix3x2F::Scale(scale, scale) * D2D1::Matrix3x2F::Translation(x, y));

    if (fill) target->FillGeometry(geometry, brush(tint));
    target->DrawGeometry(geometry, brush(tint), strokeWidth, res.roundStroke());
}

void Painter::text(IDWriteTextLayout* layout, float x, float y, D2D1_COLOR_F const& value) {
    target->DrawTextLayout(D2D1::Point2F(x, y), layout, brush(value), D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
}

void Painter::textAt(std::wstring_view value, float x, float baseline, float size, DWRITE_FONT_WEIGHT weight,
                     D2D1_COLOR_F const& tint, FontKind kind) {
    if (value.empty()) return;

    auto layout = res.layout(value, size, weight, kind);
    DWRITE_LINE_METRICS line{};
    UINT32 count = 0;
    layout->GetLineMetrics(&line, 1, &count);
    text(layout.get(), x, baseline - line.baseline, tint);
}

float Painter::middleBaseline(float centerY, float size, FontKind kind) {
    auto metrics = res.metrics(kind);
    return centerY + (metrics.ascent - metrics.descent) / 2 * size;
}

void Painter::boxShadow(D2D1_RECT_F const& rect, float radius, float offsetY, float blur, float spread, float alpha) {
    D2D1_MATRIX_3X2_F matrix{};
    target->GetTransform(&matrix);
    float scale = std::sqrt(std::abs(matrix._11 * matrix._22 - matrix._12 * matrix._21));

    com_ptr<ID2D1CommandList> list;
    if (FAILED(target->CreateCommandList(list.put()))) return;

    com_ptr<ID2D1Image> previous;
    target->GetTarget(previous.put());
    target->SetTarget(list.get());

    auto inflated = D2D1::RectF(rect.left - spread, rect.top - spread, rect.right + spread, rect.bottom + spread);
    fillRounded(inflated, std::max(0.0f, radius + spread), D2D1::ColorF(0, 0, 0, 1));

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

void Painter::ring(D2D1_RECT_F const& rect, float radius, float width, D2D1_COLOR_F const& value) {
    float half = width / 2;
    strokeRounded(D2D1::RectF(rect.left - half, rect.top - half, rect.right + half, rect.bottom + half), radius + half,
                  value, width);
}

void Painter::pushOpacity(float opacity) {
    target->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                                            D2D1::IdentityMatrix(), opacity),
                      nullptr);
}

void Painter::popOpacity() {
    target->PopLayer();
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

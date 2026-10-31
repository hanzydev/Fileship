#include "gfx.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <numbers>

namespace fileship::ui {

namespace {

std::weak_ptr<Resources> sharedResources;
std::filesystem::path customResources;

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

Matrix Matrix::scale(float x, float y, PointF center) {
    return {x, 0, 0, y, center.x - x * center.x, center.y - y * center.y};
}

Matrix Matrix::translation(float x, float y) {
    return {1, 0, 0, 1, x, y};
}

Matrix Matrix::rotation(float degrees, PointF center) {
    float radians = degrees * std::numbers::pi_v<float> / 180;
    float cosine = std::cos(radians);
    float sine = std::sin(radians);
    return {cosine,
            sine,
            -sine,
            cosine,
            center.x - center.x * cosine + center.y * sine,
            center.y - center.x * sine - center.y * cosine};
}

Matrix Matrix::operator*(Matrix const& next) const {
    return {m11 * next.m11 + m12 * next.m21,
            m11 * next.m12 + m12 * next.m22,
            m21 * next.m11 + m22 * next.m21,
            m21 * next.m12 + m22 * next.m22,
            dx * next.m11 + dy * next.m21 + next.dx,
            dx * next.m12 + dy * next.m22 + next.dy};
}

PointF Matrix::apply(PointF point) const {
    return {point.x * m11 + point.y * m21 + dx, point.x * m12 + point.y * m22 + dy};
}

void Path::moveTo(PointF point) {
    segments.push_back({Verb::Move, {point, {}, {}}});
}

void Path::lineTo(PointF point) {
    segments.push_back({Verb::Line, {point, {}, {}}});
}

void Path::quadTo(PointF control, PointF point) {
    segments.push_back({Verb::Quad, {control, point, {}}});
}

void Path::cubicTo(PointF first, PointF second, PointF point) {
    segments.push_back({Verb::Cubic, {first, second, point}});
}

void Path::arcTo(PointF point, float rx, float ry, float rotation, bool large, bool sweep) {
    segments.push_back({Verb::Arc, {point, {}, {}}, rx, ry, rotation, large, sweep});
}

void Path::close() {
    segments.push_back({Verb::Close, {}});
}

void Path::addRect(RectF const& rect) {
    moveTo({rect.left, rect.top});
    lineTo({rect.right, rect.top});
    lineTo({rect.right, rect.bottom});
    lineTo({rect.left, rect.bottom});
    close();
}

void Path::addRoundedRect(RectF const& rect, float radius) {
    if (radius <= 0) return addRect(rect);

    moveTo({rect.left + radius, rect.top});
    lineTo({rect.right - radius, rect.top});
    arcTo({rect.right, rect.top + radius}, radius, radius, 0, false, true);
    lineTo({rect.right, rect.bottom - radius});
    arcTo({rect.right - radius, rect.bottom}, radius, radius, 0, false, true);
    lineTo({rect.left + radius, rect.bottom});
    arcTo({rect.left, rect.bottom - radius}, radius, radius, 0, false, true);
    lineTo({rect.left, rect.top + radius});
    arcTo({rect.left + radius, rect.top}, radius, radius, 0, false, true);
    close();
}

void Path::addEllipse(float cx, float cy, float rx, float ry) {
    moveTo({cx + rx, cy});
    arcTo({cx - rx, cy}, rx, ry, 0, false, true);
    arcTo({cx + rx, cy}, rx, ry, 0, false, true);
    close();
}

StrokeStyle StrokeStyle::round() {
    return {CapStyle::Round, true, 0, 0};
}

StrokeStyle StrokeStyle::dashed(float dash, float gap, bool round) {
    return {round ? CapStyle::Round : CapStyle::Flat, false, dash, gap};
}

void Painter::icon(Icon const& value, float x, float y, float size, Color const& tint, bool fill, float strokeWidth) {
    auto const& path = res.icon(value);
    float scale = size / 24.0f;
    Transform transform(*this, Matrix::scale(scale, scale) * Matrix::translation(x, y));

    if (fill) fillPath(path, tint);
    strokePath(path, tint, strokeWidth, StrokeStyle::round());
}

void Painter::textAt(std::wstring_view value, float x, float baseline, float size, FontWeight weight,
                     Color const& tint, FontKind kind) {
    if (value.empty()) return;

    auto layout = res.layout(value, size, weight, kind);
    text(*layout, x, baseline - layout->baseline(), tint);
}

void Painter::ring(RectF const& rect, float radius, float width, Color const& value) {
    float half = width / 2;
    strokeRounded(RectF(rect.left - half, rect.top - half, rect.right + half, rect.bottom + half), radius + half,
                  value, width);
}

float Painter::middleBaseline(float centerY, float size, FontKind kind) {
    auto metrics = res.metrics(kind);
    return centerY + (metrics.ascent - metrics.descent) / 2 * size;
}

std::shared_ptr<Resources> Resources::acquire() {
    if (auto existing = sharedResources.lock()) return existing;

    auto resources = std::make_shared<Resources>();
    sharedResources = resources;
    return resources;
}

float Resources::measure(std::wstring_view text, float size, FontWeight weight, FontKind kind, bool tabular) {
    if (text.empty()) return 0;
    return layout(text, size, weight, kind, 0, tabular)->metrics().widthIncludingTrailingWhitespace;
}

Path const& Resources::icon(Icon const& value) {
    if (auto found = icons.find(&value); found != icons.end()) return found->second;

    Path combined;
    for (size_t index = 0; index < value.count; index++) {
        auto part = parseSvgPath(value.paths[index]);
        combined.segments.insert(combined.segments.end(), part.segments.begin(), part.segments.end());
    }

    return icons[&value] = std::move(combined);
}

void setResourcesPath(std::filesystem::path const& path) {
    customResources = path;
}

std::filesystem::path resourcesPath() {
    return customResources.empty() ? executableDirectory() : customResources;
}

Path parseSvgPath(char const* data) {
    Path path;
    PathReader reader(data);
    PointF current{};
    PointF start{};
    PointF control{};
    char previous = 0;
    char command = 0;
    bool isOpen = false;

    auto begin = [&](PointF point) {
        path.moveTo(point);
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
            return relative ? PointF(current.x + x, current.y + y) : PointF(x, y);
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
            path.lineTo(current);
            break;
        }
        case 'H': {
            float x = reader.number();
            ensureOpen();
            current.x = relative ? current.x + x : x;
            path.lineTo(current);
            break;
        }
        case 'V': {
            float y = reader.number();
            ensureOpen();
            current.y = relative ? current.y + y : y;
            path.lineTo(current);
            break;
        }
        case 'C': {
            float values[6];
            for (auto& value : values) value = reader.number();
            ensureOpen();
            auto first = point(values[0], values[1]);
            control = point(values[2], values[3]);
            auto end = point(values[4], values[5]);
            path.cubicTo(first, control, end);
            current = end;
            break;
        }
        case 'S': {
            float values[4];
            for (auto& value : values) value = reader.number();
            ensureOpen();
            bool smooth = previous == 'C' || previous == 'S';
            auto first = smooth ? PointF(current.x * 2 - control.x, current.y * 2 - control.y) : current;
            control = point(values[0], values[1]);
            auto end = point(values[2], values[3]);
            path.cubicTo(first, control, end);
            current = end;
            break;
        }
        case 'Q': {
            float values[4];
            for (auto& value : values) value = reader.number();
            ensureOpen();
            control = point(values[0], values[1]);
            auto end = point(values[2], values[3]);
            path.quadTo(control, end);
            current = end;
            break;
        }
        case 'T': {
            float values[2];
            for (auto& value : values) value = reader.number();
            ensureOpen();
            bool smooth = previous == 'Q' || previous == 'T';
            control = smooth ? PointF(current.x * 2 - control.x, current.y * 2 - control.y) : current;
            auto end = point(values[0], values[1]);
            path.quadTo(control, end);
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
                path.lineTo(end);
            } else {
                path.arcTo(end, std::abs(rx), std::abs(ry), rotation, large, sweep);
            }
            current = end;
            break;
        }
        case 'Z':
            if (isOpen) path.close();
            isOpen = false;
            current = start;
            break;
        default:
            reader.number();
            break;
        }

        previous = upper;
    }

    return path;
}

Color color(ThemeColor const& value, float opacity) {
    return Color(value.r, value.g, value.b, value.a * opacity);
}

Color withAlpha(Color value, float opacity) {
    value.a *= opacity;
    return value;
}

Color rgba(int r, int g, int b, float a) {
    return Color(r / 255.0f, g / 255.0f, b / 255.0f, a);
}

Color parseHex(std::string_view hex) {
    if (!hex.empty() && hex.front() == '#') hex.remove_prefix(1);
    if (hex.size() < 6) return rgba(255, 255, 255);

    auto value = std::strtoul(std::string(hex.substr(0, 6)).c_str(), nullptr, 16);
    return rgba((value >> 16) & 255, (value >> 8) & 255, value & 255);
}

std::string toHex(Color const& value) {
    char buffer[8];
    auto channel = [](float component) { return static_cast<int>(std::lround(std::clamp(component, 0.0f, 1.0f) * 255)); };
    std::snprintf(buffer, sizeof(buffer), "#%02x%02x%02x", channel(value.r), channel(value.g), channel(value.b));
    return buffer;
}

Color mix(Color a, Color b, float t) {
    return Color(a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t);
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

}

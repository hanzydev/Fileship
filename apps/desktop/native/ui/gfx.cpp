#include "gfx.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace fileship::ui {

namespace {

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

void setResourcesPath(std::filesystem::path const& path) {
    customResources = path;
}

std::filesystem::path resourcesPath() {
    return customResources.empty() ? executableDirectory() : customResources;
}

Matrix Matrix::scale(float x, float y, PointF center) {
    return Matrix{x, 0, 0, y, center.x - x * center.x, center.y - y * center.y};
}

Matrix Matrix::translation(float x, float y) {
    return Matrix{1, 0, 0, 1, x, y};
}

Matrix Matrix::operator*(Matrix const& next) const {
    return Matrix{m11 * next.m11 + m12 * next.m21,
                  m11 * next.m12 + m12 * next.m22,
                  m21 * next.m11 + m22 * next.m21,
                  m21 * next.m12 + m22 * next.m22,
                  dx * next.m11 + dy * next.m21 + next.dx,
                  dx * next.m12 + dy * next.m22 + next.dy};
}

float Resources::measure(std::wstring_view text, float size, FontWeight weight, FontKind kind, bool tabular) {
    if (text.empty()) return 0;
    return layout(text, size, weight, kind, 0, tabular)->metrics().width;
}

void parseSvgPath(PathSink& sink, char const* data) {
    PathReader reader(data);
    PointF current{};
    PointF start{};
    PointF control{};
    char previous = 0;
    char command = 0;
    bool isOpen = false;

    auto begin = [&](PointF point) {
        if (isOpen) sink.end(false);
        sink.begin(point, true);
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
            return relative ? PointF{current.x + x, current.y + y} : PointF{x, y};
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
            sink.lineTo(current);
            break;
        }
        case 'H': {
            float x = reader.number();
            ensureOpen();
            current.x = relative ? current.x + x : x;
            sink.lineTo(current);
            break;
        }
        case 'V': {
            float y = reader.number();
            ensureOpen();
            current.y = relative ? current.y + y : y;
            sink.lineTo(current);
            break;
        }
        case 'C': {
            float values[6];
            for (auto& value : values) value = reader.number();
            ensureOpen();
            auto first = point(values[0], values[1]);
            control = point(values[2], values[3]);
            auto end = point(values[4], values[5]);
            sink.cubicTo(first, control, end);
            current = end;
            break;
        }
        case 'S': {
            float values[4];
            for (auto& value : values) value = reader.number();
            ensureOpen();
            bool smooth = previous == 'C' || previous == 'S';
            auto first = smooth ? PointF{current.x * 2 - control.x, current.y * 2 - control.y} : current;
            control = point(values[0], values[1]);
            auto end = point(values[2], values[3]);
            sink.cubicTo(first, control, end);
            current = end;
            break;
        }
        case 'Q': {
            float values[4];
            for (auto& value : values) value = reader.number();
            ensureOpen();
            control = point(values[0], values[1]);
            auto end = point(values[2], values[3]);
            sink.quadTo(control, end);
            current = end;
            break;
        }
        case 'T': {
            float values[2];
            for (auto& value : values) value = reader.number();
            ensureOpen();
            bool smooth = previous == 'Q' || previous == 'T';
            control = smooth ? PointF{current.x * 2 - control.x, current.y * 2 - control.y} : current;
            auto end = point(values[0], values[1]);
            sink.quadTo(control, end);
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
                sink.lineTo(end);
            } else {
                sink.arcTo(end, SizeF{std::abs(rx), std::abs(ry)}, rotation, sweep, large);
            }
            current = end;
            break;
        }
        case 'Z':
            if (isOpen) sink.end(true);
            isOpen = false;
            current = start;
            break;
        default:
            reader.number();
            break;
        }

        previous = upper;
    }

    if (isOpen) sink.end(false);
}

Color color(ThemeColor const& value, float opacity) {
    return Color{value.r, value.g, value.b, value.a * opacity};
}

Color withAlpha(Color value, float opacity) {
    value.a *= opacity;
    return value;
}

Color rgba(int r, int g, int b, float a) {
    return Color{r / 255.0f, g / 255.0f, b / 255.0f, a};
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
    snprintf(buffer, sizeof(buffer), "#%02x%02x%02x", channel(value.r), channel(value.g), channel(value.b));
    return buffer;
}

Color mix(Color a, Color b, float t) {
    return Color{a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
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

void Painter::textAt(std::wstring_view value, float x, float baseline, float size, FontWeight weight,
                     Color const& tint, FontKind kind) {
    if (value.empty()) return;

    auto layout = resources().layout(value, size, weight, kind);
    text(*layout, x, baseline - layout->baseline(), tint);
}

float Painter::middleBaseline(float centerY, float size, FontKind kind) {
    auto metrics = resources().metrics(kind);
    return centerY + (metrics.ascent - metrics.descent) / 2 * size;
}

void Painter::ring(RectF const& rect, float radius, float width, Color const& value) {
    float half = width / 2;
    strokeRounded(RectF{rect.left - half, rect.top - half, rect.right + half, rect.bottom + half}, radius + half, value,
                  width);
}

}

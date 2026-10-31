#include "widgets.h"

#include <algorithm>
#include <cmath>

namespace fileship::ui {

double cubicBezier(double x1, double y1, double x2, double y2, double t) {
    if (t <= 0) return 0;
    if (t >= 1) return 1;

    auto sample = [](double a1, double a2, double s) {
        double inverse = 1 - s;
        return 3 * inverse * inverse * s * a1 + 3 * inverse * s * s * a2 + s * s * s;
    };
    auto slope = [](double a1, double a2, double s) {
        double inverse = 1 - s;
        return 3 * inverse * inverse * a1 + 6 * inverse * s * (a2 - a1) + 3 * s * s * (1 - a2);
    };

    double s = t;
    for (int iteration = 0; iteration < 8; iteration++) {
        double error = sample(x1, x2, s) - t;
        if (std::abs(error) < 1e-6) break;
        double derivative = slope(x1, x2, s);
        if (std::abs(derivative) < 1e-6) break;
        s -= error / derivative;
    }

    double low = 0;
    double high = 1;
    s = std::clamp(s, 0.0, 1.0);
    for (int iteration = 0; iteration < 20 && std::abs(sample(x1, x2, s) - t) > 1e-6; iteration++) {
        if (sample(x1, x2, s) < t) {
            low = s;
        } else {
            high = s;
        }
        s = (low + high) / 2;
    }

    return sample(y1, y2, s);
}

double ease(Easing easing, double t) {
    switch (easing) {
    case Easing::Ease:
        return cubicBezier(0.25, 0.1, 0.25, 1, t);
    case Easing::Standard:
        return cubicBezier(0.4, 0, 0.2, 1, t);
    default:
        return std::clamp(t, 0.0, 1.0);
    }
}

double Animation::value(double now) const {
    if (duration <= 0) return to;
    double progress = (now - start) / duration;
    if (progress >= 1) return to;
    return from + (to - from) * ease(easing, progress);
}

void Animation::animate(double target, double now, double length, Easing curve) {
    if (target == to) return;
    from = value(now);
    to = target;
    start = now;
    duration = length;
    easing = curve;
}

void Animation::jump(double target) {
    from = to = target;
    duration = 0;
}

bool Animation::isRunning(double now) const {
    return duration > 0 && now - start < duration;
}

Color buttonBackground(Palette const& palette, ButtonStyle const& style) {
    auto const& theme = palette.theme;
    double hover = style.isDisabled ? 0 : style.hover;

    switch (style.variant) {
    case ButtonVariant::Secondary:
        return mix(color(theme.secondary), color(theme.secondary, 0.8f), static_cast<float>(hover));
    case ButtonVariant::Destructive: {
        float base = palette.dark ? 0.2f : 0.1f;
        float hovered = palette.dark ? 0.3f : 0.2f;
        return color(theme.destructive, base + (hovered - base) * static_cast<float>(hover));
    }
    default: {
        if (style.isExpanded) return color(theme.muted);
        auto target = color(theme.muted, palette.dark ? 0.5f : 1.0f);
        return withAlpha(target, static_cast<float>(hover));
    }
    }
}

Color buttonForeground(Palette const& palette, ButtonStyle const& style) {
    switch (style.variant) {
    case ButtonVariant::Secondary:
        return color(palette.theme.secondaryForeground);
    case ButtonVariant::Destructive:
        return color(palette.theme.destructive);
    default:
        return color(palette.theme.popoverForeground);
    }
}

float buttonOffset(ButtonStyle const& style) {
    return style.isPressed && !style.hasPopup && !style.isDisabled ? 1.0f : 0.0f;
}

void drawButton(Painter& painter, Palette const& palette, RectF const& rect, ButtonStyle const& style) {
    auto background = buttonBackground(palette, style);
    if (background.a <= 0) return;

    float offset = buttonOffset(style);
    auto moved = RectF(rect.left, rect.top + offset, rect.right, rect.bottom + offset);
    painter.fillRounded(moved, 32, background);
}

TooltipLayout measureTooltip(Resources& resources, std::wstring const& text, std::wstring const& kbd) {
    float textWidth = resources.measure(text, 12, FontWeight::Normal);
    if (kbd.empty()) return {12 + textWidth + 12, 28};

    float kbdWidth = std::max(22.0f, resources.measure(kbd, 12, FontWeight::Medium) + 12);
    return {12 + textWidth + 10 + kbdWidth + 6, 34};
}

void drawKbd(Painter& painter, RectF const& rect, std::wstring const& text, Color const& background,
             Color const& foreground) {
    painter.fillRounded(rect, 10, background);

    float width = painter.resources().measure(text, 12, FontWeight::Medium);
    float x = (rect.left + rect.right) / 2 - width / 2;
    float baseline = painter.middleBaseline((rect.top + rect.bottom) / 2, 12);
    painter.textAt(text, x, baseline, 12, FontWeight::Medium, foreground);
}

void drawTooltip(Painter& painter, Palette const& palette, RectF const& rect, Side side, float arrowCenter,
                 std::wstring const& text, std::wstring const& kbd) {
    auto const& theme = palette.theme;
    auto background = color(theme.foreground);
    auto foreground = color(theme.background);

    PointF arrow{};
    switch (side) {
    case Side::Bottom:
        arrow = PointF(arrowCenter, rect.top + 2);
        break;
    case Side::Top:
        arrow = PointF(arrowCenter, rect.bottom - 2);
        break;
    case Side::Right:
        arrow = PointF(rect.left + 2, arrowCenter);
        break;
    case Side::Left:
        arrow = PointF(rect.right - 2, arrowCenter);
        break;
    }

    {
        Transform rotate(painter,
                         Matrix::rotation(45) * Matrix::translation(arrow.x, arrow.y));
        painter.fillRounded(RectF(-5, -5, 5, 5), 2, background);
    }

    painter.fillRounded(rect, 14, background);

    float centerY = (rect.top + rect.bottom) / 2;
    painter.textAt(text, rect.left + 12, painter.middleBaseline(centerY, 12), 12, FontWeight::Normal,
                   foreground);

    if (!kbd.empty()) {
        float kbdWidth = std::max(22.0f, painter.resources().measure(kbd, 12, FontWeight::Medium) + 12);
        auto kbdRect = RectF(rect.right - 6 - kbdWidth, centerY - 11, rect.right - 6, centerY + 11);
        drawKbd(painter, kbdRect, kbd, color(theme.background, palette.dark ? 0.1f : 0.2f), foreground);
    }
}

void drawPopoverFrame(Painter& painter, Palette const& palette, RectF const& rect, bool isLarge) {
    if (isLarge) {
        painter.boxShadow(rect, 24, 10, 15, -3, 0.1f);
        painter.boxShadow(rect, 24, 4, 6, -4, 0.1f);
    } else {
        painter.boxShadow(rect, 24, 4, 6, -1, 0.1f);
        painter.boxShadow(rect, 24, 2, 4, -2, 0.1f);
    }

    painter.fillRounded(rect, 24, color(palette.theme.popover));
    painter.ring(rect, 24, 1, color(palette.theme.foreground, palette.dark ? 0.1f : 0.05f));
}

void drawSlider(Painter& painter, Palette const& palette, RectF const& rect, double fraction, double hover) {
    auto const& theme = palette.theme;
    float width = rect.right - rect.left;
    float centerY = (rect.top + rect.bottom) / 2;
    auto track = RectF(rect.left, centerY - 4, rect.right, centerY + 4);

    painter.fillRounded(track, 4, color(theme.input, 0.9f));

    painter.pushClip(track, 4);
    painter.fillRect(RectF(rect.left, track.top, rect.left + static_cast<float>(fraction) * width, track.bottom),
                     color(theme.primary));
    painter.popClip();

    float center = rect.left + 12 + static_cast<float>(fraction) * (width - 24);
    auto thumb = RectF(center - 12, centerY - 8, center + 12, centerY + 8);
    painter.boxShadow(thumb, 8, 4, 6, -1, 0.1f);
    painter.boxShadow(thumb, 8, 2, 4, -2, 0.1f);

    float ringWidth = 1 + 3 * static_cast<float>(hover);
    auto ringColor = mix(rgba(0, 0, 0, 0.1f), color(theme.ring, 0.3f), static_cast<float>(hover));
    painter.ring(thumb, 8, ringWidth, ringColor);
    painter.fillRounded(thumb, 8, rgba(255, 255, 255));
}

void drawSwitch(Painter& painter, Palette const& palette, RectF const& rect, double checked) {
    auto const& theme = palette.theme;
    float value = static_cast<float>(checked);
    auto background = mix(color(theme.input, 0.9f), color(theme.primary), value);

    painter.fillRounded(rect, 10, background);

    auto thumbColor = palette.dark ? mix(color(theme.foreground), color(theme.primaryForeground), value)
                                   : color(theme.background);
    float left = rect.left + 2 + 16 * value;
    auto thumb = RectF(left, rect.top + 2, left + 24, rect.top + 18);
    painter.boxShadow(thumb, 8, 1, 3, 0, 0.1f);
    painter.boxShadow(thumb, 8, 1, 2, -1, 0.1f);
    painter.fillRounded(thumb, 8, thumbColor);
}

void drawSpinner(Painter& painter, float x, float y, float size, Color const& tint, double now) {
    float angle = static_cast<float>(std::fmod(now, 1000.0) / 1000.0 * 360.0);
    float half = size / 2;
    Transform rotate(painter, Matrix::rotation(angle, PointF(x + half, y + half)));
    painter.icon(icons::loaderCircle, x, y, size, tint);
}

Placement placeFloating(RectF const& anchor, float width, float height, Side side, float offset,
                        RectF const& bounds) {
    auto fits = [&](Side candidate) {
        switch (candidate) {
        case Side::Bottom:
            return anchor.bottom + offset + height <= bounds.bottom;
        case Side::Top:
            return anchor.top - offset - height >= bounds.top;
        case Side::Right:
            return anchor.right + offset + width <= bounds.right;
        default:
            return anchor.left - offset - width >= bounds.left;
        }
    };

    auto opposite = [](Side candidate) {
        switch (candidate) {
        case Side::Bottom:
            return Side::Top;
        case Side::Top:
            return Side::Bottom;
        case Side::Right:
            return Side::Left;
        default:
            return Side::Right;
        }
    };

    if (!fits(side) && fits(opposite(side))) side = opposite(side);

    float centerX = (anchor.left + anchor.right) / 2;
    float centerY = (anchor.top + anchor.bottom) / 2;
    float x = 0;
    float y = 0;

    switch (side) {
    case Side::Bottom:
        x = centerX - width / 2;
        y = anchor.bottom + offset;
        break;
    case Side::Top:
        x = centerX - width / 2;
        y = anchor.top - offset - height;
        break;
    case Side::Right:
        x = anchor.right + offset;
        y = centerY - height / 2;
        break;
    case Side::Left:
        x = anchor.left - offset - width;
        y = centerY - height / 2;
        break;
    }

    if (side == Side::Top || side == Side::Bottom) {
        x = std::clamp(x, bounds.left, std::max(bounds.left, bounds.right - width));
    } else {
        y = std::clamp(y, bounds.top, std::max(bounds.top, bounds.bottom - height));
    }

    Placement placement;
    placement.rect = RectF(x, y, x + width, y + height);
    placement.side = side;

    switch (side) {
    case Side::Bottom:
        placement.origin = PointF(std::clamp(centerX, x, x + width), y);
        break;
    case Side::Top:
        placement.origin = PointF(std::clamp(centerX, x, x + width), y + height);
        break;
    case Side::Right:
        placement.origin = PointF(x, std::clamp(centerY, y, y + height));
        break;
    case Side::Left:
        placement.origin = PointF(x + width, std::clamp(centerY, y, y + height));
        break;
    }

    return placement;
}

Reveal revealAt(double progress, bool isOpening, Side side, bool zoom, bool slide) {
    double eased = ease(Easing::Ease, progress);
    Reveal reveal;

    if (isOpening) {
        reveal.opacity = eased;
        reveal.scale = zoom ? 0.95 + 0.05 * eased : 1;

        if (slide) {
            float distance = static_cast<float>(8 * (1 - eased));
            switch (side) {
            case Side::Bottom:
                reveal.shift = PointF(0, -distance);
                break;
            case Side::Top:
                reveal.shift = PointF(0, distance);
                break;
            case Side::Right:
                reveal.shift = PointF(-distance, 0);
                break;
            case Side::Left:
                reveal.shift = PointF(distance, 0);
                break;
            }
        }
    } else {
        reveal.opacity = 1 - eased;
        reveal.scale = zoom ? 1 - 0.05 * eased : 1;
    }

    return reveal;
}

}

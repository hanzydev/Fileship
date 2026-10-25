#pragma once

#include "gfx.h"

#include <string>

namespace fileship::ui {

enum class Easing { Ease, Standard, Linear };

double cubicBezier(double x1, double y1, double x2, double y2, double t);
double ease(Easing easing, double t);

class Animation {
public:
    Animation(double initial = 0) : from(initial), to(initial) {}

    double value(double now) const;
    void animate(double target, double now, double duration, Easing easing = Easing::Standard);
    void jump(double value);
    bool isRunning(double now) const;

    double target() const {
        return to;
    }

private:
    double from = 0;
    double to = 0;
    double start = 0;
    double duration = 0;
    Easing easing = Easing::Standard;
};

struct Palette {
    Theme theme;
    bool dark = false;
};

enum class ButtonVariant { Ghost, Secondary, Destructive };

enum class Side { Top, Right, Bottom, Left };

constexpr float BUTTON_SIZE = 32;
constexpr float BUTTON_DURATION = 150;
constexpr float POPOVER_DURATION = 100;
constexpr float TOOLTIP_DURATION = 150;

struct ButtonStyle {
    ButtonVariant variant = ButtonVariant::Ghost;
    double hover = 0;
    bool isPressed = false;
    bool isDisabled = false;
    bool isExpanded = false;
    bool hasPopup = false;
};

D2D1_COLOR_F buttonBackground(Palette const& palette, ButtonStyle const& style);
D2D1_COLOR_F buttonForeground(Palette const& palette, ButtonStyle const& style);
void drawButton(Painter& painter, Palette const& palette, D2D1_RECT_F const& rect, ButtonStyle const& style);
float buttonOffset(ButtonStyle const& style);

struct TooltipLayout {
    float width = 0;
    float height = 0;
};

TooltipLayout measureTooltip(Resources& resources, std::wstring const& text, std::wstring const& kbd);
void drawTooltip(Painter& painter, Palette const& palette, D2D1_RECT_F const& rect, Side side, float arrowCenter,
                 std::wstring const& text, std::wstring const& kbd);

void drawPopoverFrame(Painter& painter, Palette const& palette, D2D1_RECT_F const& rect, bool isLarge);
void drawKbd(Painter& painter, D2D1_RECT_F const& rect, std::wstring const& text, D2D1_COLOR_F const& background,
             D2D1_COLOR_F const& foreground);
void drawSlider(Painter& painter, Palette const& palette, D2D1_RECT_F const& rect, double fraction, double hover);
void drawSwitch(Painter& painter, Palette const& palette, D2D1_RECT_F const& rect, double checked);
void drawSpinner(Painter& painter, float x, float y, float size, D2D1_COLOR_F const& tint, double now);

struct Placement {
    D2D1_RECT_F rect{};
    Side side = Side::Bottom;
    D2D1_POINT_2F origin{};
};

Placement placeFloating(D2D1_RECT_F const& anchor, float width, float height, Side side, float offset,
                        D2D1_RECT_F const& bounds);

struct Reveal {
    double opacity = 1;
    double scale = 1;
    D2D1_POINT_2F shift{};
};

Reveal revealAt(double progress, bool isOpening, Side side, bool zoom, bool slide);

}

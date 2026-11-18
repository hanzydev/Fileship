#pragma once

#include "gfx.h"
#include "recorderUi.h"
#include "widgets.h"

#include <array>
#include <string>
#include <vector>

namespace fileship::ui {

class RecorderToolbar {
public:
    RecorderToolbar(Resources& resources, ThemeMode theme, RecorderUiCallbacks callbacks);

    RecorderViewState state;
    double scale = 1;

    void paint(Painter& painter, double now);
    bool isAnimating(double now);
    bool contains(double x, double y);
    void pointerMove(double x, double y);
    void pointerLeave();
    void pointerDown(double x, double y);
    void pointerUp(double x, double y);
    Color borderColor() const;
    Color surfaceColor() const;
    RectF pill();

private:
    enum class Control { None, Microphone, System, Pause, Discard, Stop };

    struct ControlRect {
        Control control;
        RectF rect;
        bool isDisabled;
    };

    bool isBusy() const;
    bool isMuted(AudioSource source) const;
    std::wstring label() const;
    std::vector<ControlRect> layout(float& pillLeft, float& pillWidth, float& statusWidth);
    Animation& hoverOf(Control control);
    Control controlAt(double x, double y);
    void activate(Control control);
    void setHovered(Control control);

    Resources& resources;
    Palette palette;
    RecorderUiCallbacks callbacks;
    std::array<Animation, 6> hovers{};
    Control hovered = Control::None;
    Control pressed = Control::None;
};

}

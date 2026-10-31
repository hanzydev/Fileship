#pragma once

#include "recorderUi.h"
#include "widgets.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace fileship::ui {

constexpr float RECORDER_TOOLBAR_WIDTH = 440;
constexpr float RECORDER_TOOLBAR_HEIGHT = 64;
constexpr float RECORDER_BORDER_PADDING = 3;
constexpr float RECORDER_BORDER_WIDTH = 2;

class RecorderToolbar {
public:
    RecorderToolbar(ThemeMode theme, RecorderUiCallbacks callbacks);

    Resources& resources() const;
    Palette const& palette() const;
    double scale() const;
    void setScale(double value);
    RecorderViewState const& state() const;
    void setState(RecorderViewState const& value);

    void paint(Painter& painter, double now);
    bool isAnimating(double now) const;
    bool contains(double x, double y);
    void pointerMove(double x, double y);
    void pointerLeave();
    void pointerDown(double x, double y);
    void pointerUp(double x, double y);
    void notifyVisible() const;

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

    RecorderUiCallbacks callbacks;
    std::shared_ptr<Resources> shared;
    Palette colors;
    RecorderViewState current;
    double factor = 1;
    std::array<Animation, 6> hovers{};
    Control hovered = Control::None;
    Control pressed = Control::None;
};

void paintRecorderBorder(Painter& painter, Color const& tint, bool isHorizontal, int width, int height, float offset,
                         float dash);

}

#include "recorderToolbar.h"

#include <shared/i18n.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>

namespace fileship::ui {

namespace {

int64_t epochMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::wstring formatTime(int64_t milliseconds) {
    int64_t total = std::max<int64_t>(0, milliseconds / 1000);
    int64_t hours = total / 3600;
    int64_t minutes = total % 3600 / 60;
    int64_t seconds = total % 60;
    wchar_t buffer[32];

    if (hours) {
        swprintf(buffer, 32, L"%lld:%02lld:%02lld", hours, minutes, seconds);
    } else {
        swprintf(buffer, 32, L"%lld:%02lld", minutes, seconds);
    }
    return buffer;
}

}

RecorderToolbar::RecorderToolbar(Resources& resources, ThemeMode theme, RecorderUiCallbacks value)
    : resources(resources),
      palette{theme == ThemeMode::Dark ? DARK_THEME : LIGHT_THEME, theme == ThemeMode::Dark},
      callbacks(std::move(value)) {}

Color RecorderToolbar::borderColor() const {
    return color(palette.theme.destructive);
}

bool RecorderToolbar::isBusy() const {
    return state.phase == RecorderPhase::Preparing || state.phase == RecorderPhase::Countdown ||
           state.phase == RecorderPhase::Processing;
}

bool RecorderToolbar::isMuted(AudioSource source) const {
    return std::find(state.muted.begin(), state.muted.end(), source) != state.muted.end();
}

std::wstring RecorderToolbar::label() const {
    switch (state.phase) {
    case RecorderPhase::Preparing:
    case RecorderPhase::Idle:
        return t(L"recorder.starting");
    case RecorderPhase::Countdown:
        return t(L"recorder.startingIn", {{L"count", std::to_wstring(state.countdown)}});
    case RecorderPhase::Processing:
        return t(L"recorder.processing");
    default:
        break;
    }

    int64_t elapsed = state.elapsedMs + (state.sinceMs ? epochMs() - state.sinceMs : 0);
    auto time = formatTime(elapsed);
    return state.maxDurationMs ? time + L" / " + formatTime(state.maxDurationMs) : time;
}

std::vector<RecorderToolbar::ControlRect> RecorderToolbar::layout(float& pillLeft, float& pillWidth, float& statusWidth) {
    auto text = label();
    float textWidth = resources.measure(text, 14, FontWeight::Medium, FontKind::Sans, true);
    float indicator = isBusy() || state.phase == RecorderPhase::Paused ? 16.0f : 10.0f;
    statusWidth = std::max(96.0f, 10 + indicator + 8 + textWidth);

    float stopWidth = 12 + 16 + 4 + resources.measure(t(L"recorder.stop"), 14, FontWeight::Medium) + 12;
    std::vector<Control> buttons;
    std::vector<AudioSource> unique;
    for (auto source : state.sources)
        if (std::find(unique.begin(), unique.end(), source) == unique.end()) unique.push_back(source);
    for (auto source : unique) buttons.push_back(source == AudioSource::Microphone ? Control::Microphone : Control::System);
    buttons.push_back(Control::Pause);
    buttons.push_back(Control::Discard);

    float content = statusWidth + 4 + 9 + static_cast<float>(buttons.size()) * (4 + BUTTON_SIZE) + 4 + stopWidth;
    pillWidth = 1 + 6 + content + 6 + 1;
    pillLeft = (RECORDER_WIDTH - pillWidth) / 2;

    std::vector<ControlRect> result;
    float x = pillLeft + 7 + statusWidth + 4 + 9;
    float top = (RECORDER_HEIGHT - BUTTON_SIZE) / 2;

    for (auto control : buttons) {
        x += 4;
        bool isDisabled = control == Control::Pause    ? state.phase != RecorderPhase::Recording &&
                                                             state.phase != RecorderPhase::Paused
                          : control == Control::Discard ? state.phase == RecorderPhase::Processing
                                                        : isBusy();
        result.push_back({control, RectF(x, top, x + BUTTON_SIZE, top + BUTTON_SIZE), isDisabled});
        x += BUTTON_SIZE;
    }

    x += 4;
    result.push_back({Control::Stop, RectF(x, top, x + stopWidth, top + BUTTON_SIZE), isBusy()});
    return result;
}

Animation& RecorderToolbar::hoverOf(Control control) {
    return hovers[static_cast<size_t>(control)];
}

void RecorderToolbar::paint(Painter& painter, double now) {
    auto s = static_cast<float>(scale);
    Transform transform(painter, Matrix::scale(s, s));
    auto const& theme = palette.theme;

    float pillLeft = 0;
    float pillWidth = 0;
    float statusWidth = 0;
    auto controls = layout(pillLeft, pillWidth, statusWidth);

    auto pill = RectF(pillLeft, 8, pillLeft + pillWidth, 56);
    painter.boxShadow(pill, 24, 4, 6, -1, 0.1f);
    painter.boxShadow(pill, 24, 2, 4, -2, 0.1f);
    painter.fillRounded(pill, 24, color(theme.popover));
    painter.strokeRounded(RectF(pill.left + 0.5f, pill.top + 0.5f, pill.right - 0.5f, pill.bottom - 0.5f),
                          23.5f, color(theme.border), 1);

    auto foreground = color(theme.popoverForeground);
    float x = pillLeft + 7 + 10;
    float center = RECORDER_HEIGHT / 2;

    if (isBusy()) {
        drawSpinner(painter, x, center - 8, 16, foreground, now);
        x += 16;
    } else if (state.phase == RecorderPhase::Paused) {
        painter.icon(icons::pause, x, center - 8, 16, color(theme.mutedForeground));
        x += 16;
    } else {
        double phase = std::fmod(now, 2000.0) / 2000.0;
        double wave = phase < 0.5 ? cubicBezier(0.4, 0, 0.6, 1, phase * 2) : 1 - cubicBezier(0.4, 0, 0.6, 1, (phase - 0.5) * 2);
        painter.fillEllipse(x + 5, center, 5, 5, color(theme.destructive, static_cast<float>(1 - 0.5 * wave)));
        x += 10;
    }

    x += 8;
    auto text = label();
    auto layout = resources.layout(text, 14, FontWeight::Medium, FontKind::Sans, 0, true);
    painter.text(*layout, x, painter.middleBaseline(center, 14) - layout->baseline(), foreground);

    float separatorX = pillLeft + 7 + statusWidth + 4 + 4;
    painter.fillRect(RectF(separatorX, 20, separatorX + 1, 44), color(theme.border));

    for (auto const& control : controls) {
        ButtonStyle style;
        style.variant = control.control == Control::Stop ? ButtonVariant::Destructive : ButtonVariant::Ghost;
        style.hover = hoverOf(control.control).value(now);
        style.isPressed = pressed == control.control && hovered == control.control;
        style.isDisabled = control.isDisabled;

        if (style.isDisabled) painter.pushOpacity(0.5f);
        drawButton(painter, palette, control.rect, style);

        auto tint = buttonForeground(palette, style);
        float offset = buttonOffset(style);
        float iconX = control.rect.left + 8;
        float iconY = control.rect.top + 8 + offset;

        switch (control.control) {
        case Control::Microphone:
            painter.icon(isMuted(AudioSource::Microphone) ? icons::micOff : icons::mic, iconX, iconY, 16,
                         isMuted(AudioSource::Microphone) ? color(theme.destructive) : tint);
            break;
        case Control::System:
            painter.icon(isMuted(AudioSource::System) ? icons::volumeX : icons::volume2, iconX, iconY, 16,
                         isMuted(AudioSource::System) ? color(theme.destructive) : tint);
            break;
        case Control::Pause:
            painter.icon(state.phase == RecorderPhase::Paused ? icons::play : icons::pause, iconX, iconY, 16,
                         tint);
            break;
        case Control::Discard:
            painter.icon(icons::trash2, iconX, iconY, 16, tint);
            break;
        case Control::Stop: {
            float left = control.rect.left + 12;
            painter.icon(icons::square, left, iconY, 16, tint, true);
            painter.textAt(t(L"recorder.stop"), left + 20, painter.middleBaseline(center + offset, 14), 14,
                           FontWeight::Medium, tint);
            break;
        }
        default:
            break;
        }

        if (style.isDisabled) painter.popOpacity();
    }
}

bool RecorderToolbar::isAnimating(double now) {
    for (auto const& hover : hovers)
        if (hover.isRunning(now)) return true;
    return state.phase != RecorderPhase::Paused;
}

RecorderToolbar::Control RecorderToolbar::controlAt(double x, double y) {
    float pillLeft = 0;
    float pillWidth = 0;
    float statusWidth = 0;
    x /= scale;
    y /= scale;

    for (auto const& control : layout(pillLeft, pillWidth, statusWidth)) {
        auto const& r = control.rect;
        if (!control.isDisabled && x >= r.left && x < r.right && y >= r.top && y < r.bottom) return control.control;
    }
    return Control::None;
}

bool RecorderToolbar::contains(double x, double y) {
    float pillLeft = 0;
    float pillWidth = 0;
    float statusWidth = 0;
    layout(pillLeft, pillWidth, statusWidth);
    x /= scale;
    y /= scale;
    return x >= pillLeft && x < pillLeft + pillWidth && y >= 8 && y < 56;
}

void RecorderToolbar::activate(Control control) {
    switch (control) {
    case Control::Microphone:
        if (callbacks.onMute) callbacks.onMute(AudioSource::Microphone, !isMuted(AudioSource::Microphone));
        break;
    case Control::System:
        if (callbacks.onMute) callbacks.onMute(AudioSource::System, !isMuted(AudioSource::System));
        break;
    case Control::Pause:
        if (callbacks.onControl)
            callbacks.onControl(state.phase == RecorderPhase::Paused ? RecorderAction::Resume : RecorderAction::Pause);
        break;
    case Control::Discard:
        if (callbacks.onControl) callbacks.onControl(RecorderAction::Abort);
        break;
    case Control::Stop:
        if (callbacks.onControl) callbacks.onControl(RecorderAction::Stop);
        break;
    default:
        break;
    }
}

void RecorderToolbar::setHovered(Control control) {
    if (control == hovered) return;
    double now = nowMs();
    if (hovered != Control::None) hoverOf(hovered).animate(0, now, BUTTON_DURATION);
    if (control != Control::None) hoverOf(control).animate(1, now, BUTTON_DURATION);
    hovered = control;
}

void RecorderToolbar::pointerMove(double x, double y) {
    setHovered(controlAt(x, y));
}

void RecorderToolbar::pointerLeave() {
    setHovered(Control::None);
}

void RecorderToolbar::pointerDown(double x, double y) {
    pressed = controlAt(x, y);
}

void RecorderToolbar::pointerUp(double x, double y) {
    auto control = controlAt(x, y);
    auto previous = std::exchange(pressed, Control::None);
    if (control != Control::None && control == previous) activate(control);
}

}

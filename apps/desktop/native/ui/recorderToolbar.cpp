#include "recorderToolbar.h"

#include "fixture.h"

#include <shared/i18n.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cwchar>
#include <utility>

namespace fileship::ui {

namespace {

int64_t epochMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::wstring formatTime(int64_t milliseconds) {
    int64_t total = std::max<int64_t>(0, milliseconds / 1000);
    long long hours = total / 3600;
    long long minutes = total % 3600 / 60;
    long long seconds = total % 60;
    wchar_t buffer[32];

    if (hours) {
        std::swprintf(buffer, 32, L"%lld:%02lld:%02lld", hours, minutes, seconds);
    } else {
        std::swprintf(buffer, 32, L"%lld:%02lld", minutes, seconds);
    }
    return buffer;
}

}

RecorderToolbar::RecorderToolbar(ThemeMode theme, RecorderUiCallbacks value)
    : callbacks(std::move(value)), shared(Resources::acquire()),
      colors{theme == ThemeMode::Dark ? DARK_THEME : LIGHT_THEME, theme == ThemeMode::Dark} {
    current.phase = RecorderPhase::Preparing;
}

Resources& RecorderToolbar::resources() const {
    return *shared;
}

Palette const& RecorderToolbar::palette() const {
    return colors;
}

double RecorderToolbar::scale() const {
    return factor;
}

void RecorderToolbar::setScale(double value) {
    factor = value;
}

RecorderViewState const& RecorderToolbar::state() const {
    return current;
}

void RecorderToolbar::setState(RecorderViewState const& value) {
    current = value;
}

bool RecorderToolbar::isBusy() const {
    return current.phase == RecorderPhase::Preparing || current.phase == RecorderPhase::Countdown ||
           current.phase == RecorderPhase::Processing;
}

bool RecorderToolbar::isMuted(AudioSource source) const {
    return std::find(current.muted.begin(), current.muted.end(), source) != current.muted.end();
}

std::wstring RecorderToolbar::label() const {
    switch (current.phase) {
    case RecorderPhase::Preparing:
    case RecorderPhase::Idle:
        return t(L"recorder.starting");
    case RecorderPhase::Countdown:
        return t(L"recorder.startingIn", {{L"count", std::to_wstring(current.countdown)}});
    case RecorderPhase::Processing:
        return t(L"recorder.processing");
    default:
        break;
    }

    int64_t elapsed = current.elapsedMs + (current.sinceMs ? epochMs() - current.sinceMs : 0);
    auto time = formatTime(elapsed);
    return current.maxDurationMs ? time + L" / " + formatTime(current.maxDurationMs) : time;
}

std::vector<RecorderToolbar::ControlRect> RecorderToolbar::layout(float& pillLeft, float& pillWidth,
                                                                  float& statusWidth) {
    auto text = label();
    float textWidth = shared->measure(text, 14, FontWeight::Medium, FontKind::Sans, true);
    float indicator = isBusy() || current.phase == RecorderPhase::Paused ? 16.0f : 10.0f;
    statusWidth = std::max(96.0f, 10 + indicator + 8 + textWidth);

    float stopWidth = 12 + 16 + 4 + shared->measure(t(L"recorder.stop"), 14, FontWeight::Medium) + 12;
    std::vector<Control> buttons;
    std::vector<AudioSource> unique;
    for (auto source : current.sources)
        if (std::find(unique.begin(), unique.end(), source) == unique.end()) unique.push_back(source);
    for (auto source : unique) buttons.push_back(source == AudioSource::Microphone ? Control::Microphone : Control::System);
    buttons.push_back(Control::Pause);
    buttons.push_back(Control::Discard);

    float content = statusWidth + 4 + 9 + static_cast<float>(buttons.size()) * (4 + BUTTON_SIZE) + 4 + stopWidth;
    pillWidth = 1 + 6 + content + 6 + 1;
    pillLeft = (RECORDER_TOOLBAR_WIDTH - pillWidth) / 2;

    std::vector<ControlRect> result;
    float x = pillLeft + 7 + statusWidth + 4 + 9;
    float top = (RECORDER_TOOLBAR_HEIGHT - BUTTON_SIZE) / 2;

    for (auto control : buttons) {
        x += 4;
        bool isDisabled = control == Control::Pause    ? current.phase != RecorderPhase::Recording &&
                                                             current.phase != RecorderPhase::Paused
                          : control == Control::Discard ? current.phase == RecorderPhase::Processing
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
    auto s = static_cast<float>(factor);
    Transform transform(painter, Matrix::scale(s, s));
    auto const& theme = colors.theme;

    float pillLeft = 0;
    float pillWidth = 0;
    float statusWidth = 0;
    auto controls = layout(pillLeft, pillWidth, statusWidth);

    auto pill = RectF(pillLeft, 8, pillLeft + pillWidth, 56);
    painter.boxShadow(pill, 24, 4, 6, -1, 0.1f);
    painter.boxShadow(pill, 24, 2, 4, -2, 0.1f);
    painter.fillRounded(pill, 24, color(theme.popover));
    painter.strokeRounded(RectF(pill.left + 0.5f, pill.top + 0.5f, pill.right - 0.5f, pill.bottom - 0.5f), 23.5f,
                          color(theme.border), 1);

    auto foreground = color(theme.popoverForeground);
    float x = pillLeft + 7 + 10;
    float center = RECORDER_TOOLBAR_HEIGHT / 2;

    if (isBusy()) {
        drawSpinner(painter, x, center - 8, 16, foreground, now);
        x += 16;
    } else if (current.phase == RecorderPhase::Paused) {
        painter.icon(icons::pause, x, center - 8, 16, color(theme.mutedForeground));
        x += 16;
    } else {
        double phase = std::fmod(now, 2000.0) / 2000.0;
        double wave =
            phase < 0.5 ? cubicBezier(0.4, 0, 0.6, 1, phase * 2) : 1 - cubicBezier(0.4, 0, 0.6, 1, (phase - 0.5) * 2);
        painter.fillEllipse(x + 5, center, 5, 5, color(theme.destructive, static_cast<float>(1 - 0.5 * wave)));
        x += 10;
    }

    x += 8;
    auto text = label();
    auto textLayout = shared->layout(text, 14, FontWeight::Medium, FontKind::Sans, 0, true);
    painter.text(*textLayout, x, painter.middleBaseline(center, 14) - textLayout->baseline(), foreground);

    float separatorX = pillLeft + 7 + statusWidth + 4 + 4;
    painter.fillRect(RectF(separatorX, 20, separatorX + 1, 44), color(theme.border));

    for (auto const& control : controls) {
        ButtonStyle style;
        style.variant = control.control == Control::Stop ? ButtonVariant::Destructive : ButtonVariant::Ghost;
        style.hover = hoverOf(control.control).value(now);
        style.isPressed = pressed == control.control && hovered == control.control;
        style.isDisabled = control.isDisabled;

        if (style.isDisabled) painter.pushOpacity(0.5f);
        drawButton(painter, colors, control.rect, style);

        auto tint = buttonForeground(colors, style);
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
            painter.icon(current.phase == RecorderPhase::Paused ? icons::play : icons::pause, iconX, iconY, 16, tint);
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

bool RecorderToolbar::isAnimating(double now) const {
    for (auto const& hover : hovers)
        if (hover.isRunning(now)) return true;
    return current.phase != RecorderPhase::Paused;
}

bool RecorderToolbar::contains(double x, double y) {
    float pillLeft = 0;
    float pillWidth = 0;
    float statusWidth = 0;
    layout(pillLeft, pillWidth, statusWidth);
    x /= factor;
    y /= factor;
    return x >= pillLeft && x < pillLeft + pillWidth && y >= 8 && y < 56;
}

RecorderToolbar::Control RecorderToolbar::controlAt(double x, double y) {
    float pillLeft = 0;
    float pillWidth = 0;
    float statusWidth = 0;
    x /= factor;
    y /= factor;

    for (auto const& control : layout(pillLeft, pillWidth, statusWidth)) {
        auto const& r = control.rect;
        if (!control.isDisabled && x >= r.left && x < r.right && y >= r.top && y < r.bottom) return control.control;
    }
    return Control::None;
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
            callbacks.onControl(current.phase == RecorderPhase::Paused ? RecorderAction::Resume : RecorderAction::Pause);
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

void RecorderToolbar::notifyVisible() const {
    if (callbacks.onVisible) callbacks.onVisible();
}

void paintRecorderBorder(Painter& painter, Color const& tint, bool isHorizontal, int width, int height, float offset,
                         float dash) {
    int length = isHorizontal ? width : height;
    for (float start = -std::fmod(offset, dash * 2); start < length; start += dash * 2) {
        float end = std::min(static_cast<float>(length), start + dash);
        float begin = std::max(0.0f, start);
        if (end <= begin) continue;
        painter.fillRect(isHorizontal ? RectF(begin, 0, end, static_cast<float>(height))
                                      : RectF(0, begin, static_cast<float>(width), end),
                         tint);
    }
}

FixtureImage renderRecorderFixture(RecorderViewState const& state, ThemeMode theme, double scale) {
    RecorderToolbar toolbar(theme, {});
    toolbar.setScale(scale);
    toolbar.setState(state);

    FixtureImage image;
    image.width = static_cast<int>(std::round(RECORDER_TOOLBAR_WIDTH * scale));
    image.height = static_cast<int>(std::round(RECORDER_TOOLBAR_HEIGHT * scale));

    auto device = Device::create(toolbar.resources());
    auto canvas = device->createCanvas(image.width, image.height);
    canvas->painter().clear(Color{0, 0, 0, 0});
    toolbar.paint(canvas->painter(), nowMs());
    image.bgra = canvas->finish();
    return image;
}

}

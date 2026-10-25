#include "toolbar.h"

#include <shared/i18n.h>
#include <shared/utf8.h>

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <iterator>

namespace fileship::ui {

namespace {

constexpr float PADDING = 5;
constexpr float GAP = 4;
constexpr float SEPARATOR = 9;
constexpr float LABEL_HEIGHT = 19.25f;
constexpr int SLIDER = 200;
constexpr int TOGGLE_FILL = 210;
constexpr int TOGGLE_BACKGROUND = 211;
constexpr int TOGGLE_SHADOW = 212;
constexpr int CUSTOM_COLOR = 10;

char const* const COLORS[] = {"#ef4444", "#f97316", "#facc15", "#22c55e", "#06b6d4",
                              "#3b82f6", "#a855f7", "#ec4899", "#ffffff", "#000000"};

wchar_t const* const EMOJIS[] = {
    L"\U0001F44D", L"\U0001F44E", L"\U0001F440", L"\U0001F600", L"\U0001F602", L"\U0001F972", L"\U0001F60D",
    L"\U0001F914", L"\U0001F62E", L"\U0001F621", L"\U0001F62D", L"\U0001F64F", L"\U0001F44F", L"\U0001F525",
    L"\U0001F4AF", L"✅",      L"❌",      L"⚠️", L"❗", L"❓",      L"⭐",
    L"❤️", L"\U0001F4A1", L"\U0001F389", L"\U0001F680", L"\U0001F41B", L"\U0001F4CC", L"\U0001F449",
    L"\U0001F448", L"\U0001F446", L"\U0001F447", L"\U0001F91D",
};

struct OptionsLayout {
    bool hasRange = false;
    float sliderY = 0;
    std::vector<std::pair<int, float>> toggles;
    float height = 0;
};

OptionsLayout optionsLayout(Tool tool) {
    OptionsLayout layout;
    float y = 16;
    bool isFirst = true;

    if (sizeRange(tool)) {
        layout.hasRange = true;
        layout.sliderY = y + LABEL_HEIGHT + 12;
        y += LABEL_HEIGHT + 12 + 8;
        isFirst = false;
    }

    auto add = [&](int key) {
        if (!isFirst) y += 28;
        layout.toggles.push_back({key, y});
        y += 20;
        isFirst = false;
    };

    if (tool == Tool::Rectangle || tool == Tool::Ellipse) add(TOGGLE_FILL);
    if (tool == Tool::Text) add(TOGGLE_BACKGROUND);
    switch (tool) {
    case Tool::Rectangle:
    case Tool::Ellipse:
    case Tool::Arrow:
    case Tool::Line:
    case Tool::Freehand:
    case Tool::Text:
    case Tool::Step:
        add(TOGGLE_SHADOW);
        break;
    default:
        break;
    }

    layout.height = y + 16;
    return layout;
}

bool hasOptions(Tool tool) {
    return !optionsLayout(tool).toggles.empty() || sizeRange(tool).has_value();
}

std::wstring sizeLabel(Tool tool) {
    switch (tool) {
    case Tool::Text:
    case Tool::Emoji:
        return t(L"annotate.size");
    case Tool::Step:
        return t(L"annotate.radius");
    case Tool::Blur:
    case Tool::Pixelate:
        return t(L"annotate.strength");
    default:
        return t(L"annotate.thickness");
    }
}

std::wstring formatNumber(double value) {
    wchar_t buffer[32];
    swprintf(buffer, 32, L"%g", value);
    return buffer;
}

D2D1_RECT_F inflate(D2D1_RECT_F const& rect, float amount) {
    return D2D1::RectF(rect.left - amount, rect.top - amount, rect.right + amount, rect.bottom + amount);
}

bool inside(D2D1_RECT_F const& rect, Point point) {
    return point.x >= rect.left && point.y >= rect.top && point.x < rect.right && point.y < rect.bottom;
}

}

AnnotationToolbar::AnnotationToolbar(Resources& resources, Annotator& value, Palette const& colors)
    : res(resources), annotator(value), palette(colors) {}

void AnnotationToolbar::setSide(Side value) {
    side = value;
}

void AnnotationToolbar::setPalette(Palette const& value) {
    palette = value;
}

Animation& AnnotationToolbar::hoverOf(int key) {
    return hovers[key];
}

std::vector<AnnotationToolbar::Item> AnnotationToolbar::buildItems() {
    std::vector<Item> result;
    bool isVertical = side == Side::Left || side == Side::Right;
    auto tool = annotator.activeTool();

    auto add = [&](ItemKind kind, Tool value, std::wstring tooltip, std::wstring kbd, bool isActive, bool isDisabled) {
        Item item;
        item.kind = kind;
        item.tool = value;
        item.tooltip = std::move(tooltip);
        item.kbd = std::move(kbd);
        item.isActive = isActive;
        item.isDisabled = isDisabled;
        item.key = static_cast<int>(kind) * 100 + static_cast<int>(value);
        result.push_back(std::move(item));
    };

    add(ItemKind::Grip, Tool::Select, L"", L"", false, false);
    add(ItemKind::Region, Tool::Select, t(L"overlay.region"), L"C", !annotator.tool, false);

    for (auto const& info : TOOLS) {
        std::wstring kbd(1, static_cast<wchar_t>(std::towupper(info.shortcut)));
        add(ItemKind::Tool, info.tool, t(L"annotate.tools." + toWide(info.name)), kbd, annotator.tool == info.tool,
            false);
    }

    add(ItemKind::Separator, Tool::Select, L"", L"", false, false);
    if (isColoredTool(tool)) add(ItemKind::Color, Tool::Select, L"", L"", false, false);
    if (tool == Tool::Emoji) add(ItemKind::Emoji, Tool::Select, L"", L"", false, false);
    if (hasOptions(tool)) add(ItemKind::Options, Tool::Select, L"", L"", false, false);
    add(ItemKind::Undo, Tool::Select, t(L"annotate.undo"), L"Ctrl Z", false, !annotator.canUndo());
    add(ItemKind::Redo, Tool::Select, t(L"annotate.redo"), L"Ctrl Y", false, !annotator.canRedo());
    add(ItemKind::Delete, Tool::Select, t(L"annotate.delete"), L"Del", false, !annotator.selectedId);
    add(ItemKind::Separator, Tool::Select, L"", L"", false, false);
    result.back().key += 1;
    add(ItemKind::Cancel, Tool::Select, t(L"common.cancel"), L"Esc", false, false);

    float cursor = PADDING;
    for (size_t index = 0; index < result.size(); index++) {
        auto& item = result[index];
        if (index) cursor += GAP;

        float length = item.kind == ItemKind::Separator ? SEPARATOR : BUTTON_SIZE;
        if (isVertical) {
            item.rect = D2D1::RectF(PADDING, cursor, PADDING + BUTTON_SIZE, cursor + length);
        } else {
            item.rect = D2D1::RectF(cursor, PADDING, cursor + length, PADDING + BUTTON_SIZE);
        }
        cursor += length;
    }

    cursor += PADDING;
    currentSize = isVertical ? D2D1::SizeF(BUTTON_SIZE + PADDING * 2, cursor)
                             : D2D1::SizeF(cursor, BUTTON_SIZE + PADDING * 2);
    return result;
}

D2D1_SIZE_F AnnotationToolbar::size() {
    items = buildItems();
    return currentSize;
}

void AnnotationToolbar::place(double left, double top, double scale, D2D1_RECT_F const& bounds) {
    originX = left;
    originY = top;
    zoom = scale;
    viewport = bounds;
    items = buildItems();
}

D2D1_RECT_F AnnotationToolbar::toLocal(D2D1_RECT_F const& rect) const {
    auto z = static_cast<float>(zoom);
    auto x = static_cast<float>(originX);
    auto y = static_cast<float>(originY);
    return D2D1::RectF(x + rect.left * z, y + rect.top * z, x + rect.right * z, y + rect.bottom * z);
}

Point AnnotationToolbar::toToolbar(Point point) const {
    return {(point.x - originX) / zoom, (point.y - originY) / zoom};
}

Point AnnotationToolbar::toPopover(Point point) const {
    auto const& rect = popover->placement.rect;
    return {(point.x - rect.left) / zoom, (point.y - rect.top) / zoom};
}

bool AnnotationToolbar::contains(Point point) {
    auto local = toToolbar(point);
    return local.x >= 0 && local.y >= 0 && local.x < currentSize.width && local.y < currentSize.height;
}

bool AnnotationToolbar::popoverContains(Point point) {
    return popover && inside(popover->placement.rect, point);
}

bool AnnotationToolbar::isOverGrip(Point point) {
    auto item = itemAt(point);
    return item && item->kind == ItemKind::Grip;
}

bool AnnotationToolbar::isInteracting() const {
    return isSliderDragging || pressedKey >= 0 || popoverPressed >= 0;
}

AnnotationToolbar::Item const* AnnotationToolbar::itemAt(Point point) {
    if (!contains(point)) return nullptr;

    auto local = toToolbar(point);
    for (auto const& item : items)
        if (item.kind != ItemKind::Separator && inside(item.rect, local)) return &item;
    return nullptr;
}

D2D1_SIZE_F AnnotationToolbar::popoverSize(ItemKind kind) {
    switch (kind) {
    case ItemKind::Color:
        return D2D1::SizeF(16 + 5 * 28 + 4 * 8 + 16, 16 + 3 * 28 + 2 * 8 + 16);
    case ItemKind::Emoji:
        return D2D1::SizeF(288, 16 + 4 * 32 + 3 * 4 + 16);
    default:
        return D2D1::SizeF(256, optionsLayout(annotator.activeTool()).height);
    }
}

void AnnotationToolbar::updatePopover(double now) {
    if (!popover) return;

    auto found = std::find_if(items.begin(), items.end(), [&](Item const& item) { return item.kind == popover->kind; });
    if (found == items.end()) {
        popover.reset();
        isSliderDragging = false;
        return;
    }

    auto anchor = toLocal(found->rect);
    auto content = popoverSize(popover->kind);
    auto z = static_cast<float>(zoom);
    popover->placement = placeFloating(anchor, content.width * z, content.height * z, side, 4 * z, viewport);
    (void)now;
}

void AnnotationToolbar::openPopover(int kind, double now) {
    items = buildItems();
    auto value = static_cast<ItemKind>(kind);
    if (popover && popover->kind == value) return;
    if (popover) closePopover(now, true);

    popover = PopoverState{value, now, {}};
    updatePopover(now);
}

void AnnotationToolbar::closePopover(double now, bool animate) {
    if (!popover) return;

    if (animate) {
        closingPopover = *popover;
        closingPopover->start = now;
    }
    popover.reset();
    isSliderDragging = false;
    popoverHover = -1;
    popoverPressed = -1;
}

void AnnotationToolbar::activate(Item const& item, double now) {
    switch (item.kind) {
    case ItemKind::Region:
        annotator.setTool(std::nullopt);
        break;
    case ItemKind::Tool:
        annotator.setTool(item.tool);
        break;
    case ItemKind::Color:
    case ItemKind::Emoji:
    case ItemKind::Options:
        if (popover && popover->kind == item.kind) {
            closePopover(now, true);
        } else {
            openPopover(static_cast<int>(item.kind), now);
        }
        break;
    case ItemKind::Undo:
        annotator.undo();
        break;
    case ItemKind::Redo:
        annotator.redo();
        break;
    case ItemKind::Delete:
        annotator.removeSelected();
        break;
    case ItemKind::Cancel:
        if (onCancel) onCancel();
        break;
    default:
        break;
    }
}

void AnnotationToolbar::updateTooltip(Item const* item, double now) {
    bool wants = item && !item->tooltip.empty() && !item->isDisabled && item->key != suppressedKey;
    if (tooltip && wants && tooltip->key == item->key) return;

    if (tooltip) {
        closingTooltip = *tooltip;
        closingTooltip->start = now;
        tooltip.reset();
        tooltipClosedAt = now;
    }

    if (!wants) return;

    auto layout = measureTooltip(res, item->tooltip, item->kbd);
    auto z = static_cast<float>(zoom);
    TooltipState state;
    state.key = item->key;
    state.text = item->tooltip;
    state.kbd = item->kbd;
    state.start = now;
    state.isInstant = now - tooltipClosedAt < 300;
    state.placement = placeFloating(toLocal(item->rect), layout.width * z, layout.height * z, side, 5 * z, viewport);
    tooltip = std::move(state);
    if (tooltip->isInstant) closingTooltip.reset();
}

bool AnnotationToolbar::pointerMove(Point point, double now) {
    if (isSliderDragging) {
        setSliderValue(point);
        return true;
    }

    int element = popover && popoverContains(point) ? popoverElementAt(point) : -1;
    if (element != popoverHover) {
        if (popoverHover >= 0) hoverOf(10000 + popoverHover).animate(0, now, BUTTON_DURATION);
        if (element >= 0) hoverOf(10000 + element).animate(1, now, BUTTON_DURATION);
        popoverHover = element;
    }

    auto item = popoverContains(point) ? nullptr : itemAt(point);
    int key = item && !item->isDisabled ? item->key : -1;

    if (key != hoveredKey) {
        if (hoveredKey >= 0) hoverOf(hoveredKey).animate(0, now, BUTTON_DURATION);
        if (key >= 0) hoverOf(key).animate(1, now, BUTTON_DURATION);
        if (hoveredKey != key && suppressedKey != key) suppressedKey = -1;
        hoveredKey = key;
    }

    updateTooltip(item, now);
    return popoverContains(point) || contains(point);
}

void AnnotationToolbar::pointerLeave(double now) {
    if (hoveredKey >= 0) hoverOf(hoveredKey).animate(0, now, BUTTON_DURATION);
    if (popoverHover >= 0) hoverOf(10000 + popoverHover).animate(0, now, BUTTON_DURATION);
    hoveredKey = -1;
    popoverHover = -1;
    suppressedKey = -1;
    updateTooltip(nullptr, now);
}

bool AnnotationToolbar::pointerDown(Point point, double now) {
    if (popover && popoverContains(point)) {
        int element = popoverElementAt(point);
        popoverPressed = element;

        if (element == SLIDER) {
            isSliderDragging = true;
            setSliderValue(point);
        }
        return true;
    }

    auto item = itemAt(point);

    if (popover) {
        bool isTrigger = item && item->kind == popover->kind;
        closePopover(now, true);
        if (isTrigger) {
            pressedKey = -2;
            return true;
        }
    }

    if (!contains(point)) return false;
    if (!item || item->isDisabled) return true;

    suppressedKey = item->key;
    updateTooltip(nullptr, now);

    if (item->kind == ItemKind::Grip) {
        if (onDragStart) onDragStart(point);
        return true;
    }

    pressedKey = item->key;
    return true;
}

void AnnotationToolbar::pointerUp(Point point, double now) {
    if (isSliderDragging) {
        isSliderDragging = false;
        popoverPressed = -1;
        return;
    }

    if (popoverPressed >= 0) {
        int element = popover && popoverContains(point) ? popoverElementAt(point) : -1;
        int pressed = std::exchange(popoverPressed, -1);
        if (element == pressed) popoverClick(element, now);
        return;
    }

    int pressed = std::exchange(pressedKey, -1);
    if (pressed < 0) return;

    auto item = itemAt(point);
    if (item && item->key == pressed && !item->isDisabled) {
        auto copy = *item;
        activate(copy, now);
        items = buildItems();
        updatePopover(now);
    }
}

bool AnnotationToolbar::key(UINT key, double now) {
    if (key == VK_ESCAPE && popover) {
        closePopover(now, true);
        return true;
    }
    return false;
}

int AnnotationToolbar::popoverElementAt(Point point) {
    if (!popover) return -1;

    auto local = toPopover(point);

    switch (popover->kind) {
    case ItemKind::Color:
        for (int index = 0; index <= CUSTOM_COLOR; index++) {
            float x = 16 + static_cast<float>(index % 5) * 36;
            float y = 16 + static_cast<float>(index / 5) * 36;
            if (inside(D2D1::RectF(x, y, x + 28, y + 28), local)) return index;
        }
        return -1;
    case ItemKind::Emoji:
        for (int index = 0; index < static_cast<int>(std::size(EMOJIS)); index++) {
            float x = 16 + static_cast<float>(index % 8) * 32.5f;
            float y = 16 + static_cast<float>(index / 8) * 36;
            if (inside(D2D1::RectF(x, y, x + 32, y + 32), local)) return 100 + index;
        }
        return -1;
    default: {
        auto layout = optionsLayout(annotator.activeTool());
        if (layout.hasRange && inside(D2D1::RectF(16, layout.sliderY - 4, 240, layout.sliderY + 12), local)) return SLIDER;
        for (auto [toggle, y] : layout.toggles) {
            if (inside(D2D1::RectF(196, y, 240, y + 20), local)) return toggle;
            float labelWidth = res.measure(t(toggle == TOGGLE_FILL         ? L"annotate.fill"
                                             : toggle == TOGGLE_BACKGROUND ? L"annotate.background"
                                                                           : L"annotate.shadow"),
                                           14, DWRITE_FONT_WEIGHT_MEDIUM);
            if (inside(D2D1::RectF(16, y, 16 + labelWidth, y + 20), local)) return toggle;
        }
        return -1;
    }
    }
}

void AnnotationToolbar::setSliderValue(Point point) {
    auto range = sizeRange(annotator.activeTool());
    if (!range || !popover) return;

    auto local = toPopover(point);
    double fraction = std::clamp((local.x - 16) / 224, 0.0, 1.0);
    double value = std::round(range->first + (range->second - range->first) * fraction);
    if (value != annotator.current().size) annotator.setToolSize(value);
}

void AnnotationToolbar::popoverClick(int element, double now) {
    if (element >= 0 && element < CUSTOM_COLOR) {
        annotator.setToolColor(COLORS[element]);
    } else if (element == CUSTOM_COLOR) {
        if (chooseColor) {
            auto chosen = chooseColor(annotator.current().color);
            if (chosen) annotator.setToolColor(*chosen);
        }
    } else if (element >= 100 && element < 100 + static_cast<int>(std::size(EMOJIS))) {
        annotator.setEmoji(EMOJIS[element - 100]);
    } else if (element == TOGGLE_FILL) {
        annotator.setFill(!annotator.options.fill);
    } else if (element == TOGGLE_BACKGROUND) {
        annotator.setBackground(!annotator.options.background);
    } else if (element == TOGGLE_SHADOW) {
        annotator.setShadow(!annotator.options.shadow);
    }

    items = buildItems();
    updatePopover(now);
}

bool AnnotationToolbar::isAnimating(double now) {
    for (auto const& [key, animation] : hovers)
        if (animation.isRunning(now)) return true;

    if (popover && now - popover->start < POPOVER_DURATION) return true;
    if (closingPopover && now - closingPopover->start < POPOVER_DURATION) return true;
    if (tooltip && !tooltip->isInstant && now - tooltip->start < TOOLTIP_DURATION) return true;
    if (closingTooltip && now - closingTooltip->start < TOOLTIP_DURATION) return true;
    return false;
}

void AnnotationToolbar::drawItem(Painter& painter, Item const& item, double now) {
    auto context = painter.context();

    if (item.kind == ItemKind::Separator) {
        bool isVertical = side == Side::Left || side == Side::Right;
        auto const& r = item.rect;
        auto line = isVertical ? D2D1::RectF(r.left, r.top + 4, r.right, r.top + 5)
                               : D2D1::RectF(r.left + 4, r.top + 6, r.left + 5, r.bottom - 6);
        painter.fillRect(line, color(palette.theme.border));
        return;
    }

    ButtonStyle style;
    style.variant = item.isActive ? ButtonVariant::Secondary : ButtonVariant::Ghost;
    style.hover = hoverOf(item.key).value(now);
    style.isPressed = pressedKey == item.key && hoveredKey == item.key;
    style.isDisabled = item.isDisabled;
    style.hasPopup = item.kind == ItemKind::Color || item.kind == ItemKind::Emoji || item.kind == ItemKind::Options;
    style.isExpanded = style.hasPopup && popover && popover->kind == item.kind;

    if (style.isDisabled) painter.pushOpacity(0.5f);

    drawButton(painter, palette, item.rect, style);

    auto foreground = buttonForeground(palette, style);
    float offset = buttonOffset(style);
    float x = item.rect.left + 8;
    float y = item.rect.top + 8 + offset;

    switch (item.kind) {
    case ItemKind::Grip:
        painter.icon(side == Side::Left || side == Side::Right ? icons::gripHorizontal : icons::gripVertical, x, y, 16,
                     foreground);
        break;
    case ItemKind::Region:
        painter.icon(icons::scan, x, y, 16, foreground);
        break;
    case ItemKind::Tool:
        painter.icon(*TOOLS[static_cast<size_t>(item.tool)].icon, x, y, 16, foreground);
        break;
    case ItemKind::Color: {
        auto swatch = parseHex(annotator.current().color);
        painter.fillEllipse(x + 8, y + 8, 8, 8, swatch);
        painter.strokeEllipse(x + 8, y + 8, 7.5f, 7.5f, color(palette.theme.border), 1);
        break;
    }
    case ItemKind::Emoji: {
        auto const& emoji = annotator.options.emoji;
        float width = res.measure(emoji, 16, DWRITE_FONT_WEIGHT_NORMAL, FontKind::Emoji);
        float baseline = painter.middleBaseline(y + 8, 16, FontKind::Emoji);
        painter.textAt(emoji, x + 8 - width / 2, baseline, 16, DWRITE_FONT_WEIGHT_NORMAL, foreground, FontKind::Emoji);
        break;
    }
    case ItemKind::Options:
        painter.icon(icons::slidersHorizontal, x, y, 16, foreground);
        break;
    case ItemKind::Undo:
        painter.icon(icons::undo2, x, y, 16, foreground);
        break;
    case ItemKind::Redo:
        painter.icon(icons::redo2, x, y, 16, foreground);
        break;
    case ItemKind::Delete:
        painter.icon(icons::trash2, x, y, 16, foreground);
        break;
    case ItemKind::Cancel:
        painter.icon(icons::x, x, y, 16, foreground);
        break;
    default:
        break;
    }

    if (style.isDisabled) painter.popOpacity();
    (void)context;
}

void AnnotationToolbar::drawPopoverContent(Painter& painter, ItemKind kind, double now) {
    auto const& theme = palette.theme;
    auto foreground = color(theme.popoverForeground);

    if (kind == ItemKind::Color) {
        auto const& selected = annotator.current().color;

        for (int index = 0; index <= CUSTOM_COLOR; index++) {
            float x = 16 + static_cast<float>(index % 5) * 36;
            float y = 16 + static_cast<float>(index / 5) * 36;
            float scale = 1 + 0.1f * static_cast<float>(hoverOf(10000 + index).value(now));
            Transform grow(painter.context(), D2D1::Matrix3x2F::Scale(scale, scale, D2D1::Point2F(x + 14, y + 14)));

            if (index == CUSTOM_COLOR) {
                painter.fillEllipse(x + 14, y + 14, 14, 14, color(theme.muted));
                painter.strokeEllipse(x + 14, y + 14, 13.5f, 13.5f, color(theme.border), 1);
                painter.icon(icons::pipette, x + 7, y + 7, 14, foreground);
                continue;
            }

            painter.fillEllipse(x + 14, y + 14, 14, 14, parseHex(COLORS[index]));
            painter.strokeEllipse(x + 14, y + 14, 13.5f, 13.5f, color(theme.border), 1);

            if (selected == COLORS[index]) painter.strokeEllipse(x + 14, y + 14, 17, 17, color(theme.ring), 2);
        }
        return;
    }

    if (kind == ItemKind::Emoji) {
        for (int index = 0; index < static_cast<int>(std::size(EMOJIS)); index++) {
            float x = 16 + static_cast<float>(index % 8) * 32.5f;
            float y = 16 + static_cast<float>(index / 8) * 36;
            bool isPressed = annotator.options.emoji == EMOJIS[index];
            double hover = std::max(isPressed ? 1.0 : 0.0, hoverOf(10000 + 100 + index).value(now));
            if (hover > 0) painter.fillEllipse(x + 16, y + 16, 16, 16, color(theme.muted, static_cast<float>(hover)));

            float width = res.measure(EMOJIS[index], 18, DWRITE_FONT_WEIGHT_NORMAL, FontKind::Emoji);
            float baseline = painter.middleBaseline(y + 16, 18, FontKind::Emoji);
            painter.textAt(EMOJIS[index], x + 16 - width / 2, baseline, 18, DWRITE_FONT_WEIGHT_NORMAL, foreground,
                           FontKind::Emoji);
        }
        return;
    }

    auto tool = annotator.activeTool();
    auto layout = optionsLayout(tool);

    if (layout.hasRange) {
        auto range = *sizeRange(tool);
        double size = annotator.current().size;
        auto label = sizeLabel(tool) + L" · " + formatNumber(size);
        painter.textAt(label, 16, painter.middleBaseline(16 + LABEL_HEIGHT / 2, 14), 14, DWRITE_FONT_WEIGHT_MEDIUM,
                       foreground);

        double fraction = std::clamp((size - range.first) / (range.second - range.first), 0.0, 1.0);
        double hover = std::max(hoverOf(10000 + SLIDER).value(now), isSliderDragging ? 1.0 : 0.0);
        drawSlider(painter, palette, D2D1::RectF(16, layout.sliderY - 4, 240, layout.sliderY + 12), fraction, hover);
    }

    for (auto [toggle, y] : layout.toggles) {
        auto label = t(toggle == TOGGLE_FILL         ? L"annotate.fill"
                       : toggle == TOGGLE_BACKGROUND ? L"annotate.background"
                                                     : L"annotate.shadow");
        bool checked = toggle == TOGGLE_FILL         ? annotator.options.fill
                       : toggle == TOGGLE_BACKGROUND ? annotator.options.background
                                                     : annotator.options.shadow;

        if (!hovers.contains(20000 + toggle)) hovers.emplace(20000 + toggle, Animation(checked ? 1 : 0));
        auto& animation = hoverOf(20000 + toggle);
        animation.animate(checked ? 1 : 0, now, BUTTON_DURATION);

        painter.textAt(label, 16, painter.middleBaseline(y + 10, 14), 14, DWRITE_FONT_WEIGHT_MEDIUM, foreground);
        drawSwitch(painter, palette, D2D1::RectF(196, y, 240, y + 20), animation.value(now));
    }
}

void AnnotationToolbar::drawPopover(Painter& painter, PopoverState const& state, bool isOpening, double now) {
    double progress = std::clamp((now - state.start) / POPOVER_DURATION, 0.0, 1.0);
    bool isEmoji = state.kind == ItemKind::Emoji;
    auto reveal = revealAt(progress, isOpening, state.placement.side, !isEmoji, !isEmoji);
    if (reveal.opacity <= 0) return;

    auto const& rect = state.placement.rect;
    auto origin = state.placement.origin;
    auto z = static_cast<float>(zoom);
    auto scale = static_cast<float>(reveal.scale);

    Transform transform(painter.context(), D2D1::Matrix3x2F::Scale(z, z) *
                                               D2D1::Matrix3x2F::Translation(rect.left, rect.top) *
                                               D2D1::Matrix3x2F::Scale(scale, scale, origin) *
                                               D2D1::Matrix3x2F::Translation(reveal.shift.x * z, reveal.shift.y * z));

    painter.pushOpacity(static_cast<float>(reveal.opacity));
    auto content = popoverSize(state.kind);
    drawPopoverFrame(painter, palette, D2D1::RectF(0, 0, content.width, content.height), !isEmoji);
    drawPopoverContent(painter, state.kind, now);
    painter.popOpacity();
}

void AnnotationToolbar::drawTooltipState(Painter& painter, TooltipState const& state, bool isOpening, double now) {
    double progress = state.isInstant && isOpening ? 1 : std::clamp((now - state.start) / TOOLTIP_DURATION, 0.0, 1.0);
    auto reveal = revealAt(progress, isOpening, state.placement.side, true, true);
    if (reveal.opacity <= 0) return;

    auto const& rect = state.placement.rect;
    auto z = static_cast<float>(zoom);
    auto scale = static_cast<float>(reveal.scale);
    auto origin = state.placement.origin;

    Transform transform(painter.context(), D2D1::Matrix3x2F::Scale(z, z) *
                                               D2D1::Matrix3x2F::Translation(rect.left, rect.top) *
                                               D2D1::Matrix3x2F::Scale(scale, scale, origin) *
                                               D2D1::Matrix3x2F::Translation(reveal.shift.x * z, reveal.shift.y * z));

    float width = (rect.right - rect.left) / z;
    float height = (rect.bottom - rect.top) / z;
    bool isHorizontal = state.placement.side == Side::Top || state.placement.side == Side::Bottom;
    float arrowCenter = isHorizontal ? (origin.x - rect.left) / z : (origin.y - rect.top) / z;

    painter.pushOpacity(static_cast<float>(reveal.opacity));
    drawTooltip(painter, palette, D2D1::RectF(0, 0, width, height), state.placement.side, arrowCenter, state.text,
                state.kbd);
    painter.popOpacity();
}

void AnnotationToolbar::render(Painter& painter, double now) {
    items = buildItems();
    updatePopover(now);

    {
        auto z = static_cast<float>(zoom);
        Transform transform(painter.context(), D2D1::Matrix3x2F::Scale(z, z) *
                                                   D2D1::Matrix3x2F::Translation(static_cast<float>(originX),
                                                                                 static_cast<float>(originY)));

        auto frame = D2D1::RectF(0, 0, currentSize.width, currentSize.height);
        float radius = std::min(32.0f, std::min(currentSize.width, currentSize.height) / 2);
        painter.boxShadow(frame, radius, 4, 6, -1, 0.1f);
        painter.boxShadow(frame, radius, 2, 4, -2, 0.1f);
        painter.fillRounded(frame, radius, color(palette.theme.popover));
        painter.strokeRounded(inflate(frame, -0.5f), radius - 0.5f, color(palette.theme.border), 1);

        for (auto const& item : items) drawItem(painter, item, now);
    }

    if (closingPopover) {
        if (now - closingPopover->start >= POPOVER_DURATION) {
            closingPopover.reset();
        } else {
            drawPopover(painter, *closingPopover, false, now);
        }
    }
    if (popover) drawPopover(painter, *popover, true, now);

    if (closingTooltip) {
        if (now - closingTooltip->start >= TOOLTIP_DURATION) {
            closingTooltip.reset();
        } else {
            drawTooltipState(painter, *closingTooltip, false, now);
        }
    }
    if (tooltip) drawTooltipState(painter, *tooltip, true, now);
}

}

#pragma once

#include "annotator.h"
#include "widgets.h"

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fileship::ui {

class AnnotationToolbar {
public:
    AnnotationToolbar(Resources& resources, Annotator& annotator, Palette const& palette);

    std::function<void()> onCancel;
    std::function<void(Point)> onDragStart;
    std::function<std::optional<std::string>(std::string const&)> chooseColor;

    void setSide(Side value);
    void setPalette(Palette const& value);
    D2D1_SIZE_F size();
    void place(double left, double top, double zoom, D2D1_RECT_F const& viewport);

    bool contains(Point point);
    bool popoverContains(Point point);
    bool isOverGrip(Point point);
    bool isInteracting() const;

    bool pointerMove(Point point, double now);
    bool pointerDown(Point point, double now);
    void pointerUp(Point point, double now);
    void pointerLeave(double now);
    bool key(UINT key, double now);

    void render(Painter& painter, double now);
    bool isAnimating(double now);
    void openPopover(int kind, double now);

private:
    enum class ItemKind { Grip, Region, Tool, Separator, Color, Emoji, Options, Undo, Redo, Delete, Cancel };

    struct Item {
        ItemKind kind;
        Tool tool = Tool::Select;
        D2D1_RECT_F rect{};
        bool isActive = false;
        bool isDisabled = false;
        std::wstring tooltip;
        std::wstring kbd;
        int key = 0;
    };

    struct PopoverState {
        ItemKind kind;
        double start = 0;
        Placement placement;
    };

    struct TooltipState {
        int key = 0;
        std::wstring text;
        std::wstring kbd;
        Placement placement;
        double start = 0;
        bool isInstant = false;
    };

    std::vector<Item> buildItems();
    Item const* itemAt(Point point);
    D2D1_RECT_F toLocal(D2D1_RECT_F const& rect) const;
    Point toToolbar(Point point) const;
    Point toPopover(Point point) const;
    D2D1_SIZE_F popoverSize(ItemKind kind);
    void updatePopover(double now);
    void activate(Item const& item, double now);
    void closePopover(double now, bool animate);
    void updateTooltip(Item const* item, double now);

    int popoverElementAt(Point point);
    void popoverClick(int element, double now);
    void setSliderValue(Point point);

    void drawItem(Painter& painter, Item const& item, double now);
    void drawPopover(Painter& painter, PopoverState const& state, bool isOpening, double now);
    void drawPopoverContent(Painter& painter, ItemKind kind, double now);
    void drawTooltipState(Painter& painter, TooltipState const& state, bool isOpening, double now);

    Animation& hoverOf(int key);

    Resources& res;
    Annotator& annotator;
    Palette palette;
    Side side = Side::Bottom;
    std::vector<Item> items;
    double originX = 0;
    double originY = 0;
    double zoom = 1;
    D2D1_RECT_F viewport{};
    D2D1_SIZE_F currentSize{};

    std::unordered_map<int, Animation> hovers;
    int hoveredKey = -1;
    int pressedKey = -1;
    int suppressedKey = -1;
    int popoverHover = -1;
    int popoverPressed = -1;
    bool isSliderDragging = false;
    std::optional<PopoverState> popover;
    std::optional<PopoverState> closingPopover;
    std::optional<TooltipState> tooltip;
    std::optional<TooltipState> closingTooltip;
    double tooltipClosedAt = -1e9;
};

}

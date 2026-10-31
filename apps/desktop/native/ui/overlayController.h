#pragma once

#include "annotator.h"
#include "gfx.h"
#include "input.h"
#include "overlay.h"
#include "preferences.h"
#include "toolbar.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace fileship::ui {

class OverlayController;

class OverlayWindow {
public:
    virtual ~OverlayWindow() = default;

    virtual bool open() = 0;
    virtual void destroy() = 0;
    virtual void invalidate() = 0;
    virtual void textInputChanged(bool isEditing) = 0;
    virtual void caretMoved() = 0;
    virtual std::optional<std::string> pickColor(std::string const& current) = 0;
    virtual void copyText(std::wstring const& text) = 0;
    virtual std::wstring pasteText() = 0;
};

std::unique_ptr<OverlayWindow> createOverlayWindow(OverlayController& controller);

struct FixtureScene;

class OverlayController {
public:
    OverlayController(std::shared_ptr<OverlayRequest> request, OverlayCallback done);
    ~OverlayController();

    bool open();
    void close();

    Device& device() {
        return *gpu;
    }

    Resources& resources() {
        return *shared;
    }

    ScreenRect const& screenBounds() const {
        return bounds;
    }

    double scale() const {
        return ratio;
    }

    std::vector<ScreenRect> displayRects() const;
    bool isFinished() const {
        return isFinishing;
    }

    void render(Painter& painter);
    bool isAnimating();
    void pointerMove(Point point, bool shift);
    void pointerDown(Point point, bool shift);
    void pointerUp(Point point);
    void pointerCancel();
    void pointerLeave();
    void rightClick(Point point);
    void keyDown(Key key, Modifiers modifiers);
    void character(wchar_t character, Modifiers modifiers);
    void composition(std::wstring const& text);
    void commit(std::wstring const& text);
    CursorKind cursorAt(Point point);
    bool isEditingText() const;
    bool caretVisible() const;
    std::optional<Box> caretRect();
    void afterInput();

    std::vector<uint8_t> renderScene(FixtureScene const& scene);
    OverlayResult renderRegion(FixtureScene const& scene, Box const& region);
    void initializeDevice();

private:
    struct Display {
        int64_t id = 0;
        ScreenRect physical;
        Box local;
        double scaleFactor = 1;
        bool isPrimary = false;
        std::shared_ptr<Image> image;
    };

    struct Target {
        Box rect;
        std::vector<Box> controls;
    };

    enum class Mode { Idle, Pending, Drawing };

    struct RegionDrag {
        Point start;
        bool isDeselect = false;
    };

    struct ToolbarDrag {
        Point offset;
        Point pointer;
        int64_t display = 0;
    };

    struct ToolbarLayout {
        double left = 0;
        double top = 0;
        Point center;
    };

    void finish(OverlayResult result);
    void applyScene(FixtureScene const& scene);
    Box toLocal(ScreenRect const& rect) const;
    Box viewport() const;
    ScreenRect toPhysical(Box const& rect) const;
    ScreenRect toScreen(Box const& rect) const;
    Display const* displayAt(Point point) const;
    Box targetAt(Point point) const;
    bool isRecording() const;
    bool hasToolbar() const;
    bool isAnnotating() const;
    Display const* toolbarDisplay() const;
    Box toolbarHost() const;
    double toolbarZoom() const;
    ToolbarEdge edgeAt(Point point) const;
    ToolbarEdge currentEdge() const;
    Side toolbarSide() const;
    ToolbarLayout layoutToolbar();
    void startToolbarDrag(Point point);
    void moveToolbarDrag(Point point);
    void endToolbarDrag();
    std::optional<Box> animatedHover();
    void setHover(std::optional<Box> next);
    std::vector<uint8_t> readSource(PixelRect const& rect);
    void drawLabel(Painter& painter, std::wstring const& text, double x, double y);
    void drawSourcePixels(Painter& painter, ScreenRect const& source, RectF const& destination);
    void drawMagnifier(Painter& painter, Point point);
    OverlayResult exportRegion(Box const& rect);
    void finishRegion(Box const& rect);
    bool isLargeEnough(Box const& rect) const;
    void cancelRegion();

    std::shared_ptr<OverlayRequest> request;
    OverlayCallback done;
    std::shared_ptr<Resources> shared;
    std::unique_ptr<Device> gpu;
    std::unique_ptr<OverlayWindow> window;
    Palette palette;
    ScreenRect bounds;
    double ratio = 1;
    std::vector<Display> displays;
    std::vector<Target> windows;
    OverlayPreferences preferences;
    OverlayPreferences loaded;
    std::unique_ptr<Annotator> annotator;
    std::unique_ptr<AnnotationToolbar> toolbar;

    Mode mode = Mode::Idle;
    std::optional<Box> selection;
    std::optional<Point> pointer;
    std::optional<Box> hover;
    std::optional<std::pair<Box, double>> hoverAnimation;
    std::optional<RegionDrag> regionDrag;
    std::optional<ToolbarDrag> toolbarDrag;
    bool isFinishing = false;
    bool isAnimatingHover = false;
    bool wasEditing = false;
};

}

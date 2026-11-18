#pragma once

#include "annotator.h"
#include "overlay.h"

#include <memory>
#include <optional>
#include <string>

namespace fileship::ui {

enum class Cursor { Arrow, Hand, Cross, Text, Move, ResizeNwse, ResizeNesw, ResizeNs, ResizeWe };

struct Modifiers {
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
};

class OverlayClient {
public:
    virtual ~OverlayClient() = default;

    virtual void paint(Painter& painter) = 0;
    virtual bool isAnimating() = 0;
    virtual void pointerMove(Point point, bool shift) = 0;
    virtual void pointerLeave() = 0;
    virtual void pointerDown(Point point, bool shift) = 0;
    virtual void pointerUp(Point point) = 0;
    virtual void pointerCancel() = 0;
    virtual void rightClick(Point point) = 0;
    virtual void keyDown(Key key, Modifiers modifiers) = 0;
    virtual void character(wchar_t character, Modifiers modifiers) = 0;
    virtual Cursor cursorAt(Point point) = 0;
    virtual void cancel() = 0;
};

class OverlayWindow {
public:
    static std::unique_ptr<OverlayWindow> open(Canvas& canvas, OverlayRequest const& request, double ratio,
                                               OverlayClient& client, Annotator& annotator);

    virtual ~OverlayWindow() = default;

    virtual void close() = 0;
    virtual std::optional<std::string> pickColor(std::string const& current) = 0;
};

}

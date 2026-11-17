#pragma once

#include "gfx.h"

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace fileship::ui {

enum class Tool { Select, Rectangle, Ellipse, Arrow, Line, Freehand, Highlight, Text, Step, Blur, Pixelate, Eraser, Emoji };

constexpr size_t TOOL_COUNT = 13;

struct ToolInfo {
    Tool tool;
    char const* name;
    wchar_t shortcut;
    Icon const* icon;
};

extern ToolInfo const TOOLS[TOOL_COUNT];

bool isColoredTool(Tool tool);
std::optional<std::pair<int, int>> sizeRange(Tool tool);

struct ToolOptions {
    std::string color;
    double size = 0;
};

struct AnnotationOptions {
    std::array<ToolOptions, TOOL_COUNT> tools;
    bool fill = false;
    bool background = false;
    bool shadow = true;
    std::wstring emoji = L"\U0001F44D";
};

AnnotationOptions defaultAnnotationOptions();

struct Shape {
    std::string id;
    Tool type = Tool::Rectangle;
    double x = 0;
    double y = 0;
    double scaleX = 1;
    double scaleY = 1;
    double width = 0;
    double height = 0;
    std::vector<double> points;
    std::wstring text;
    std::string color;
    double size = 0;
    bool fill = false;
    bool shadow = false;
    bool background = false;
    int value = 0;

    bool operator==(Shape const&) const = default;
};

bool isBoxShape(Tool type);
bool isLineShape(Tool type);

struct AnnotationSource {
    double scale = 1;
    std::function<std::vector<uint8_t>(RECT const&)> read;
};

enum class Anchor { None, TopLeft, Top, TopRight, Right, BottomRight, Bottom, BottomLeft, Left };

struct Point {
    double x = 0;
    double y = 0;
};

class Annotator {
public:
    Annotator(Resources& resources, Theme const& theme);

    std::optional<Tool> tool;
    AnnotationOptions options;
    std::vector<Shape> shapes;
    std::optional<std::string> selectedId;

    bool canUndo() const {
        return !past.empty();
    }

    bool canRedo() const {
        return !future.empty();
    }

    bool isEditingText() const {
        return editor.has_value();
    }

    Tool activeTool() const;
    ToolOptions const& current() const;

    void setSource(AnnotationSource value);
    void setTheme(Theme const& value);
    void setTool(std::optional<Tool> value);
    void setToolColor(std::string const& value);
    void setToolSize(double value);
    void setFill(bool value);
    void setBackground(bool value);
    void setShadow(bool value);
    void setEmoji(std::wstring const& value);

    void reset();
    void undo();
    void redo();
    bool removeSelected();
    void select(std::optional<std::string> id);
    bool handleKey(UINT key, bool ctrl, bool shift);
    bool handleShortcut(wchar_t character);

    bool isOverShape(Point point);
    std::optional<Anchor> anchorAt(Point point);
    bool isSelectable() const;

    void pointerDown(Point point);
    void pointerMove(Point point, bool shift);
    void pointerUp(Point point);
    bool isBusy() const;

    void render(ID2D1DeviceContext* context, bool forExport);
    void releaseDeviceResources();

    void finishTextEdit();
    bool editorKey(UINT key, bool ctrl, bool shift);
    void editorChar(wchar_t character);
    void editorComposition(std::wstring const& text);
    void editorCommit(std::wstring const& text);
    bool editorContains(Point point);
    void editorClick(Point point, bool shift);
    Point editorCaret(float& height);
    bool caretVisible() const;

    std::function<void()> onChange;

private:
    struct Draft {
        Shape shape;
        Point start;
    };

    struct Drag {
        std::string id;
        Point start;
        Shape original;
        bool isActive = false;
    };

    struct Resize {
        std::string id;
        Anchor anchor;
        Box box;
        Shape original;
    };

    struct Editor {
        Shape shape;
        bool isNew = false;
        std::wstring text;
        size_t caret = 0;
        size_t anchor = 0;
        std::wstring composition;
        double blinkStart = 0;
    };

    struct EffectCache {
        std::string key;
        com_ptr<ID2D1Bitmap1> bitmap;
        RECT crop{};
    };

    Shape const* findShape(std::optional<std::string> const& id) const;
    void commit(std::vector<Shape> next);
    void replaceShape(Shape const& shape);
    std::optional<Shape> startShape(Point point);
    void updateDraft(Point point, bool isSquare);
    void finishDraft();
    void editText(Shape const& shape, bool isNew);
    void syncSelected();
    void changed();

    std::string sampleColor(Point point);
    std::string newId();

    Box nodeBox(Shape const& shape);
    Box localBox(Shape const& shape);
    Shape transformed(Shape const& shape, Box const& from, Box const& to);
    Box resizedBox(Box const& box, Anchor anchor, Point point, bool keepRatio);
    bool hitShape(Shape const& shape, Point point);
    std::optional<size_t> shapeAt(Point point);
    std::vector<Anchor> anchors(Shape const& shape) const;
    Point anchorPoint(Box const& box, Anchor anchor) const;

    void drawShape(ID2D1DeviceContext* context, Painter& painter, Shape const& shape);
    void drawShapeBody(ID2D1DeviceContext* context, Painter& painter, Shape const& shape);
    void drawEffect(ID2D1DeviceContext* context, Shape const& shape);
    void drawTransformer(Painter& painter, Shape const& shape);
    void drawEditor(Painter& painter);
    com_ptr<ID2D1PathGeometry> lineGeometry(Shape const& shape);
    com_ptr<ID2D1PathGeometry> arrowHead(Shape const& shape);
    com_ptr<IDWriteTextLayout4> textLayout(Shape const& shape, std::wstring const& text, float scale = 1);
    Box textBox(Shape const& shape);
    Box emojiBox(Shape const& shape);

    std::wstring editorText() const;
    void editorInsert(std::wstring const& text);
    void editorErase(bool forward);
    void editorMove(size_t position, bool extend);
    size_t editorIndexAt(Point point);
    void editorLayout(com_ptr<IDWriteTextLayout4>& layout, Box& box, float& padding, float& scale);

    Resources& res;
    Theme theme;
    AnnotationSource source;
    std::vector<std::vector<Shape>> past;
    std::vector<std::vector<Shape>> future;
    std::optional<Draft> draft;
    std::optional<Drag> drag;
    std::optional<Resize> resize;
    std::optional<Editor> editor;
    std::vector<std::pair<std::string, EffectCache>> effects;
    std::string lastSync;
    int idCounter = 0;
    double lastClickTime = 0;
    std::string lastClickId;
    std::string pendingDoubleClick;
    Point resizeOffset;
};

}

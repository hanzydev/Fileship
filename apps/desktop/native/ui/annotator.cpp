#include "annotator.h"

#include "effects.h"

#include <d2d1effects_2.h>

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <numbers>

namespace fileship::ui {

namespace {

constexpr char const* RED = "#ef4444";
constexpr double MIN_SIZE = 3;
constexpr double DOUBLE_CLICK_MS = 400;
constexpr double DRAG_DISTANCE = 3;
constexpr float ANCHOR_SIZE = 8;

std::string contrastColor(std::string const& value) {
    auto parsed = parseHex(value);
    double luminance = 0.299 * std::lround(parsed.r * 255) + 0.587 * std::lround(parsed.g * 255) +
                       0.114 * std::lround(parsed.b * 255);
    return luminance / 255 > 0.6 ? "#000000" : "#ffffff";
}

bool hasColor(Tool type) {
    return type != Tool::Emoji;
}

bool hasShadow(Tool type) {
    return type != Tool::Emoji;
}

bool keepsRatio(Tool type) {
    return type == Tool::Text || type == Tool::Emoji || type == Tool::Step;
}

bool drawsShadow(Shape const& shape) {
    switch (shape.type) {
    case Tool::Rectangle:
    case Tool::Ellipse:
    case Tool::Arrow:
    case Tool::Line:
    case Tool::Freehand:
    case Tool::Step:
    case Tool::Text:
        return shape.shadow;
    default:
        return false;
    }
}

std::vector<double> controlPoints(double x0, double y0, double x1, double y1, double x2, double y2, double t) {
    double d01 = std::hypot(x1 - x0, y1 - y0);
    double d12 = std::hypot(x2 - x1, y2 - y1);
    double sum = d01 + d12;
    if (sum == 0) return {x1, y1, x1, y1};

    double fa = t * d01 / sum;
    double fb = t * d12 / sum;
    return {x1 - fa * (x2 - x0), y1 - fa * (y2 - y0), x1 + fb * (x2 - x0), y1 + fb * (y2 - y0)};
}

std::vector<double> expandPoints(std::vector<double> const& p, double tension) {
    std::vector<double> result;
    size_t length = p.size();

    for (size_t n = 2; n + 2 < length; n += 2) {
        auto cp = controlPoints(p[n - 2], p[n - 1], p[n], p[n + 1], p[n + 2], p[n + 3], tension);
        if (std::isnan(cp[0])) continue;
        result.insert(result.end(), {cp[0], cp[1], p[n], p[n + 1], cp[2], cp[3]});
    }

    return result;
}

D2D1_POINT_2F point2(double x, double y) {
    return D2D1::Point2F(static_cast<float>(x), static_cast<float>(y));
}

D2D1_RECT_F rect(double x, double y, double width, double height) {
    return D2D1::RectF(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x + width),
                       static_cast<float>(y + height));
}

double pointerAngle(std::vector<double> const& points) {
    size_t n = points.size();
    double ex = points[n - 2];
    double ey = points[n - 1];
    double dx = 0;
    double dy = 0;

    for (size_t i = n - 2; !dx && !dy; i -= 2) {
        dx = ex - points[i];
        dy = ey - points[i + 1];
        if (i == 0) break;
    }

    double turn = std::numbers::pi * 2;
    return std::fmod(std::atan2(dy, dx) + turn, turn);
}

double arrowPointer(Shape const& shape) {
    return shape.size * 3 + 6;
}

}

ToolInfo const TOOLS[TOOL_COUNT] = {
    {Tool::Select, "select", L'v', &icons::mousePointer2},
    {Tool::Rectangle, "rectangle", L'r', &icons::square},
    {Tool::Ellipse, "ellipse", L'e', &icons::circle},
    {Tool::Arrow, "arrow", L'a', &icons::moveUpRight},
    {Tool::Line, "line", L'l', &icons::minus},
    {Tool::Freehand, "freehand", L'f', &icons::pencil},
    {Tool::Highlight, "highlight", L'h', &icons::highlighter},
    {Tool::Text, "text", L't', &icons::type},
    {Tool::Step, "step", L'n', &icons::listOrdered},
    {Tool::Blur, "blur", L'b', &icons::droplets},
    {Tool::Pixelate, "pixelate", L'p', &icons::grid3x3},
    {Tool::Eraser, "eraser", L'x', &icons::eraser},
    {Tool::Emoji, "emoji", L'j', &icons::smile},
};

bool isColoredTool(Tool tool) {
    switch (tool) {
    case Tool::Rectangle:
    case Tool::Ellipse:
    case Tool::Arrow:
    case Tool::Line:
    case Tool::Freehand:
    case Tool::Highlight:
    case Tool::Text:
    case Tool::Step:
        return true;
    default:
        return false;
    }
}

std::optional<std::pair<int, int>> sizeRange(Tool tool) {
    switch (tool) {
    case Tool::Rectangle:
    case Tool::Ellipse:
    case Tool::Arrow:
    case Tool::Line:
    case Tool::Freehand:
        return std::pair{1, 30};
    case Tool::Text:
        return std::pair{10, 120};
    case Tool::Step:
        return std::pair{10, 60};
    case Tool::Blur:
    case Tool::Pixelate:
        return std::pair{2, 40};
    case Tool::Emoji:
        return std::pair{16, 256};
    default:
        return std::nullopt;
    }
}

AnnotationOptions defaultAnnotationOptions() {
    AnnotationOptions result;
    result.tools = {{
        {RED, 0},
        {RED, 4},
        {RED, 4},
        {RED, 4},
        {RED, 4},
        {RED, 4},
        {"#facc15", 0},
        {RED, 28},
        {RED, 18},
        {RED, 12},
        {RED, 12},
        {RED, 0},
        {RED, 64},
    }};
    return result;
}

bool isBoxShape(Tool type) {
    switch (type) {
    case Tool::Rectangle:
    case Tool::Ellipse:
    case Tool::Highlight:
    case Tool::Blur:
    case Tool::Pixelate:
    case Tool::Eraser:
        return true;
    default:
        return false;
    }
}

bool isLineShape(Tool type) {
    return type == Tool::Arrow || type == Tool::Line || type == Tool::Freehand;
}

Annotator::Annotator(Resources& resources, Theme const& value)
    : options(defaultAnnotationOptions()), res(resources), theme(value) {}

Shape const* Annotator::findShape(std::optional<std::string> const& id) const {
    if (!id) return nullptr;
    for (auto const& shape : shapes)
        if (shape.id == *id) return &shape;
    return nullptr;
}

Tool Annotator::activeTool() const {
    auto selected = findShape(selectedId);
    if (tool == Tool::Select && selected) return selected->type;
    return tool.value_or(Tool::Select);
}

ToolOptions const& Annotator::current() const {
    return options.tools[static_cast<size_t>(activeTool())];
}

void Annotator::setSource(AnnotationSource value) {
    source = std::move(value);
    effects.clear();
}

void Annotator::setTheme(Theme const& value) {
    theme = value;
}

void Annotator::commit(std::vector<Shape> next) {
    past.push_back(std::move(shapes));
    if (past.size() > 100) past.erase(past.begin(), past.begin() + static_cast<ptrdiff_t>(past.size() - 100));
    future.clear();
    shapes = std::move(next);
}

void Annotator::replaceShape(Shape const& shape) {
    auto next = shapes;
    for (auto& item : next)
        if (item.id == shape.id) item = shape;
    commit(std::move(next));
}

void Annotator::changed() {
    syncSelected();
    if (onChange) onChange();
}

void Annotator::syncSelected() {
    auto const& toolOptions = current();
    char buffer[160];
    snprintf(buffer, sizeof(buffer), "%d|%d|%d|%s|%g", options.fill, options.background, options.shadow,
             toolOptions.color.c_str(), toolOptions.size);

    std::string key = buffer;
    if (key == lastSync) return;
    lastSync = key;

    auto shape = findShape(selectedId);
    if (!shape || shape->type != activeTool()) return;

    Shape next = *shape;
    if (hasColor(next.type) && next.type != Tool::Eraser) next.color = toolOptions.color;
    next.size = toolOptions.size;
    if (isBoxShape(next.type) && next.type != Tool::Eraser) next.fill = options.fill;
    if (next.type == Tool::Text) next.background = options.background;
    if (hasShadow(next.type)) next.shadow = options.shadow;

    if (next == *shape) return;
    replaceShape(next);
}

std::string Annotator::newId() {
    return "s" + std::to_string(++idCounter);
}

std::string Annotator::sampleColor(Point point) {
    if (!source.read) return "#ffffff";

    LONG x = std::lround(point.x * source.scale);
    LONG y = std::lround(point.y * source.scale);
    auto pixel = source.read(RECT{x, y, x + 1, y + 1});
    if (pixel.size() < 4) return "#000000";

    return toHex(rgba(pixel[2], pixel[1], pixel[0]));
}

bool Annotator::isSelectable() const {
    return tool == Tool::Select || !tool;
}

void Annotator::setTool(std::optional<Tool> value) {
    if (value == tool) return;

    tool = value;
    finishTextEdit();
    if (tool != Tool::Select) select(std::nullopt);
    changed();
}

void Annotator::setToolColor(std::string const& value) {
    options.tools[static_cast<size_t>(activeTool())].color = value;
    changed();
}

void Annotator::setToolSize(double value) {
    options.tools[static_cast<size_t>(activeTool())].size = value;
    changed();
}

void Annotator::setFill(bool value) {
    options.fill = value;
    changed();
}

void Annotator::setBackground(bool value) {
    options.background = value;
    changed();
}

void Annotator::setShadow(bool value) {
    options.shadow = value;
    changed();
}

void Annotator::setEmoji(std::wstring const& value) {
    options.emoji = value;
    changed();
}

void Annotator::select(std::optional<std::string> id) {
    selectedId = std::move(id);
    changed();
}

void Annotator::reset() {
    finishTextEdit();
    draft.reset();
    drag.reset();
    resize.reset();
    shapes.clear();
    past.clear();
    future.clear();
    selectedId.reset();
    effects.clear();
    changed();
}

void Annotator::undo() {
    if (past.empty()) return;

    finishTextEdit();
    future.insert(future.begin(), std::move(shapes));
    shapes = std::move(past.back());
    past.pop_back();
    if (!findShape(selectedId)) selectedId.reset();
    changed();
}

void Annotator::redo() {
    if (future.empty()) return;

    finishTextEdit();
    past.push_back(std::move(shapes));
    shapes = std::move(future.front());
    future.erase(future.begin());
    if (!findShape(selectedId)) selectedId.reset();
    changed();
}

bool Annotator::removeSelected() {
    if (!selectedId) return false;

    auto id = *selectedId;
    std::vector<Shape> next;
    for (auto const& shape : shapes)
        if (shape.id != id) next.push_back(shape);

    commit(std::move(next));
    selectedId.reset();
    changed();
    return true;
}

bool Annotator::handleKey(UINT key, bool ctrl, bool shift) {
    if (editor) return false;

    if (ctrl && key == 'Z') {
        if (shift) {
            redo();
        } else {
            undo();
        }
        return true;
    }

    if (ctrl && key == 'Y') {
        redo();
        return true;
    }

    if (key == VK_DELETE || key == VK_BACK) return removeSelected();
    return false;
}

bool Annotator::handleShortcut(wchar_t character) {
    if (editor) return false;

    auto lower = static_cast<wchar_t>(std::towlower(character));
    for (auto const& info : TOOLS) {
        if (info.shortcut != lower) continue;
        setTool(info.tool);
        return true;
    }

    return false;
}

std::optional<Shape> Annotator::startShape(Point point) {
    auto const& toolOptions = options.tools[static_cast<size_t>(tool.value_or(Tool::Select))];
    Shape shape;
    shape.id = newId();
    shape.x = point.x;
    shape.y = point.y;
    shape.color = toolOptions.color;
    shape.size = toolOptions.size;
    shape.shadow = options.shadow;

    switch (tool.value_or(Tool::Select)) {
    case Tool::Rectangle:
    case Tool::Ellipse:
    case Tool::Highlight:
    case Tool::Blur:
    case Tool::Pixelate:
        shape.type = *tool;
        shape.fill = options.fill;
        return shape;
    case Tool::Eraser:
        shape.type = Tool::Eraser;
        shape.color = sampleColor(point);
        shape.fill = true;
        shape.shadow = false;
        return shape;
    case Tool::Arrow:
    case Tool::Line:
        shape.type = *tool;
        shape.points = {0, 0, 0, 0};
        return shape;
    case Tool::Freehand:
        shape.type = Tool::Freehand;
        shape.points = {0, 0};
        return shape;
    case Tool::Text:
        shape.type = Tool::Text;
        shape.background = options.background;
        return shape;
    case Tool::Step: {
        int value = 0;
        for (auto const& item : shapes)
            if (item.type == Tool::Step) value = std::max(value, item.value);
        shape.type = Tool::Step;
        shape.value = value + 1;
        return shape;
    }
    case Tool::Emoji:
        shape.type = Tool::Emoji;
        shape.x = point.x - toolOptions.size / 2;
        shape.y = point.y - toolOptions.size / 2;
        shape.text = options.emoji;
        shape.color.clear();
        shape.shadow = false;
        return shape;
    default:
        return std::nullopt;
    }
}

void Annotator::updateDraft(Point point, bool isSquare) {
    if (!draft) return;

    auto& shape = draft->shape;
    auto start = draft->start;
    double dx = point.x - start.x;
    double dy = point.y - start.y;

    if (isSquare) {
        double length = std::max(std::abs(dx), std::abs(dy));
        dx = (dx < 0 ? -1 : 1) * length;
        dy = (dy < 0 ? -1 : 1) * length;
    }

    if (isBoxShape(shape.type)) {
        shape.x = std::min(start.x, start.x + dx);
        shape.y = std::min(start.y, start.y + dy);
        shape.width = std::abs(dx);
        shape.height = std::abs(dy);
    } else if (shape.type == Tool::Freehand) {
        shape.points.push_back(point.x - start.x);
        shape.points.push_back(point.y - start.y);
    } else if (isLineShape(shape.type)) {
        shape.points = {0, 0, dx, dy};
    }
}

void Annotator::finishDraft() {
    if (!draft) return;

    auto shape = std::move(draft->shape);
    draft.reset();

    bool isTooSmall = false;
    if (isBoxShape(shape.type)) {
        isTooSmall = shape.width < MIN_SIZE || shape.height < MIN_SIZE;
    } else if (isLineShape(shape.type)) {
        double largest = 0;
        for (double value : shape.points) largest = std::max(largest, std::abs(value));
        isTooSmall = largest < MIN_SIZE;
    }

    if (isTooSmall) return;

    auto next = shapes;
    next.push_back(shape);
    commit(std::move(next));
    select(shape.id);
}

bool Annotator::isBusy() const {
    return draft || drag || resize;
}

void Annotator::pointerDown(Point point) {
    if (editor) return;

    if (auto anchor = anchorAt(point)) {
        auto shape = *findShape(selectedId);
        auto box = nodeBox(shape);
        auto origin = anchorPoint(box, *anchor);
        resize = Resize{shape.id, *anchor, box, shape};
        resizeOffset = {origin.x - point.x, origin.y - point.y};
        pendingDoubleClick.clear();
        lastClickId.clear();
        return;
    }

    auto index = shapeAt(point);
    if (index) {
        auto const& shape = shapes[*index];
        double now = nowMs();

        pendingDoubleClick = lastClickId == shape.id && now - lastClickTime < DOUBLE_CLICK_MS ? shape.id : "";
        lastClickId = shape.id;
        lastClickTime = now;

        if (isSelectable() || selectedId == shape.id) {
            auto id = shape.id;
            select(id);
            drag = Drag{id, point, *findShape(id)};
            return;
        }
    } else {
        lastClickId.clear();
        pendingDoubleClick.clear();
    }

    if (isSelectable()) {
        if (!index) select(std::nullopt);
        return;
    }

    select(std::nullopt);

    auto shape = startShape(point);
    if (!shape) return;

    if (shape->type == Tool::Text) {
        editText(*shape, true);
        return;
    }

    if (shape->type == Tool::Step || shape->type == Tool::Emoji) {
        auto next = shapes;
        next.push_back(*shape);
        commit(std::move(next));
        select(shape->id);
        return;
    }

    draft = Draft{*shape, point};
}

void Annotator::pointerMove(Point point, bool shift) {
    if (resize) {
        auto found = std::find_if(shapes.begin(), shapes.end(), [&](Shape const& item) { return item.id == resize->id; });
        if (found == shapes.end()) return;

        bool isCorner = resize->anchor == Anchor::TopLeft || resize->anchor == Anchor::TopRight ||
                        resize->anchor == Anchor::BottomLeft || resize->anchor == Anchor::BottomRight;
        bool keepRatio = keepsRatio(resize->original.type) || (shift && isCorner);
        Point target{point.x + resizeOffset.x, point.y + resizeOffset.y};
        auto box = resizedBox(resize->box, resize->anchor, target, keepRatio);
        *found = transformed(resize->original, resize->box, box);
        return;
    }

    if (drag) {
        double distance = std::hypot(point.x - drag->start.x, point.y - drag->start.y);
        if (!drag->isActive && distance >= DRAG_DISTANCE) drag->isActive = true;
        if (!drag->isActive) return;

        auto found = std::find_if(shapes.begin(), shapes.end(), [&](Shape const& item) { return item.id == drag->id; });
        if (found == shapes.end()) return;

        *found = drag->original;
        found->x += point.x - drag->start.x;
        found->y += point.y - drag->start.y;
        return;
    }

    if (draft) updateDraft(point, shift);
}

void Annotator::pointerUp(Point point) {
    if (resize) {
        auto next = shapes;
        for (auto& shape : shapes)
            if (shape.id == resize->id) shape = resize->original;

        bool isChanged = next != shapes;
        resize.reset();
        if (isChanged) commit(std::move(next));
        changed();
        return;
    }

    if (drag) {
        bool isActive = drag->isActive;
        auto next = shapes;
        for (auto& shape : shapes)
            if (shape.id == drag->id) shape = drag->original;

        drag.reset();
        if (isActive && next != shapes) commit(std::move(next));
        changed();
    }

    if (draft) finishDraft();

    if (!pendingDoubleClick.empty()) {
        auto id = std::exchange(pendingDoubleClick, "");
        auto index = shapeAt(point);
        if (index && shapes[*index].id == id && shapes[*index].type == Tool::Text) editText(shapes[*index], false);
    }
}

void Annotator::editText(Shape const& shape, bool isNew) {
    finishTextEdit();

    Editor value;
    value.shape = shape;
    value.isNew = isNew;
    value.text = shape.text;
    value.caret = value.anchor = value.text.size();
    value.blinkStart = nowMs();
    editor = std::move(value);
    changed();
}

void Annotator::finishTextEdit() {
    if (!editor) return;

    auto value = std::move(*editor);
    editor.reset();

    auto text = value.text;
    while (!text.empty() && std::iswspace(text.back())) text.pop_back();

    if (text.empty()) {
        if (!value.isNew) {
            std::vector<Shape> next;
            for (auto const& shape : shapes)
                if (shape.id != value.shape.id) next.push_back(shape);
            commit(std::move(next));
        }
        changed();
        return;
    }

    auto next = value.shape;
    next.text = text;

    if (value.isNew) {
        auto list = shapes;
        list.push_back(next);
        commit(std::move(list));
    } else {
        replaceShape(next);
    }

    select(next.id);
}

Box Annotator::textBox(Shape const& shape) {
    double padding = shape.background ? std::round(shape.size * 0.35) : 0;
    auto layout = textLayout(shape, shape.text.empty() ? L" " : shape.text);

    DWRITE_TEXT_METRICS metrics{};
    layout->GetMetrics(&metrics);

    double width = metrics.widthIncludingTrailingWhitespace + padding * 2;
    double height = shape.size * 1.2 * metrics.lineCount + padding * 2;
    return {shape.x, shape.y, width * shape.scaleX, height * shape.scaleY};
}

Box Annotator::emojiBox(Shape const& shape) {
    double width = res.measure(shape.text, static_cast<float>(shape.size), DWRITE_FONT_WEIGHT_NORMAL, FontKind::Emoji);
    return {shape.x, shape.y, width * shape.scaleX, shape.size * shape.scaleY};
}

com_ptr<IDWriteTextLayout4> Annotator::textLayout(Shape const& shape, std::wstring const& text, float scale) {
    float size = static_cast<float>(shape.size) * scale;
    return res.layout(text, size, DWRITE_FONT_WEIGHT_BOLD, FontKind::Sans, size * 1.2f);
}

Box Annotator::localBox(Shape const& shape) {
    if (isLineShape(shape.type)) {
        auto geometry = lineGeometry(shape);
        D2D1_RECT_F bounds{};
        geometry->GetBounds(nullptr, &bounds);

        double minX = bounds.left;
        double minY = bounds.top;
        double maxX = bounds.right;
        double maxY = bounds.bottom;

        if (shape.type == Tool::Arrow && shape.points.size() >= 4) {
            double angle = pointerAngle(shape.points);
            double length = arrowPointer(shape);
            double cosine = std::cos(angle);
            double sine = std::sin(angle);
            size_t n = shape.points.size();
            double x = shape.points[n - 2] - length * cosine;
            double y = shape.points[n - 1] - length * sine;
            double dx = std::abs(length * sine / 2);
            double dy = std::abs(length * cosine / 2);
            minX = std::min(minX, x - dx);
            minY = std::min(minY, y - dy);
            maxX = std::max(maxX, x + dx);
            maxY = std::max(maxY, y + dy);
        }

        return {minX, minY, maxX - minX, maxY - minY};
    }

    if (shape.type == Tool::Text) {
        auto box = textBox(shape);
        return {0, 0, box.width / shape.scaleX, box.height / shape.scaleY};
    }

    if (shape.type == Tool::Emoji) {
        auto box = emojiBox(shape);
        return {0, 0, box.width / shape.scaleX, box.height / shape.scaleY};
    }

    if (shape.type == Tool::Step) return {-shape.size, -shape.size, shape.size * 2, shape.size * 2};
    if (shape.type == Tool::Ellipse) return {-shape.width / 2, -shape.height / 2, shape.width, shape.height};
    return {0, 0, shape.width, shape.height};
}

Box Annotator::nodeBox(Shape const& shape) {
    auto local = localBox(shape);
    double originX = shape.type == Tool::Ellipse ? shape.x + shape.width / 2 : shape.x;
    double originY = shape.type == Tool::Ellipse ? shape.y + shape.height / 2 : shape.y;

    return {originX + local.x * shape.scaleX, originY + local.y * shape.scaleY, local.width * shape.scaleX,
            local.height * shape.scaleY};
}

Shape Annotator::transformed(Shape const& shape, Box const& from, Box const& to) {
    double kx = from.width ? to.width / from.width : 1;
    double ky = from.height ? to.height / from.height : 1;
    auto local = localBox(shape);
    Shape next = shape;

    if (isBoxShape(shape.type)) {
        double width = std::max(MIN_SIZE, shape.width * kx);
        double height = std::max(MIN_SIZE, shape.height * ky);

        if (shape.type == Tool::Ellipse) {
            next.x = to.x + shape.width * kx / 2 - width / 2;
            next.y = to.y + shape.height * ky / 2 - height / 2;
        } else {
            next.x = to.x;
            next.y = to.y;
        }

        next.width = width;
        next.height = height;
        next.scaleX = next.scaleY = 1;
    } else if (isLineShape(shape.type)) {
        for (size_t index = 0; index < next.points.size(); index++) next.points[index] *= index % 2 ? ky : kx;
        next.x = to.x - local.x * kx;
        next.y = to.y - local.y * ky;
        next.scaleX = next.scaleY = 1;
    } else {
        next.scaleX = shape.scaleX * kx;
        next.scaleY = shape.scaleY * ky;
        next.x = to.x - local.x * next.scaleX;
        next.y = to.y - local.y * next.scaleY;
    }

    return next;
}

Box Annotator::resizedBox(Box const& box, Anchor anchor, Point point, bool keepRatio) {
    double left = box.x;
    double top = box.y;
    double right = box.right();
    double bottom = box.bottom();

    bool isCorner = anchor == Anchor::TopLeft || anchor == Anchor::TopRight || anchor == Anchor::BottomLeft ||
                    anchor == Anchor::BottomRight;

    if (isCorner && keepRatio) {
        double ox = anchor == Anchor::TopLeft || anchor == Anchor::BottomLeft ? right : left;
        double oy = anchor == Anchor::TopLeft || anchor == Anchor::TopRight ? bottom : top;
        double signX = anchor == Anchor::TopLeft || anchor == Anchor::BottomLeft ? -1 : 1;
        double signY = anchor == Anchor::TopLeft || anchor == Anchor::TopRight ? -1 : 1;

        double hypotenuse = std::hypot(box.width, box.height);
        if (!hypotenuse) return box;

        double distance = std::hypot(point.x - ox, point.y - oy);
        bool crossedX = (point.x - ox) * signX < 0;
        bool crossedY = (point.y - oy) * signY < 0;
        if (crossedX || crossedY) return box;

        double width = distance * box.width / hypotenuse;
        double height = distance * box.height / hypotenuse;
        if (width < 1 || height < 1) return box;

        return {signX < 0 ? ox - width : ox, signY < 0 ? oy - height : oy, width, height};
    }

    if (anchor == Anchor::TopLeft || anchor == Anchor::Left || anchor == Anchor::BottomLeft) left = point.x;
    if (anchor == Anchor::TopRight || anchor == Anchor::Right || anchor == Anchor::BottomRight) right = point.x;
    if (anchor == Anchor::TopLeft || anchor == Anchor::Top || anchor == Anchor::TopRight) top = point.y;
    if (anchor == Anchor::BottomLeft || anchor == Anchor::Bottom || anchor == Anchor::BottomRight) bottom = point.y;

    if (right - left < 1 || bottom - top < 1) return box;
    return {left, top, right - left, bottom - top};
}

std::vector<Anchor> Annotator::anchors(Shape const& shape) const {
    if (keepsRatio(shape.type)) return {Anchor::TopLeft, Anchor::TopRight, Anchor::BottomLeft, Anchor::BottomRight};
    return {Anchor::TopLeft,     Anchor::Top,    Anchor::TopRight,   Anchor::Right,
            Anchor::BottomRight, Anchor::Bottom, Anchor::BottomLeft, Anchor::Left};
}

Point Annotator::anchorPoint(Box const& box, Anchor anchor) const {
    double cx = box.x + box.width / 2;
    double cy = box.y + box.height / 2;

    switch (anchor) {
    case Anchor::TopLeft:
        return {box.x, box.y};
    case Anchor::Top:
        return {cx, box.y};
    case Anchor::TopRight:
        return {box.right(), box.y};
    case Anchor::Right:
        return {box.right(), cy};
    case Anchor::BottomRight:
        return {box.right(), box.bottom()};
    case Anchor::Bottom:
        return {cx, box.bottom()};
    case Anchor::BottomLeft:
        return {box.x, box.bottom()};
    case Anchor::Left:
        return {box.x, cy};
    default:
        return {cx, cy};
    }
}

std::optional<Anchor> Annotator::anchorAt(Point point) {
    if (editor) return std::nullopt;

    auto shape = findShape(selectedId);
    if (!shape) return std::nullopt;

    auto box = nodeBox(*shape);
    double half = ANCHOR_SIZE / 2 + 0.5;

    for (auto anchor : anchors(*shape)) {
        auto center = anchorPoint(box, anchor);
        if (std::abs(point.x - center.x) <= half && std::abs(point.y - center.y) <= half) return anchor;
    }

    return std::nullopt;
}

bool Annotator::hitShape(Shape const& shape, Point point) {
    if (editor && editor->shape.id == shape.id) return false;

    double originX = shape.type == Tool::Ellipse ? shape.x + shape.width / 2 : shape.x;
    double originY = shape.type == Tool::Ellipse ? shape.y + shape.height / 2 : shape.y;
    auto local = point2((point.x - originX) / shape.scaleX, (point.y - originY) / shape.scaleY);
    auto factory = res.factory();
    BOOL contains = FALSE;

    switch (shape.type) {
    case Tool::Rectangle: {
        float radius = static_cast<float>(std::min({shape.size, 6.0, shape.width / 2, shape.height / 2}));
        com_ptr<ID2D1RoundedRectangleGeometry> geometry;
        factory->CreateRoundedRectangleGeometry(
            D2D1::RoundedRect(rect(0, 0, shape.width, shape.height), radius, radius), geometry.put());
        if (shape.fill) geometry->FillContainsPoint(local, nullptr, 0.25f, &contains);
        if (!contains)
            geometry->StrokeContainsPoint(local, static_cast<float>(shape.size), nullptr, nullptr, 0.25f, &contains);
        return contains;
    }
    case Tool::Ellipse: {
        com_ptr<ID2D1EllipseGeometry> geometry;
        factory->CreateEllipseGeometry(
            D2D1::Ellipse(D2D1::Point2F(0, 0), static_cast<float>(shape.width / 2), static_cast<float>(shape.height / 2)),
            geometry.put());
        if (shape.fill) geometry->FillContainsPoint(local, nullptr, 0.25f, &contains);
        if (!contains)
            geometry->StrokeContainsPoint(local, static_cast<float>(shape.size), nullptr, nullptr, 0.25f, &contains);
        return contains;
    }
    case Tool::Highlight:
    case Tool::Eraser:
    case Tool::Blur:
    case Tool::Pixelate:
        return local.x >= 0 && local.y >= 0 && local.x < shape.width && local.y < shape.height;
    case Tool::Arrow:
    case Tool::Line:
    case Tool::Freehand: {
        float width = static_cast<float>(std::max(12.0, shape.size));
        auto geometry = lineGeometry(shape);
        geometry->StrokeContainsPoint(local, width, res.roundStroke(), nullptr, 0.25f, &contains);
        if (!contains && shape.type == Tool::Arrow && shape.points.size() >= 4) {
            auto head = arrowHead(shape);
            head->FillContainsPoint(local, nullptr, 0.25f, &contains);
            if (!contains) head->StrokeContainsPoint(local, width, res.roundStroke(), nullptr, 0.25f, &contains);
        }
        return contains;
    }
    case Tool::Text: {
        auto box = localBox(shape);
        return local.x >= 0 && local.y >= 0 && local.x < box.width && local.y < box.height;
    }
    case Tool::Step: {
        double radius = shape.size + std::max(2.0, shape.size / 8) / 2;
        bool inSquare = std::abs(local.x) <= shape.size && std::abs(local.y) <= shape.size;
        return inSquare || std::hypot(local.x, local.y) <= radius;
    }
    case Tool::Emoji: {
        auto box = localBox(shape);
        return local.x >= 0 && local.y >= 0 && local.x < box.width && local.y < box.height;
    }
    default:
        return false;
    }
}

std::optional<size_t> Annotator::shapeAt(Point point) {
    for (size_t index = shapes.size(); index-- > 0;)
        if (hitShape(shapes[index], point)) return index;
    return std::nullopt;
}

bool Annotator::isOverShape(Point point) {
    return anchorAt(point).has_value() || shapeAt(point).has_value();
}

com_ptr<ID2D1PathGeometry> Annotator::lineGeometry(Shape const& shape) {
    com_ptr<ID2D1PathGeometry> geometry;
    winrt::check_hresult(res.factory()->CreatePathGeometry(geometry.put()));

    com_ptr<ID2D1GeometrySink> sink;
    winrt::check_hresult(geometry->Open(sink.put()));

    auto const& p = shape.points;
    if (p.size() >= 2) {
        sink->BeginFigure(point2(p[0], p[1]), D2D1_FIGURE_BEGIN_HOLLOW);

        if (shape.type == Tool::Freehand && p.size() > 4) {
            auto tp = expandPoints(p, 0.4);
            size_t length = tp.size();

            if (length >= 4) {
                sink->AddQuadraticBezier(D2D1::QuadraticBezierSegment(point2(tp[0], tp[1]), point2(tp[2], tp[3])));
                size_t n = 4;
                while (n + 2 < length) {
                    sink->AddBezier(D2D1::BezierSegment(point2(tp[n], tp[n + 1]), point2(tp[n + 2], tp[n + 3]),
                                                        point2(tp[n + 4], tp[n + 5])));
                    n += 6;
                }
                sink->AddQuadraticBezier(D2D1::QuadraticBezierSegment(point2(tp[length - 2], tp[length - 1]),
                                                                      point2(p[p.size() - 2], p[p.size() - 1])));
            }
        } else {
            for (size_t n = 2; n + 1 < p.size(); n += 2) sink->AddLine(point2(p[n], p[n + 1]));
        }

        sink->EndFigure(D2D1_FIGURE_END_OPEN);
    }

    winrt::check_hresult(sink->Close());
    return geometry;
}

com_ptr<ID2D1PathGeometry> Annotator::arrowHead(Shape const& shape) {
    com_ptr<ID2D1PathGeometry> geometry;
    winrt::check_hresult(res.factory()->CreatePathGeometry(geometry.put()));

    com_ptr<ID2D1GeometrySink> sink;
    winrt::check_hresult(geometry->Open(sink.put()));

    size_t n = shape.points.size();
    double angle = pointerAngle(shape.points);
    double length = arrowPointer(shape);
    double cosine = std::cos(angle);
    double sine = std::sin(angle);
    double ex = shape.points[n - 2];
    double ey = shape.points[n - 1];

    auto rotate = [&](double x, double y) { return point2(ex + x * cosine - y * sine, ey + x * sine + y * cosine); };

    sink->BeginFigure(rotate(0, 0), D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLine(rotate(-length, length / 2));
    sink->AddLine(rotate(-length, -length / 2));
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);

    winrt::check_hresult(sink->Close());
    return geometry;
}

void Annotator::drawEffect(ID2D1DeviceContext* context, Shape const& shape) {
    if (shape.width < 1 || shape.height < 1 || !source.read) return;

    double scale = source.scale;
    RECT crop{std::lround(shape.x * scale), std::lround(shape.y * scale), 0, 0};
    crop.right = crop.left + std::max(1L, std::lround(shape.width * scale));
    crop.bottom = crop.top + std::max(1L, std::lround(shape.height * scale));

    int strength = shape.type == Tool::Blur
                       ? static_cast<int>(std::lround(std::min(shape.size * scale, 180.0) * scale))
                       : static_cast<int>(std::ceil(std::max(2L, std::lround(shape.size * scale)) * scale));

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%d:%ld:%ld:%ld:%ld:%d", static_cast<int>(shape.type), crop.left, crop.top,
             crop.right, crop.bottom, strength);

    auto found = std::find_if(effects.begin(), effects.end(), [&](auto const& item) { return item.first == shape.id; });
    if (found == effects.end()) {
        effects.push_back({shape.id, {}});
        found = effects.end() - 1;
    }

    auto& cache = found->second;
    if (cache.key != buffer || !cache.bitmap) {
        int width = crop.right - crop.left;
        int height = crop.bottom - crop.top;
        auto pixels = source.read(crop);

        if (shape.type == Tool::Blur) {
            stackBlur(pixels.data(), width, height, strength);
        } else {
            pixelate(pixels.data(), width, height, strength);
        }

        auto properties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_NONE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        cache.bitmap = nullptr;
        winrt::check_hresult(context->CreateBitmap(D2D1::SizeU(width, height), pixels.data(), width * 4, properties,
                                                   cache.bitmap.put()));
        cache.key = buffer;
        cache.crop = crop;
    }

    context->DrawBitmap(cache.bitmap.get(), rect(0, 0, shape.width, shape.height), 1.0f,
                        D2D1_INTERPOLATION_MODE_LINEAR, nullptr, nullptr);
}

void Annotator::drawShapeBody(ID2D1DeviceContext* context, Painter& painter, Shape const& shape) {
    auto tint = shape.color.empty() ? rgba(0, 0, 0) : parseHex(shape.color);
    float size = static_cast<float>(shape.size);

    switch (shape.type) {
    case Tool::Rectangle: {
        auto bounds = rect(0, 0, shape.width, shape.height);
        float radius = std::min(size, 6.0f);
        if (shape.fill) painter.fillRounded(bounds, radius, tint);
        painter.strokeRounded(bounds, radius, tint, size);
        break;
    }
    case Tool::Ellipse: {
        float rx = static_cast<float>(shape.width / 2);
        float ry = static_cast<float>(shape.height / 2);
        if (shape.fill) painter.fillEllipse(rx, ry, rx, ry, tint);
        painter.strokeEllipse(rx, ry, rx, ry, tint, size);
        break;
    }
    case Tool::Highlight:
        painter.fillRect(rect(0, 0, shape.width, shape.height), withAlpha(tint, 0.4f));
        break;
    case Tool::Eraser:
        painter.fillRect(rect(0, 0, shape.width, shape.height), tint);
        break;
    case Tool::Blur:
    case Tool::Pixelate:
        drawEffect(context, shape);
        break;
    case Tool::Arrow:
    case Tool::Line:
    case Tool::Freehand: {
        auto geometry = lineGeometry(shape);
        context->DrawGeometry(geometry.get(), painter.brush(tint), size, res.roundStroke());
        if (shape.type == Tool::Arrow && shape.points.size() >= 4) {
            auto head = arrowHead(shape);
            context->FillGeometry(head.get(), painter.brush(tint));
            context->DrawGeometry(head.get(), painter.brush(tint), size, res.roundStroke());
        }
        break;
    }
    case Tool::Text: {
        double padding = shape.background ? std::round(shape.size * 0.35) : 0;
        auto box = localBox(shape);
        if (shape.background)
            painter.fillRounded(rect(0, 0, box.width, box.height), static_cast<float>(padding), tint);

        auto layout = textLayout(shape, shape.text.empty() ? L" " : shape.text);
        auto fill = shape.background ? parseHex(contrastColor(shape.color)) : tint;
        painter.text(layout.get(), static_cast<float>(padding), static_cast<float>(padding), fill);
        break;
    }
    case Tool::Step: {
        painter.fillEllipse(0, 0, size, size, tint);
        painter.strokeEllipse(0, 0, size, size, rgba(255, 255, 255), std::max(2.0f, size / 8));

        auto label = std::to_wstring(shape.value);
        auto layout = res.layout(label, size, DWRITE_FONT_WEIGHT_BOLD, FontKind::Sans, size);
        DWRITE_TEXT_METRICS metrics{};
        layout->GetMetrics(&metrics);
        painter.text(layout.get(), -metrics.widthIncludingTrailingWhitespace / 2, -size / 2,
                     parseHex(contrastColor(shape.color)));
        break;
    }
    case Tool::Emoji: {
        auto layout = res.layout(shape.text, size, DWRITE_FONT_WEIGHT_NORMAL, FontKind::Emoji, size);
        painter.text(layout.get(), 0, 0, rgba(0, 0, 0));
        break;
    }
    default:
        break;
    }
}

void Annotator::drawShape(ID2D1DeviceContext* context, Painter& painter, Shape const& shape) {
    Transform node(context, D2D1::Matrix3x2F::Scale(static_cast<float>(shape.scaleX), static_cast<float>(shape.scaleY)) *
                                D2D1::Matrix3x2F::Translation(static_cast<float>(shape.x), static_cast<float>(shape.y)));

    if (!drawsShadow(shape)) {
        drawShapeBody(context, painter, shape);
        return;
    }

    com_ptr<ID2D1CommandList> list;
    winrt::check_hresult(context->CreateCommandList(list.put()));

    com_ptr<ID2D1Image> previous;
    context->GetTarget(previous.put());
    context->SetTarget(list.get());
    drawShapeBody(context, painter, shape);
    context->SetTarget(previous.get());
    list->Close();

    D2D1_MATRIX_3X2_F matrix{};
    context->GetTransform(&matrix);
    float scaleX = std::abs(matrix._11);
    float scaleY = std::abs(matrix._22);

    com_ptr<ID2D1Effect> shadow;
    winrt::check_hresult(context->CreateEffect(SHADOW_EFFECT, shadow.put()));
    shadow->SetInput(0, list.get());
    shadow->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, 3.0f * std::min(scaleX, scaleY));
    shadow->SetValue(D2D1_SHADOW_PROP_COLOR, D2D1::Vector4F(0, 0, 0, 0.45f));

    context->SetTransform(D2D1::Matrix3x2F::Identity());
    context->DrawImage(shadow.get(), D2D1::Point2F(scaleX, scaleY));
    context->DrawImage(list.get());
    context->SetTransform(matrix);
}

void Annotator::drawTransformer(Painter& painter, Shape const& shape) {
    auto box = nodeBox(shape);
    auto primary = color(theme.primary);
    painter.context()->DrawRectangle(box.rect(), painter.brush(primary), 1.0f);

    for (auto anchor : anchors(shape)) {
        auto center = anchorPoint(box, anchor);
        auto bounds = rect(center.x - ANCHOR_SIZE / 2, center.y - ANCHOR_SIZE / 2, ANCHOR_SIZE, ANCHOR_SIZE);
        painter.fillRounded(bounds, 2, rgba(255, 255, 255));
        painter.strokeRounded(bounds, 2, primary, 1);
    }
}

void Annotator::render(ID2D1DeviceContext* context, bool forExport) {
    Painter painter(context, res);

    std::vector<std::pair<std::string, EffectCache>> used;
    for (auto const& shape : shapes) {
        if (editor && editor->shape.id == shape.id) continue;
        drawShape(context, painter, shape);
    }

    if (draft) drawShape(context, painter, draft->shape);

    for (auto& entry : effects) {
        bool isUsed = std::any_of(shapes.begin(), shapes.end(), [&](Shape const& shape) { return shape.id == entry.first; }) ||
                      (draft && draft->shape.id == entry.first);
        if (isUsed) used.push_back(std::move(entry));
    }
    effects = std::move(used);

    if (forExport) return;

    if (auto selected = findShape(selectedId); selected && !editor) drawTransformer(painter, *selected);
    if (editor) drawEditor(painter);
}

void Annotator::releaseDeviceResources() {
    effects.clear();
}

std::wstring Annotator::editorText() const {
    if (!editor) return {};
    auto text = editor->text;
    text.insert(editor->caret, editor->composition);
    return text;
}

void Annotator::editorLayout(com_ptr<IDWriteTextLayout4>& layout, Box& box, float& padding, float& scale) {
    auto const& shape = editor->shape;
    scale = static_cast<float>(shape.scaleX);
    padding = shape.background ? static_cast<float>(std::round(shape.size * 0.35)) * scale : 0;

    auto text = editorText();
    layout = textLayout(shape, text, scale);

    DWRITE_TEXT_METRICS metrics{};
    layout->GetMetrics(&metrics);

    double fontSize = shape.size * scale;
    double width = std::max(fontSize * 2, static_cast<double>(metrics.widthIncludingTrailingWhitespace + padding * 2 + 4));
    double height = fontSize * 1.2 * metrics.lineCount + padding * 2 + 2;
    box = {shape.x, shape.y, width, height};
}

void Annotator::drawEditor(Painter& painter) {
    com_ptr<IDWriteTextLayout4> layout;
    Box box;
    float padding = 0;
    float scale = 1;
    editorLayout(layout, box, padding, scale);

    auto const& shape = editor->shape;
    auto tint = parseHex(shape.color);
    auto textColor = shape.background ? parseHex(contrastColor(shape.color)) : tint;
    float radius = padding;

    if (shape.background) painter.fillRounded(box.rect(), radius, tint);

    auto inner = D2D1::RectF(static_cast<float>(box.x) + 0.5f, static_cast<float>(box.y) + 0.5f,
                             static_cast<float>(box.right()) - 0.5f, static_cast<float>(box.bottom()) - 0.5f);
    painter.strokeRounded(inner, radius, textColor, 1, res.dashStroke(3, 3));

    float originX = static_cast<float>(box.x) + 1 + padding;
    float originY = static_cast<float>(box.y) + 1 + padding;

    if (editor->caret != editor->anchor && editor->composition.empty()) {
        size_t start = std::min(editor->caret, editor->anchor);
        size_t length = std::max(editor->caret, editor->anchor) - start;
        UINT32 count = 0;
        layout->HitTestTextRange(static_cast<UINT32>(start), static_cast<UINT32>(length), 0, 0, nullptr, 0, &count);
        std::vector<DWRITE_HIT_TEST_METRICS> ranges(count);
        layout->HitTestTextRange(static_cast<UINT32>(start), static_cast<UINT32>(length), 0, 0, ranges.data(), count,
                                 &count);
        for (auto const& range : ranges)
            painter.fillRect(D2D1::RectF(originX + range.left, originY + range.top, originX + range.left + range.width,
                                         originY + range.top + range.height),
                             rgba(51, 144, 255, 0.4f));
    }

    painter.text(layout.get(), originX, originY, textColor);

    if (!editor->composition.empty()) {
        DWRITE_HIT_TEST_METRICS hit{};
        float x = 0;
        float y = 0;
        layout->HitTestTextPosition(static_cast<UINT32>(editor->caret), FALSE, &x, &y, &hit);
        float endX = 0;
        float endY = 0;
        layout->HitTestTextPosition(static_cast<UINT32>(editor->caret + editor->composition.size()), FALSE, &endX,
                                    &endY, &hit);
        float underline = originY + y + hit.height - 2 * scale;
        painter.fillRect(D2D1::RectF(originX + x, underline, originX + endX, underline + std::max(1.0f, scale)),
                         textColor);
    }

    if (caretVisible()) {
        float height = 0;
        auto caret = editorCaret(height);
        painter.fillRect(D2D1::RectF(static_cast<float>(caret.x), static_cast<float>(caret.y),
                                     static_cast<float>(caret.x) + 1, static_cast<float>(caret.y) + height),
                         textColor);
    }
}

Point Annotator::editorCaret(float& height) {
    com_ptr<IDWriteTextLayout4> layout;
    Box box;
    float padding = 0;
    float scale = 1;
    editorLayout(layout, box, padding, scale);

    float x = 0;
    float y = 0;
    DWRITE_HIT_TEST_METRICS hit{};
    layout->HitTestTextPosition(static_cast<UINT32>(editor->caret + editor->composition.size()), FALSE, &x, &y, &hit);

    auto metrics = res.metrics(FontKind::Sans);
    float fontSize = static_cast<float>(editor->shape.size) * scale;
    height = (metrics.ascent + metrics.descent) * fontSize;
    float lineHeight = fontSize * 1.2f;

    return {box.x + 1 + padding + x, box.y + 1 + padding + y + (lineHeight - height) / 2};
}

bool Annotator::caretVisible() const {
    if (!editor) return false;
    double blink = GetCaretBlinkTime();
    if (blink <= 0 || blink >= INFINITE) return true;
    return std::fmod(nowMs() - editor->blinkStart, blink * 2) < blink;
}

bool Annotator::editorContains(Point point) {
    if (!editor) return false;

    com_ptr<IDWriteTextLayout4> layout;
    Box box;
    float padding = 0;
    float scale = 1;
    editorLayout(layout, box, padding, scale);
    return box.contains(point.x, point.y);
}

size_t Annotator::editorIndexAt(Point point) {
    com_ptr<IDWriteTextLayout4> layout;
    Box box;
    float padding = 0;
    float scale = 1;
    editorLayout(layout, box, padding, scale);

    BOOL trailing = FALSE;
    BOOL inside = FALSE;
    DWRITE_HIT_TEST_METRICS hit{};
    layout->HitTestPoint(static_cast<float>(point.x - box.x - 1 - padding),
                         static_cast<float>(point.y - box.y - 1 - padding), &trailing, &inside, &hit);
    return std::min(editor->text.size(), static_cast<size_t>(hit.textPosition + (trailing ? hit.length : 0)));
}

void Annotator::editorClick(Point point, bool shift) {
    if (!editor || !editor->composition.empty()) return;
    editorMove(editorIndexAt(point), shift);
}

void Annotator::editorMove(size_t position, bool extend) {
    auto& text = editor->text;
    position = std::min(position, text.size());
    if (position > 0 && position < text.size() && IS_LOW_SURROGATE(text[position]) &&
        IS_HIGH_SURROGATE(text[position - 1]))
        position += position > editor->caret ? 1 : -1;

    editor->caret = position;
    if (!extend) editor->anchor = position;
    editor->blinkStart = nowMs();
}

void Annotator::editorInsert(std::wstring const& value) {
    auto& text = editor->text;
    size_t start = std::min(editor->caret, editor->anchor);
    size_t end = std::max(editor->caret, editor->anchor);
    text.replace(start, end - start, value);
    editor->caret = editor->anchor = start + value.size();
    editor->blinkStart = nowMs();
}

void Annotator::editorErase(bool forward) {
    auto& text = editor->text;
    if (editor->caret != editor->anchor) {
        editorInsert(L"");
        return;
    }

    size_t caret = editor->caret;
    if (forward) {
        if (caret >= text.size()) return;
        size_t length = caret + 1 < text.size() && IS_HIGH_SURROGATE(text[caret]) ? 2 : 1;
        text.erase(caret, length);
    } else {
        if (caret == 0) return;
        size_t length = caret >= 2 && IS_LOW_SURROGATE(text[caret - 1]) && IS_HIGH_SURROGATE(text[caret - 2]) ? 2 : 1;
        text.erase(caret - length, length);
        editor->caret = editor->anchor = caret - length;
    }
    editor->blinkStart = nowMs();
}

bool Annotator::editorKey(UINT key, bool ctrl, bool shift) {
    if (!editor) return false;
    if (!editor->composition.empty()) return true;

    auto& text = editor->text;

    switch (key) {
    case VK_ESCAPE:
        finishTextEdit();
        return true;
    case VK_RETURN:
        if (shift) {
            editorInsert(L"\n");
        } else {
            finishTextEdit();
        }
        return true;
    case VK_BACK:
        editorErase(false);
        return true;
    case VK_DELETE:
        editorErase(true);
        return true;
    case VK_LEFT:
        if (!shift && editor->caret != editor->anchor) {
            editorMove(std::min(editor->caret, editor->anchor), false);
        } else if (editor->caret > 0) {
            editorMove(editor->caret - 1, shift);
        }
        return true;
    case VK_RIGHT:
        if (!shift && editor->caret != editor->anchor) {
            editorMove(std::max(editor->caret, editor->anchor), false);
        } else {
            editorMove(editor->caret + 1, shift);
        }
        return true;
    case VK_HOME: {
        size_t start = editor->caret == 0 ? 0 : text.rfind(L'\n', editor->caret - 1);
        editorMove(start == std::wstring::npos ? 0 : (editor->caret == 0 ? 0 : start + 1), shift);
        return true;
    }
    case VK_END: {
        size_t end = text.find(L'\n', editor->caret);
        editorMove(end == std::wstring::npos ? text.size() : end, shift);
        return true;
    }
    case VK_UP:
    case VK_DOWN: {
        float height = 0;
        auto caret = editorCaret(height);
        double lineHeight = editor->shape.size * editor->shape.scaleX * 1.2;
        Point target{caret.x, caret.y + height / 2 + (key == VK_UP ? -lineHeight : lineHeight)};
        editorMove(editorIndexAt(target), shift);
        return true;
    }
    default:
        break;
    }

    if (ctrl && key == 'A') {
        editor->anchor = 0;
        editor->caret = text.size();
        return true;
    }

    if (ctrl && (key == 'C' || key == 'X') && editor->caret != editor->anchor) {
        size_t start = std::min(editor->caret, editor->anchor);
        auto selected = text.substr(start, std::max(editor->caret, editor->anchor) - start);

        if (OpenClipboard(nullptr)) {
            EmptyClipboard();
            size_t bytes = (selected.size() + 1) * sizeof(wchar_t);
            if (HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes)) {
                memcpy(GlobalLock(memory), selected.c_str(), bytes);
                GlobalUnlock(memory);
                SetClipboardData(CF_UNICODETEXT, memory);
            }
            CloseClipboard();
        }

        if (key == 'X') editorInsert(L"");
        return true;
    }

    if (ctrl && key == 'V') {
        if (OpenClipboard(nullptr)) {
            if (HANDLE data = GetClipboardData(CF_UNICODETEXT)) {
                std::wstring pasted(static_cast<wchar_t const*>(GlobalLock(data)));
                GlobalUnlock(data);
                pasted.erase(std::remove(pasted.begin(), pasted.end(), L'\r'), pasted.end());
                editorInsert(pasted);
            }
            CloseClipboard();
        }
        return true;
    }

    return true;
}

void Annotator::editorChar(wchar_t character) {
    if (!editor || character < 32 || character == 127) return;
    editorInsert(std::wstring(1, character));
}

void Annotator::editorComposition(std::wstring const& text) {
    if (!editor) return;
    if (editor->caret != editor->anchor) editorInsert(L"");
    editor->composition = text;
    editor->blinkStart = nowMs();
}

void Annotator::editorCommit(std::wstring const& text) {
    if (!editor) return;
    editor->composition.clear();
    editorInsert(text);
}

}

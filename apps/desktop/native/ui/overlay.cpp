#include "overlayController.h"

#include "fixture.h"
#include "uiThread.h"

#include <shared/log.h>
#include <shared/system.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cwctype>

namespace fileship::ui {

namespace {

constexpr double DRAG_THRESHOLD = 3;
constexpr double MAGNIFIER_PIXELS = 15;
constexpr double MAGNIFIER_ZOOM = 8;
constexpr double HOVER_ANIMATION_MS = 140;
constexpr double TOOLBAR_MARGIN = 16;
constexpr double TOOLBAR_EDGE = 96;
constexpr double TOOLBAR_EDGE_Y = 64;
constexpr double TOOLBAR_MAGNET = 72;

std::atomic<bool> isOpen{false};
OverlayController* active = nullptr;

double jsRound(double value) {
    return std::floor(value + 0.5);
}

bool isSameBox(std::optional<Box> const& a, std::optional<Box> const& b) {
    if (!a || !b) return !a && !b;
    return a->x == b->x && a->y == b->y && a->width == b->width && a->height == b->height;
}

std::optional<Box> intersect(Box const& rect, Box const& bounds) {
    double x = std::max(rect.x, bounds.x);
    double y = std::max(rect.y, bounds.y);
    double right = std::min(rect.right(), bounds.right());
    double bottom = std::min(rect.bottom(), bounds.bottom());
    if (right > x && bottom > y) return Box{x, y, right - x, bottom - y};
    return std::nullopt;
}

bool covers(Box const& rect, Box const& inner) {
    return rect.x <= inner.x && rect.y <= inner.y && rect.right() >= inner.right() && rect.bottom() >= inner.bottom();
}

Box fromPoints(Point a, Point b) {
    return {std::min(a.x, b.x), std::min(a.y, b.y), std::abs(a.x - b.x), std::abs(a.y - b.y)};
}

RectF rectOf(double x, double y, double width, double height) {
    return RectF(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x + width),
                 static_cast<float>(y + height));
}

bool sameOptions(AnnotationOptions const& a, AnnotationOptions const& b) {
    if (a.fill != b.fill || a.background != b.background || a.shadow != b.shadow || a.emoji != b.emoji) return false;
    for (size_t index = 0; index < TOOL_COUNT; index++)
        if (a.tools[index].color != b.tools[index].color || a.tools[index].size != b.tools[index].size) return false;
    return true;
}

}

OverlayController::OverlayController(std::shared_ptr<OverlayRequest> value, OverlayCallback callback)
    : request(std::move(value)), done(std::move(callback)) {
    shared = Resources::acquire();
    palette = {request->theme == ThemeMode::Dark ? DARK_THEME : LIGHT_THEME, request->theme == ThemeMode::Dark};
    bounds = request->bounds;

    ratio = 1;
    for (auto const& display : request->displays)
        if (display.isPrimary) ratio = display.scaleFactor;
    if (ratio <= 0) ratio = 1;

    for (auto const& display : request->displays) {
        Display item;
        item.id = display.id;
        item.physical = display.rect;
        item.local = toLocal(display.rect);
        item.scaleFactor = display.scaleFactor;
        item.isPrimary = display.isPrimary;
        displays.push_back(std::move(item));
    }

    for (auto const& item : request->windows) {
        Target target;
        target.rect = toLocal(item.rect);
        for (auto const& control : item.controls) target.controls.push_back(toLocal(control));
        windows.push_back(std::move(target));
    }

    preferences = loadPreferences();
    loaded = preferences;

    annotator = std::make_unique<Annotator>(*shared, palette.theme);
    annotator->options = preferences.annotations;
    annotator->tool.reset();
    annotator->reset();
    annotator->copyText = [this](std::wstring const& text) {
        if (window) window->copyText(text);
    };
    annotator->pasteText = [this] { return window ? window->pasteText() : std::wstring(); };

    toolbar = std::make_unique<AnnotationToolbar>(*shared, *annotator, palette);
    toolbar->onCancel = [this] { finish({}); };
    toolbar->onDragStart = [this](Point point) { startToolbarDrag(point); };
    toolbar->chooseColor = [this](std::string const& current) {
        return window ? window->pickColor(current) : std::nullopt;
    };

    auto cursor = toLocal({request->cursor.x, request->cursor.y, 0, 0});
    pointer = Point{cursor.x, cursor.y};
    hover = targetAt(*pointer);
}

OverlayController::~OverlayController() {
    if (window) window->destroy();
    window.reset();
    for (auto& display : displays) display.image = nullptr;
    if (annotator) annotator->releaseDeviceResources();
    gpu.reset();
}

void OverlayController::initializeDevice() {
    gpu = Device::create(*shared);

    for (size_t index = 0; index < displays.size(); index++) {
        auto& source = request->displays[index];
        auto& display = displays[index];
        if (source.bgra.size() < static_cast<size_t>(source.rect.width) * source.rect.height * 4) continue;

        display.image = gpu->createImage(source.bgra.data(), source.rect.width, source.rect.height, true);
        source.bgra.clear();
        source.bgra.shrink_to_fit();
    }

    annotator->setSource({ratio, [this](PixelRect const& rect) { return readSource(rect); }});
}

bool OverlayController::open() {
    initializeDevice();
    window = createOverlayWindow(*this);
    return window && window->open();
}

void OverlayController::close() {
    finish({});
}

void OverlayController::finish(OverlayResult result) {
    if (isFinishing) return;
    isFinishing = true;

    if (preferences.toolbar.display != loaded.toolbar.display || preferences.toolbar.edge != loaded.toolbar.edge ||
        preferences.toolbar.cx != loaded.toolbar.cx || preferences.toolbar.cy != loaded.toolbar.cy ||
        !sameOptions(annotator->options, loaded.annotations)) {
        preferences.annotations = annotator->options;
        savePreferences(preferences);
    }

    if (window) window->destroy();
    if (active == this) active = nullptr;

    auto callback = std::move(done);
    auto payload = std::make_shared<OverlayResult>(std::move(result));
    runOnUi([this, callback, payload] {
        delete this;
        isOpen = false;
        if (callback) callback(std::move(*payload));
    });
}

std::vector<uint8_t> OverlayController::renderScene(FixtureScene const& scene) {
    applyScene(scene);

    auto canvas = gpu->createCanvas(bounds.width, bounds.height);
    render(canvas->painter());
    return canvas->finish();
}

OverlayResult OverlayController::renderRegion(FixtureScene const& scene, Box const& region) {
    applyScene(scene);
    return exportRegion(region);
}

std::vector<ScreenRect> OverlayController::displayRects() const {
    std::vector<ScreenRect> result;
    for (auto const& display : displays) result.push_back(display.physical);
    return result;
}

void OverlayController::applyScene(FixtureScene const& scene) {
    annotator->shapes = scene.shapes;
    annotator->tool = scene.tool;
    annotator->selectedId = scene.selectedId;
    pointer = scene.pointer;
    hover = pointer ? std::optional<Box>(targetAt(*pointer)) : std::nullopt;
    hoverAnimation.reset();

    if (scene.selection) {
        selection = scene.selection;
        mode = Mode::Drawing;
    }

    if (scene.popover >= 0) {
        layoutToolbar();
        toolbar->openPopover(scene.popover, nowMs() - 10000);
    }
}

Box OverlayController::toLocal(ScreenRect const& rect) const {
    return {(rect.x - bounds.x) / ratio, (rect.y - bounds.y) / ratio, rect.width / ratio, rect.height / ratio};
}

Box OverlayController::viewport() const {
    return {0, 0, bounds.width / ratio, bounds.height / ratio};
}

ScreenRect OverlayController::toPhysical(Box const& rect) const {
    int x = static_cast<int>(jsRound(rect.x * ratio));
    int y = static_cast<int>(jsRound(rect.y * ratio));
    int width = std::max(1, static_cast<int>(jsRound((rect.x + rect.width) * ratio)) - x);
    int height = std::max(1, static_cast<int>(jsRound((rect.y + rect.height) * ratio)) - y);
    return {x, y, width, height};
}

ScreenRect OverlayController::toScreen(Box const& rect) const {
    auto physical = toPhysical(rect);
    return {physical.x + bounds.x, physical.y + bounds.y, physical.width, physical.height};
}

OverlayController::Display const* OverlayController::displayAt(Point point) const {
    for (auto const& display : displays)
        if (display.local.contains(point.x, point.y)) return &display;
    return nullptr;
}

Box OverlayController::targetAt(Point point) const {
    auto display = displayAt(point);
    auto host = display ? display->local : viewport();

    auto found = std::find_if(windows.begin(), windows.end(),
                              [&](Target const& item) { return item.rect.contains(point.x, point.y); });
    if (found == windows.end()) return host;

    Box const* control = nullptr;
    for (auto const& item : found->controls) {
        if (!item.contains(point.x, point.y)) continue;
        if (!control || item.width * item.height < control->width * control->height) control = &item;
    }

    auto rect = intersect(control ? *control : found->rect, viewport());
    return !rect || covers(*rect, host) ? host : *rect;
}

bool OverlayController::isRecording() const {
    return request->purpose == OverlayPurpose::Video || request->purpose == OverlayPurpose::Gif;
}

bool OverlayController::hasToolbar() const {
    return request->purpose == OverlayPurpose::Capture;
}

bool OverlayController::isAnnotating() const {
    return hasToolbar() && annotator->tool.has_value();
}

OverlayController::Display const* OverlayController::toolbarDisplay() const {
    auto id = toolbarDrag ? std::optional<int64_t>(toolbarDrag->display) : preferences.toolbar.display;
    for (auto const& display : displays)
        if (id && display.id == *id) return &display;
    for (auto const& display : displays)
        if (display.isPrimary) return &display;
    return displays.empty() ? nullptr : &displays.front();
}

Box OverlayController::toolbarHost() const {
    auto display = toolbarDisplay();
    return display ? display->local : viewport();
}

double OverlayController::toolbarZoom() const {
    auto display = toolbarDisplay();
    return (display ? display->scaleFactor : 1) / ratio;
}

ToolbarEdge OverlayController::edgeAt(Point point) const {
    auto host = toolbarHost();
    if (point.x < host.x + TOOLBAR_EDGE) return ToolbarEdge::Left;
    if (point.x > host.right() - TOOLBAR_EDGE) return ToolbarEdge::Right;
    if (point.y < host.y + TOOLBAR_EDGE_Y) return ToolbarEdge::Top;
    if (point.y > host.bottom() - TOOLBAR_EDGE_Y) return ToolbarEdge::Bottom;
    return ToolbarEdge::None;
}

ToolbarEdge OverlayController::currentEdge() const {
    return toolbarDrag ? edgeAt(toolbarDrag->pointer) : preferences.toolbar.edge;
}

Side OverlayController::toolbarSide() const {
    switch (currentEdge()) {
    case ToolbarEdge::Left:
        return Side::Right;
    case ToolbarEdge::Right:
        return Side::Left;
    case ToolbarEdge::Bottom:
        return Side::Top;
    default:
        return Side::Bottom;
    }
}

OverlayController::ToolbarLayout OverlayController::layoutToolbar() {
    toolbar->setSide(toolbarSide());
    auto size = toolbar->size();
    double zoom = toolbarZoom();
    double width = size.width * zoom;
    double height = size.height * zoom;
    auto host = toolbarHost();
    auto edge = currentEdge();

    Point center;
    if (toolbarDrag) {
        bool isOnEdge = edge != ToolbarEdge::None;
        center = {isOnEdge ? toolbarDrag->pointer.x : toolbarDrag->pointer.x - toolbarDrag->offset.x + width / 2,
                  isOnEdge ? toolbarDrag->pointer.y : toolbarDrag->pointer.y - toolbarDrag->offset.y + height / 2};
    } else {
        center = {host.x + preferences.toolbar.cx * host.width, host.y + preferences.toolbar.cy * host.height};
    }

    double minX = host.x + TOOLBAR_MARGIN;
    double minY = host.y + TOOLBAR_MARGIN;
    double maxX = host.right() - TOOLBAR_MARGIN;
    double maxY = host.bottom() - TOOLBAR_MARGIN;
    double x = center.x;
    double y = center.y;
    auto magnet = [](double value, double middle) {
        return std::abs(value - middle) < TOOLBAR_MAGNET ? middle : value;
    };

    if (edge == ToolbarEdge::Left) x = minX + width / 2;
    if (edge == ToolbarEdge::Right) x = maxX - width / 2;
    if (edge == ToolbarEdge::Top) y = minY + height / 2;
    if (edge == ToolbarEdge::Bottom) y = maxY - height / 2;
    if (edge == ToolbarEdge::Left || edge == ToolbarEdge::Right) y = magnet(y, host.y + host.height / 2);
    if (edge == ToolbarEdge::Top || edge == ToolbarEdge::Bottom) x = magnet(x, host.x + host.width / 2);

    double left = std::clamp(x - width / 2, minX, std::max(minX, maxX - width));
    double top = std::clamp(y - height / 2, minY, std::max(minY, maxY - height));

    auto view = viewport();
    toolbar->place(left, top, zoom, view.rect());
    return {left, top, {left + width / 2, top + height / 2}};
}

void OverlayController::startToolbarDrag(Point point) {
    auto layout = layoutToolbar();
    auto display = toolbarDisplay();
    toolbarDrag = ToolbarDrag{{point.x - layout.left, point.y - layout.top}, point, display ? display->id : 0};
}

void OverlayController::moveToolbarDrag(Point point) {
    auto display = displayAt(point);
    toolbarDrag->pointer = point;
    if (display) toolbarDrag->display = display->id;
}

void OverlayController::endToolbarDrag() {
    auto host = toolbarHost();
    auto layout = layoutToolbar();
    auto display = toolbarDisplay();

    preferences.toolbar.display = display ? std::optional<int64_t>(display->id) : std::nullopt;
    preferences.toolbar.edge = currentEdge();
    preferences.toolbar.cx = (layout.center.x - host.x) / host.width;
    preferences.toolbar.cy = (layout.center.y - host.y) / host.height;
    toolbarDrag.reset();
}

std::optional<Box> OverlayController::animatedHover() {
    if (!hoverAnimation || !hover) return hover;

    double progress = std::min(1.0, (nowMs() - hoverAnimation->second) / HOVER_ANIMATION_MS);
    double eased = 1 - std::pow(1 - progress, 3);
    auto const& from = hoverAnimation->first;
    auto mix = [&](double a, double b) { return a + (b - a) * eased; };

    if (progress >= 1) {
        hoverAnimation.reset();
        return hover;
    }

    isAnimatingHover = true;
    return Box{mix(from.x, hover->x), mix(from.y, hover->y), mix(from.width, hover->width),
               mix(from.height, hover->height)};
}

void OverlayController::setHover(std::optional<Box> next) {
    if (isSameBox(next, hover)) return;

    auto from = animatedHover();
    hover = next;
    if (from && next) {
        hoverAnimation = std::pair{*from, nowMs()};
    } else {
        hoverAnimation.reset();
    }
}

std::vector<uint8_t> OverlayController::readSource(PixelRect const& rect) {
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    std::vector<uint8_t> pixels(static_cast<size_t>(std::max(0, width)) * std::max(0, height) * 4, 0);
    for (size_t index = 3; index < pixels.size(); index += 4) pixels[index] = 255;
    if (width <= 0 || height <= 0) return pixels;

    for (auto const& display : displays) {
        if (!display.image) continue;

        int left = display.physical.x - bounds.x;
        int top = display.physical.y - bounds.y;
        int x0 = std::max(rect.left, left);
        int y0 = std::max(rect.top, top);
        int x1 = std::min(rect.right, left + display.physical.width);
        int y1 = std::min(rect.bottom, top + display.physical.height);
        if (x1 <= x0 || y1 <= y0) continue;

        auto part = gpu->readImage(*display.image, {x0 - left, y0 - top, x1 - left, y1 - top});
        int partWidth = x1 - x0;
        for (int row = 0; row < y1 - y0; row++) {
            std::copy_n(part.data() + static_cast<size_t>(row) * partWidth * 4, static_cast<size_t>(partWidth) * 4,
                        pixels.data() + ((static_cast<size_t>(row + y0 - rect.top) * width) + (x0 - rect.left)) * 4);
        }
    }

    for (size_t index = 3; index < pixels.size(); index += 4) pixels[index] = 255;
    return pixels;
}

void OverlayController::drawLabel(Painter& painter, std::wstring const& text, double x, double y) {
    float width = painter.resources().measure(text, 12, FontWeight::Medium) + 12;
    painter.fillRounded(rectOf(x, y, width, 20), 6, rgba(0, 0, 0, 0.75f));
    painter.textAt(text, static_cast<float>(x + 6), painter.middleBaseline(static_cast<float>(y + 10), 12), 12,
                   FontWeight::Medium, rgba(255, 255, 255));
}

void OverlayController::drawSourcePixels(Painter& painter, ScreenRect const& source, RectF const& destination) {
    double scaleX = (destination.right - destination.left) / source.width;
    double scaleY = (destination.bottom - destination.top) / source.height;

    for (auto const& display : displays) {
        if (!display.image) continue;

        int left = display.physical.x - bounds.x;
        int top = display.physical.y - bounds.y;
        int x0 = std::max(source.x, left);
        int y0 = std::max(source.y, top);
        int x1 = std::min(source.x + source.width, left + display.physical.width);
        int y1 = std::min(source.y + source.height, top + display.physical.height);
        if (x1 <= x0 || y1 <= y0) continue;

        auto target = RectF(destination.left + static_cast<float>((x0 - source.x) * scaleX),
                            destination.top + static_cast<float>((y0 - source.y) * scaleY),
                            destination.left + static_cast<float>((x1 - source.x) * scaleX),
                            destination.top + static_cast<float>((y1 - source.y) * scaleY));
        auto cut = RectF(static_cast<float>(x0 - left), static_cast<float>(y0 - top), static_cast<float>(x1 - left),
                         static_cast<float>(y1 - top));
        painter.image(*display.image, target, cut, Interpolation::Nearest);
    }
}

void OverlayController::drawMagnifier(Painter& painter, Point point) {
    auto display = displayAt(point);
    auto host = display ? display->local : viewport();
    double size = MAGNIFIER_PIXELS * MAGNIFIER_ZOOM;
    auto physical = toPhysical({point.x, point.y, 0, 0});
    int half = static_cast<int>(MAGNIFIER_PIXELS / 2);

    double x = point.x + 24;
    double y = point.y + 24;
    if (x + size > host.right()) x = point.x - size - 24;
    if (y + size + 28 > host.bottom()) y = point.y - size - 52;

    auto frame = rectOf(x, y, size, size);
    painter.pushClip(frame, 8);

    painter.fillRect(frame, rgba(0, 0, 0));
    drawSourcePixels(painter,
                     {physical.x - half, physical.y - half, static_cast<int>(MAGNIFIER_PIXELS),
                      static_cast<int>(MAGNIFIER_PIXELS)},
                     frame);

    auto grid = rgba(255, 255, 255, 0.12f);
    for (int index = 1; index < static_cast<int>(MAGNIFIER_PIXELS); index++) {
        float offset = static_cast<float>(index * MAGNIFIER_ZOOM) + 0.5f;
        painter.line(PointF(frame.left + offset, frame.top), PointF(frame.left + offset, frame.bottom), grid, 1);
        painter.line(PointF(frame.left, frame.top + offset), PointF(frame.right, frame.top + offset), grid, 1);
    }

    float center = static_cast<float>(half * MAGNIFIER_ZOOM);
    painter.strokeRect(RectF(frame.left + center, frame.top + center,
                             frame.left + center + static_cast<float>(MAGNIFIER_ZOOM),
                             frame.top + center + static_cast<float>(MAGNIFIER_ZOOM)),
                       color(palette.theme.primary), 2);
    painter.popClip();

    painter.strokeRounded(rectOf(x - 0.5, y - 0.5, size + 1, size + 1), 8, rgba(255, 255, 255, 0.8f), 1);

    auto label = std::to_wstring(physical.x + bounds.x) + L", " + std::to_wstring(physical.y + bounds.y);
    drawLabel(painter, label, x, y + size + 6);
}

void OverlayController::render(Painter& painter) {
    isAnimatingHover = false;

    painter.clear(Color(0, 0, 0, 1));
    for (auto const& display : displays) {
        if (!display.image) continue;
        float left = static_cast<float>(display.physical.x - bounds.x);
        float top = static_cast<float>(display.physical.y - bounds.y);
        painter.image(*display.image, RectF(left, top, left + display.physical.width, top + display.physical.height),
                      std::nullopt, Interpolation::Nearest);
    }

    auto scale = static_cast<float>(ratio);
    Transform local(painter, Matrix::scale(scale, scale));

    if (hasToolbar()) annotator->render(painter, false);

    if (!isAnnotating()) {
        auto view = viewport();
        auto region = mode == Mode::Drawing ? selection : animatedHover();
        auto dim = rgba(0, 0, 0, 0.45f);

        if (region) {
            Path outside;
            outside.fill = FillMode::Alternate;
            outside.addRect(view.rect());
            outside.addRect(region->rect());
            painter.fillPath(outside, dim);

            bool isDrawing = mode == Mode::Drawing;
            float width = isDrawing ? 2.0f : 1.5f;
            auto dashed = StrokeStyle::dashed(6 / width, 4 / width);
            painter.strokeRect(region->rect(), color(palette.theme.primary), width, isDrawing ? nullptr : &dashed);
        } else {
            painter.fillRect(view.rect(), dim);
        }

        if (mode == Mode::Drawing && selection) {
            auto physical = toPhysical(*selection);
            double labelY = selection->y >= 28 ? selection->y - 26 : selection->y + 6;
            drawLabel(painter, std::to_wstring(physical.width) + L" × " + std::to_wstring(physical.height),
                      selection->x, labelY);
        }

        if (request->showMagnifier && pointer) drawMagnifier(painter, *pointer);
    }

    if (hasToolbar()) {
        layoutToolbar();
        toolbar->render(painter, nowMs());
    }
}

bool OverlayController::isAnimating() {
    double now = nowMs();
    return isAnimatingHover || (hasToolbar() && toolbar->isAnimating(now));
}

OverlayResult OverlayController::exportRegion(Box const& rect) {
    auto screen = toScreen(rect);
    OverlayResult result;
    result.rect = screen;

    if (isRecording()) {
        result.type = OverlayResultType::Area;
        return result;
    }

    annotator->finishTextEdit();

    auto local = toLocal(screen);
    ScreenRect target{static_cast<int>(jsRound(local.x * ratio)), static_cast<int>(jsRound(local.y * ratio)),
                      std::max(1, static_cast<int>(jsRound(local.width * ratio))),
                      std::max(1, static_cast<int>(jsRound(local.height * ratio)))};

    auto canvas = gpu->createCanvas(target.width, target.height);
    auto& painter = canvas->painter();
    painter.clear(Color(0, 0, 0, 1));
    drawSourcePixels(painter, target, rectOf(0, 0, target.width, target.height));

    if (hasToolbar() && !annotator->shapes.empty()) {
        Transform transform(painter, Matrix::translation(static_cast<float>(-local.x), static_cast<float>(-local.y)) *
                                         Matrix::scale(static_cast<float>(ratio), static_cast<float>(ratio)));
        annotator->render(painter, true);
    }

    result.type = OverlayResultType::Region;
    result.width = target.width;
    result.height = target.height;
    result.bgra = canvas->finish();
    for (size_t index = 3; index < result.bgra.size(); index += 4) result.bgra[index] = 255;
    return result;
}

void OverlayController::finishRegion(Box const& rect) {
    OverlayResult result;
    try {
        result = exportRegion(rect);
    } catch (...) {
        logError(L"Could not export the capture region: " + describeException(std::current_exception()));
        result = {};
    }
    finish(std::move(result));
}

bool OverlayController::isLargeEnough(Box const& rect) const {
    auto physical = toPhysical(rect);
    return physical.width >= 2 && physical.height >= 2;
}

void OverlayController::cancelRegion() {
    regionDrag.reset();
    selection.reset();
    mode = Mode::Idle;
    if (pointer) setHover(targetAt(*pointer));
}

void OverlayController::pointerMove(Point point, bool shift) {
    if (toolbarDrag) {
        moveToolbarDrag(point);
        return;
    }

    bool isBusy = regionDrag.has_value() || annotator->isBusy();

    if (hasToolbar() && !isBusy) {
        toolbar->pointerMove(point, nowMs());

        if (toolbar->isInteracting()) return;

        if (toolbar->popoverContains(point)) {
            pointer.reset();
            setHover(std::nullopt);
            return;
        }

        if (toolbar->contains(point)) return;
    }

    pointer = point;

    if (regionDrag) {
        double distance = std::hypot(point.x - regionDrag->start.x, point.y - regionDrag->start.y);
        if (mode == Mode::Pending && distance >= DRAG_THRESHOLD) mode = Mode::Drawing;

        if (mode == Mode::Drawing) {
            auto end = point;
            if (shift) {
                double size = std::max(std::abs(point.x - regionDrag->start.x), std::abs(point.y - regionDrag->start.y));
                auto sign = [](double value) { return value > 0 ? 1.0 : value < 0 ? -1.0 : 0.0; };
                end = {regionDrag->start.x + sign(point.x - regionDrag->start.x) * size,
                       regionDrag->start.y + sign(point.y - regionDrag->start.y) * size};
            }
            selection = intersect(fromPoints(regionDrag->start, end), viewport());
        }
    } else {
        setHover(targetAt(point));
    }

    if (hasToolbar()) annotator->pointerMove(point, shift);
}

void OverlayController::pointerDown(Point point, bool shift) {
    if (hasToolbar() && toolbar->pointerDown(point, nowMs())) return;

    if (annotator->isEditingText()) {
        if (annotator->editorContains(point)) {
            annotator->editorClick(point, shift);
        } else {
            annotator->finishTextEdit();
        }
        return;
    }

    if (!isAnnotating() && !regionDrag && !(hasToolbar() && annotator->isOverShape(point))) {
        regionDrag = RegionDrag{point, annotator->selectedId.has_value()};
        selection.reset();
        mode = Mode::Pending;
    }

    if (hasToolbar()) annotator->pointerDown(point);
}

void OverlayController::pointerUp(Point point) {
    if (toolbarDrag) {
        endToolbarDrag();
        return;
    }

    if (hasToolbar()) {
        toolbar->pointerUp(point, nowMs());
        annotator->pointerUp(point);
    }

    if (!regionDrag) return;

    bool isClick = mode == Mode::Pending;
    auto drag = *regionDrag;
    std::optional<Box> rect = isClick ? std::optional<Box>(targetAt(point)) : selection;
    regionDrag.reset();

    if (isClick && drag.isDeselect) return cancelRegion();
    if (rect && isLargeEnough(*rect)) return finishRegion(*rect);
    cancelRegion();
}

void OverlayController::pointerCancel() {
    if (toolbarDrag) endToolbarDrag();
    if (regionDrag) cancelRegion();
    if (hasToolbar() && pointer) annotator->pointerUp(*pointer);
}

void OverlayController::pointerLeave() {
    if (regionDrag || toolbarDrag) return;

    pointer.reset();
    setHover(std::nullopt);
    if (hasToolbar()) toolbar->pointerLeave(nowMs());
}

void OverlayController::rightClick(Point point) {
    if (regionDrag) return cancelRegion();
    if (annotator->isEditingText()) return;
    if (hasToolbar() && (toolbar->contains(point) || toolbar->popoverContains(point))) return;
    if (annotator->selectedId) return annotator->select(std::nullopt);
    if (annotator->tool) return annotator->setTool(std::nullopt);
    finish({});
}

void OverlayController::keyDown(Key key, Modifiers modifiers) {
    if (annotator->isEditingText()) {
        annotator->editorKey(key, modifiers.ctrl, modifiers.shift);
        return;
    }

    if (hasToolbar() && toolbar->key(key, nowMs())) return;

    if (key == Key::Escape) {
        if (regionDrag) return cancelRegion();
        if (annotator->selectedId) return annotator->select(std::nullopt);
        return finish({});
    }

    if (hasToolbar() && annotator->handleKey(key, modifiers.ctrl, modifiers.shift)) return;
    if (regionDrag) return;

    if (key == Key::Space) return finishRegion(viewport());

    if (key == Key::Enter) {
        auto rect = pointer ? std::optional<Box>(targetAt(*pointer)) : hover;
        if (rect) finishRegion(*rect);
        return;
    }

    if (modifiers.ctrl || modifiers.alt || modifiers.shift) return;

    int digit = digitOf(key);
    if (digit >= 1 && digit <= static_cast<int>(displays.size())) finishRegion(displays[digit - 1].local);
}

void OverlayController::character(wchar_t character, Modifiers modifiers) {
    if (annotator->isEditingText()) {
        annotator->editorChar(character);
        return;
    }

    if (!hasToolbar() || character <= 32) return;
    if (modifiers.ctrl || modifiers.alt) return;

    if (std::towlower(character) == L'c') {
        annotator->setTool(std::nullopt);
        return;
    }

    annotator->handleShortcut(character);
}

void OverlayController::composition(std::wstring const& text) {
    annotator->editorComposition(text);
}

void OverlayController::commit(std::wstring const& text) {
    annotator->editorCommit(text);
}

CursorKind OverlayController::cursorAt(Point point) {
    if (toolbarDrag) return CursorKind::Hand;

    if (hasToolbar()) {
        if (toolbar->popoverContains(point)) return CursorKind::Arrow;
        if (toolbar->contains(point)) return toolbar->isOverGrip(point) ? CursorKind::Hand : CursorKind::Arrow;

        if (auto anchor = annotator->anchorAt(point)) {
            switch (*anchor) {
            case Anchor::TopLeft:
            case Anchor::BottomRight:
                return CursorKind::ResizeNwse;
            case Anchor::TopRight:
            case Anchor::BottomLeft:
                return CursorKind::ResizeNesw;
            case Anchor::Top:
            case Anchor::Bottom:
                return CursorKind::ResizeNs;
            default:
                return CursorKind::ResizeWe;
            }
        }

        if (annotator->isEditingText() && annotator->editorContains(point)) return CursorKind::IBeam;

        if (annotator->tool == Tool::Select) return CursorKind::Arrow;
        if (annotator->tool == Tool::Text) return CursorKind::IBeam;
        if (annotator->tool) return CursorKind::Cross;
        if (!regionDrag && annotator->isOverShape(point)) return CursorKind::Move;
    }

    return CursorKind::Cross;
}

bool OverlayController::isEditingText() const {
    return annotator->isEditingText();
}

bool OverlayController::caretVisible() const {
    return annotator->caretVisible();
}

std::optional<Box> OverlayController::caretRect() {
    if (!annotator->isEditingText()) return std::nullopt;

    float height = 0;
    auto caret = annotator->editorCaret(height);
    return Box{caret.x, caret.y, 1, height};
}

void OverlayController::afterInput() {
    if (!window || isFinishing) return;

    bool editing = annotator->isEditingText();
    if (editing != wasEditing) {
        wasEditing = editing;
        window->textInputChanged(editing);
    }

    window->invalidate();
}

void setUiResourcesPath(std::filesystem::path const& path) {
    setResourcesPath(path);
}

bool openOverlay(OverlayRequest request, OverlayCallback done) {
    bool expected = false;
    if (!isOpen.compare_exchange_strong(expected, true)) return false;

    auto shared = std::make_shared<OverlayRequest>(std::move(request));
    runOnUi([shared, done = std::move(done)]() mutable {
        OverlayController* overlay = nullptr;

        try {
            overlay = new OverlayController(shared, done);
            active = overlay;
            if (overlay->open()) return;
            logError(L"Could not create the capture overlay window");
        } catch (...) {
            logError(L"Could not open the capture overlay: " + describeException(std::current_exception()));
        }

        if (overlay) {
            overlay->close();
            return;
        }

        active = nullptr;
        isOpen = false;
        if (done) done({});
    });

    return true;
}

bool isOverlayOpen() {
    return isOpen;
}

void closeOverlay() {
    runOnUi([] {
        if (active) active->close();
    });
}

FixtureImage renderFixture(OverlayRequest const& request, FixtureScene const& scene) {
    auto shared = std::make_shared<OverlayRequest>(request);
    OverlayController overlay(shared, nullptr);
    overlay.initializeDevice();

    FixtureImage image;
    image.width = request.bounds.width;
    image.height = request.bounds.height;
    image.bgra = overlay.renderScene(scene);
    return image;
}

FixtureImage renderFixtureRegion(OverlayRequest const& request, FixtureScene const& scene, Box const& region) {
    auto shared = std::make_shared<OverlayRequest>(request);
    OverlayController overlay(shared, nullptr);
    overlay.initializeDevice();

    auto result = overlay.renderRegion(scene, region);
    FixtureImage image;
    image.width = result.width;
    image.height = result.height;
    image.bgra = std::move(result.bgra);
    return image;
}

}

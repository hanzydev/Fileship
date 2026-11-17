#include "overlay.h"

#include "annotator.h"
#include "effects.h"
#include "fixture.h"
#include "gfx.h"
#include "preferences.h"
#include "toolbar.h"
#include "uiThread.h"

#include <shared/i18n.h>
#include <shared/log.h>

#include <commdlg.h>
#include <imm.h>
#include <windowsx.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cwctype>
#include <iterator>
#include <numeric>

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
constexpr UINT_PTR CARET_TIMER = 1;
constexpr double SCAN_OPEN_MS = 250;
constexpr double SCAN_MIN_MS = 900;
constexpr double SCAN_PHASE_MS = 1200;
constexpr double SCAN_TEXT_MS = 300;
constexpr double SCAN_RESULT_MS = 1400;
constexpr double SCAN_FADE_MS = 400;
constexpr double PILL_OUT_MS = 300;
constexpr double COMET_MS = 2400;
constexpr double BEAM_MS = 1600;
constexpr double BEAM_FADE_MS = 300;
constexpr double REVEAL_DELAY_MS = 100;
constexpr double REVEAL_GAP_MS = 150;
constexpr double LOCK_MS = 180;
constexpr double BLUR_MS = 220;
constexpr double BLOCKS_MS = 250;
constexpr double BLOCK_DROP_MS = 80;
constexpr double REVEAL_MS = LOCK_MS + BLUR_MS + BLOCKS_MS;
constexpr double LABEL_MS = 1000;
constexpr double LABEL_TOTAL_MS = 150 + LABEL_MS + 250;
constexpr double TOOLBAR_IN_MS = 300;
constexpr double FIXTURE_NOW = 1e7;

constexpr CLSID BLUR_EFFECT{0x1feb6d69, 0x2fe6, 0x4ac9, {0x8c, 0x58, 0x1d, 0x7f, 0x93, 0xe7, 0xa6, 0xa5}};

wchar_t const* const SCAN_PHASES[] = {
    L"annotate.scan.screen",
    L"annotate.scan.text",
    L"annotate.scan.keys",
    L"annotate.scan.secrets",
    L"annotate.scan.contacts",
    L"annotate.scan.cards",
    L"annotate.scan.addresses",
    L"annotate.scan.almost",
};

wchar_t const* const KIND_KEYS[] = {
    L"annotate.scan.kinds.secret",
    L"annotate.scan.kinds.email",
    L"annotate.scan.kinds.phone",
    L"annotate.scan.kinds.card",
    L"annotate.scan.kinds.account",
    L"annotate.scan.kinds.document",
    L"annotate.scan.kinds.ip",
};

std::atomic<bool> isOpen{false};

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

D2D1_RECT_F rectOf(double x, double y, double width, double height) {
    return D2D1::RectF(static_cast<float>(x), static_cast<float>(y), static_cast<float>(x + width),
                       static_cast<float>(y + height));
}

D2D1_POINT_2F point(double x, double y) {
    return D2D1::Point2F(static_cast<float>(x), static_cast<float>(y));
}

double progressOf(double now, double start, double duration) {
    return std::clamp((now - start) / duration, 0.0, 1.0);
}

double easeInOut(double t) {
    return t < 0.5 ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3) / 2;
}

void forceForeground(HWND window) {
    HWND foreground = GetForegroundWindow();
    DWORD current = GetCurrentThreadId();
    DWORD target = foreground ? GetWindowThreadProcessId(foreground, nullptr) : 0;
    bool shouldAttach = target != 0 && target != current;

    if (shouldAttach) AttachThreadInput(current, target, TRUE);
    BringWindowToTop(window);
    SetForegroundWindow(window);
    SetFocus(window);
    if (shouldAttach) AttachThreadInput(current, target, FALSE);
}

class Overlay;
Overlay* active = nullptr;

class Overlay {
public:
    Overlay(std::shared_ptr<OverlayRequest> value, OverlayCallback callback)
        : request(std::move(value)), done(std::move(callback)) {
        resources = Resources::acquire();
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

        annotator = std::make_unique<Annotator>(*resources, palette.theme);
        annotator->options = preferences.annotations;
        annotator->tool.reset();
        annotator->reset();

        toolbar = std::make_unique<AnnotationToolbar>(*resources, *annotator, palette);
        toolbar->onCancel = [this] { finish({}); };
        toolbar->onDragStart = [this](Point point) { startToolbarDrag(point); };
        toolbar->chooseColor = [this](std::string const& current) { return pickColor(current); };

        auto cursor = toLocal({request->cursor.x, request->cursor.y, 0, 0});
        pointer = Point{cursor.x, cursor.y};
        hover = targetAt(*pointer);

        if (request->scanId && hasToolbar()) startScan(*request->scanId, nowMs());
    }

    ~Overlay() {
        destroyWindow();
    }

    bool initializeGpu() {
        gpu = Gpu::create(*resources);
        auto context = gpu->context.get();

        for (size_t index = 0; index < displays.size(); index++) {
            auto& source = request->displays[index];
            auto& display = displays[index];
            if (source.bgra.size() < static_cast<size_t>(source.rect.width) * source.rect.height * 4) continue;

            auto properties = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_NONE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));
            winrt::check_hresult(context->CreateBitmap(
                D2D1::SizeU(source.rect.width, source.rect.height), source.bgra.data(), source.rect.width * 4,
                properties, display.bitmap.put()));

            source.bgra.clear();
            source.bgra.shrink_to_fit();
        }

        annotator->setSource(
            {ratio, bounds.width, bounds.height, [this](RECT const& rect) { return readSource(rect); }});
        return true;
    }

    bool open() {
        initializeGpu();

        static bool isRegistered = false;
        if (!isRegistered) {
            WNDCLASSW windowClass{};
            windowClass.lpfnWndProc = handleMessage;
            windowClass.hInstance = GetModuleHandleW(nullptr);
            windowClass.lpszClassName = L"FileshipOverlay";
            windowClass.hCursor = nullptr;
            RegisterClassW(&windowClass);
            isRegistered = true;
        }

        window = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOREDIRECTIONBITMAP, L"FileshipOverlay",
                                 L"Fileship", WS_POPUP, bounds.x, bounds.y, bounds.width, bounds.height, nullptr,
                                 nullptr, GetModuleHandleW(nullptr), this);
        if (!window) return false;

        ImmAssociateContextEx(window, nullptr, 0);
        surface.create(*gpu, window, bounds.width, bounds.height, false);
        paint();
        gpu->composition->WaitForCommitCompletion();

        ShowWindow(window, SW_SHOW);
        forceForeground(window);
        return true;
    }

    void finish(OverlayResult result) {
        if (isFinishing) return;
        isFinishing = true;

        if (preferences.toolbar.display != loaded.toolbar.display || preferences.toolbar.edge != loaded.toolbar.edge ||
            preferences.toolbar.cx != loaded.toolbar.cx || preferences.toolbar.cy != loaded.toolbar.cy ||
            !sameOptions(annotator->options, loaded.annotations)) {
            preferences.annotations = annotator->options;
            savePreferences(preferences);
        }

        destroyWindow();
        if (active == this) active = nullptr;

        auto callback = std::move(done);
        auto payload = std::make_shared<OverlayResult>(std::move(result));
        runOnUi([this, callback, payload] {
            delete this;
            isOpen = false;
            if (callback) callback(std::move(*payload));
        });
    }

    FixtureImage renderOffscreen(FixtureScene const& scene) {
        applyScene(scene);

        auto context = gpu->context.get();
        com_ptr<ID2D1Bitmap1> target;
        auto properties =
            D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,
                                    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        winrt::check_hresult(context->CreateBitmap(D2D1::SizeU(bounds.width, bounds.height), nullptr, 0, properties,
                                                   target.put()));

        context->SetTarget(target.get());
        context->BeginDraw();
        context->SetTransform(D2D1::Matrix3x2F::Identity());
        render(context);
        winrt::check_hresult(context->EndDraw());
        context->SetTarget(nullptr);

        FixtureImage image;
        image.width = bounds.width;
        image.height = bounds.height;
        image.bgra = readBitmap(context, target.get(),
                                D2D1::RectU(0, 0, static_cast<UINT32>(bounds.width), static_cast<UINT32>(bounds.height)));
        return image;
    }

    FixtureImage renderRegion(FixtureScene const& scene, Box const& region) {
        applyScene(scene);
        auto result = exportRegion(region);

        FixtureImage image;
        image.width = result.width;
        image.height = result.height;
        image.bgra = std::move(result.bgra);
        return image;
    }

    void close() {
        finish({});
    }

    void receiveScan(uint64_t id, std::vector<Redaction> redactions, ScanOutcome outcome) {
        if (!scan || scan->id != id || scan->result != ScanResult::Pending) return;

        scan->deliveries++;
        if (outcome == ScanOutcome::Failed) scan->failed = true;
        if (outcome == ScanOutcome::Unavailable) scan->unavailable = true;
        for (auto& redaction : redactions) scan->found.push_back(redaction);
        if (scan->deliveries >= displays.size()) endScan(clock());
        invalidate();
    }

    void completeScan(uint64_t id) {
        if (!scan || scan->id != id || scan->result != ScanResult::Pending) return;

        endScan(clock());
        invalidate();
    }

private:
    struct Display {
        int64_t id = 0;
        ScreenRect physical;
        Box local;
        double scaleFactor = 1;
        bool isPrimary = false;
        com_ptr<ID2D1Bitmap1> bitmap;
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

    enum class ScanResult { Pending, Found, Failed, Unavailable };

    struct Reveal {
        std::string id;
        RedactionKind kind = RedactionKind::Secret;
        Box box;
        size_t display = 0;
        double start = 0;
        RECT crop{};
        int block = 0;
        int columns = 0;
        float deviation = 0;
        std::vector<int> order;
        com_ptr<ID2D1Bitmap1> final;
        com_ptr<ID2D1Effect> blur;
    };

    struct Scan {
        uint64_t id = 0;
        double start = 0;
        size_t pill = 0;
        size_t deliveries = 0;
        bool failed = false;
        bool unavailable = false;
        std::vector<Redaction> found;
        ScanResult result = ScanResult::Pending;
        double resultAt = 0;
        double unlockAt = 0;
        double holdUntil = 0;
        std::wstring from;
        std::vector<Reveal> reveals;
        bool isUnlocked = false;
    };

    struct Beam {
        double edge = 0;
        double band = 0;
        double alpha = 0;
    };

    static bool sameOptions(AnnotationOptions const& a, AnnotationOptions const& b) {
        if (a.fill != b.fill || a.background != b.background || a.shadow != b.shadow || a.emoji != b.emoji) return false;
        for (size_t index = 0; index < TOOL_COUNT; index++)
            if (a.tools[index].color != b.tools[index].color || a.tools[index].size != b.tools[index].size) return false;
        return true;
    }

    void applyScene(FixtureScene const& scene) {
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

        if (scene.scan && hasToolbar()) {
            frozen = FIXTURE_NOW;
            startScan(1, FIXTURE_NOW - scene.scan->elapsedMs);
            reducedMotion = false;

            if (scene.scan->deliveredMs) {
                frozen = scan->start + *scene.scan->deliveredMs;
                receiveScan(1, scene.scan->redactions, scene.scan->outcome);
                completeScan(1);
                frozen = FIXTURE_NOW;
            }
        }
    }

    Box toLocal(ScreenRect const& rect) const {
        return {(rect.x - bounds.x) / ratio, (rect.y - bounds.y) / ratio, rect.width / ratio, rect.height / ratio};
    }

    Box viewport() const {
        return {0, 0, bounds.width / ratio, bounds.height / ratio};
    }

    ScreenRect toPhysical(Box const& rect) const {
        int x = static_cast<int>(jsRound(rect.x * ratio));
        int y = static_cast<int>(jsRound(rect.y * ratio));
        int width = std::max(1, static_cast<int>(jsRound((rect.x + rect.width) * ratio)) - x);
        int height = std::max(1, static_cast<int>(jsRound((rect.y + rect.height) * ratio)) - y);
        return {x, y, width, height};
    }

    ScreenRect toScreen(Box const& rect) const {
        auto physical = toPhysical(rect);
        return {physical.x + bounds.x, physical.y + bounds.y, physical.width, physical.height};
    }

    Display const* displayAt(Point point) const {
        for (auto const& display : displays)
            if (display.local.contains(point.x, point.y)) return &display;
        return nullptr;
    }

    Box targetAt(Point point) const {
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

    bool isRecording() const {
        return request->purpose == OverlayPurpose::Video || request->purpose == OverlayPurpose::Gif;
    }

    bool hasToolbar() const {
        return request->purpose == OverlayPurpose::Capture;
    }

    bool isAnnotating() const {
        return hasToolbar() && annotator->tool.has_value();
    }

    Display const* toolbarDisplay() const {
        auto id = toolbarDrag ? std::optional<int64_t>(toolbarDrag->display) : preferences.toolbar.display;
        for (auto const& display : displays)
            if (id && display.id == *id) return &display;
        for (auto const& display : displays)
            if (display.isPrimary) return &display;
        return displays.empty() ? nullptr : &displays.front();
    }

    Box toolbarHost() const {
        auto display = toolbarDisplay();
        return display ? display->local : viewport();
    }

    double toolbarZoom() const {
        auto display = toolbarDisplay();
        return (display ? display->scaleFactor : 1) / ratio;
    }

    ToolbarEdge edgeAt(Point point) const {
        auto host = toolbarHost();
        if (point.x < host.x + TOOLBAR_EDGE) return ToolbarEdge::Left;
        if (point.x > host.right() - TOOLBAR_EDGE) return ToolbarEdge::Right;
        if (point.y < host.y + TOOLBAR_EDGE_Y) return ToolbarEdge::Top;
        if (point.y > host.bottom() - TOOLBAR_EDGE_Y) return ToolbarEdge::Bottom;
        return ToolbarEdge::None;
    }

    ToolbarEdge currentEdge() const {
        return toolbarDrag ? edgeAt(toolbarDrag->pointer) : preferences.toolbar.edge;
    }

    Side toolbarSide() const {
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

    ToolbarLayout layoutToolbar() {
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

    void startToolbarDrag(Point point) {
        auto layout = layoutToolbar();
        auto display = toolbarDisplay();
        toolbarDrag = ToolbarDrag{{point.x - layout.left, point.y - layout.top}, point, display ? display->id : 0};
    }

    void moveToolbarDrag(Point point) {
        auto display = displayAt(point);
        toolbarDrag->pointer = point;
        if (display) toolbarDrag->display = display->id;
    }

    void endToolbarDrag() {
        auto host = toolbarHost();
        auto layout = layoutToolbar();
        auto display = toolbarDisplay();

        preferences.toolbar.display = display ? std::optional<int64_t>(display->id) : std::nullopt;
        preferences.toolbar.edge = currentEdge();
        preferences.toolbar.cx = (layout.center.x - host.x) / host.width;
        preferences.toolbar.cy = (layout.center.y - host.y) / host.height;
        toolbarDrag.reset();
    }

    std::optional<Box> animatedHover() {
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

    void setHover(std::optional<Box> next) {
        if (isSameBox(next, hover)) return;

        auto from = animatedHover();
        hover = next;
        if (from && next) {
            hoverAnimation = std::pair{*from, nowMs()};
        } else {
            hoverAnimation.reset();
        }
    }

    std::vector<uint8_t> readSource(RECT const& rect) {
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        std::vector<uint8_t> pixels(static_cast<size_t>(std::max(0, width)) * std::max(0, height) * 4, 0);
        for (size_t index = 3; index < pixels.size(); index += 4) pixels[index] = 255;
        if (width <= 0 || height <= 0) return pixels;

        for (auto const& display : displays) {
            if (!display.bitmap) continue;

            LONG left = display.physical.x - bounds.x;
            LONG top = display.physical.y - bounds.y;
            LONG x0 = std::max(rect.left, left);
            LONG y0 = std::max(rect.top, top);
            LONG x1 = std::min(rect.right, left + display.physical.width);
            LONG y1 = std::min(rect.bottom, top + display.physical.height);
            if (x1 <= x0 || y1 <= y0) continue;

            auto part = readBitmap(gpu->context.get(), display.bitmap.get(),
                                   D2D1::RectU(static_cast<UINT32>(x0 - left), static_cast<UINT32>(y0 - top),
                                               static_cast<UINT32>(x1 - left), static_cast<UINT32>(y1 - top)));
            LONG partWidth = x1 - x0;
            for (LONG row = 0; row < y1 - y0; row++) {
                memcpy(pixels.data() + ((static_cast<size_t>(row + y0 - rect.top) * width) + (x0 - rect.left)) * 4,
                       part.data() + static_cast<size_t>(row) * partWidth * 4, static_cast<size_t>(partWidth) * 4);
            }
        }

        for (size_t index = 3; index < pixels.size(); index += 4) pixels[index] = 255;
        return pixels;
    }

    void drawLabel(Painter& painter, std::wstring const& text, double x, double y) {
        float width = painter.resources().measure(text, 12, DWRITE_FONT_WEIGHT_MEDIUM) + 12;
        painter.fillRounded(rectOf(x, y, width, 20), 6, rgba(0, 0, 0, 0.75f));
        painter.textAt(text, static_cast<float>(x + 6), painter.middleBaseline(static_cast<float>(y + 10), 12), 12,
                       DWRITE_FONT_WEIGHT_MEDIUM, rgba(255, 255, 255));
    }

    void drawSourcePixels(ID2D1DeviceContext* context, ScreenRect const& source, D2D1_RECT_F const& destination) {
        double scaleX = (destination.right - destination.left) / source.width;
        double scaleY = (destination.bottom - destination.top) / source.height;

        for (auto const& display : displays) {
            if (!display.bitmap) continue;

            int left = display.physical.x - bounds.x;
            int top = display.physical.y - bounds.y;
            int x0 = std::max(source.x, left);
            int y0 = std::max(source.y, top);
            int x1 = std::min(source.x + source.width, left + display.physical.width);
            int y1 = std::min(source.y + source.height, top + display.physical.height);
            if (x1 <= x0 || y1 <= y0) continue;

            auto target = D2D1::RectF(destination.left + static_cast<float>((x0 - source.x) * scaleX),
                                      destination.top + static_cast<float>((y0 - source.y) * scaleY),
                                      destination.left + static_cast<float>((x1 - source.x) * scaleX),
                                      destination.top + static_cast<float>((y1 - source.y) * scaleY));
            auto cut = D2D1::RectF(static_cast<float>(x0 - left), static_cast<float>(y0 - top),
                                   static_cast<float>(x1 - left), static_cast<float>(y1 - top));
            context->DrawBitmap(display.bitmap.get(), target, 1.0f, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &cut,
                                nullptr);
        }
    }

    void drawMagnifier(Painter& painter, Point point) {
        auto context = painter.context();
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
        com_ptr<ID2D1RoundedRectangleGeometry> clip;
        resources->factory()->CreateRoundedRectangleGeometry(D2D1::RoundedRect(frame, 8, 8), clip.put());
        context->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), clip.get()), nullptr);

        painter.fillRect(frame, rgba(0, 0, 0));
        drawSourcePixels(context,
                         {physical.x - half, physical.y - half, static_cast<int>(MAGNIFIER_PIXELS),
                          static_cast<int>(MAGNIFIER_PIXELS)},
                         frame);

        auto grid = rgba(255, 255, 255, 0.12f);
        for (int index = 1; index < static_cast<int>(MAGNIFIER_PIXELS); index++) {
            float offset = static_cast<float>(index * MAGNIFIER_ZOOM) + 0.5f;
            context->DrawLine(D2D1::Point2F(frame.left + offset, frame.top),
                              D2D1::Point2F(frame.left + offset, frame.bottom), painter.brush(grid), 1);
            context->DrawLine(D2D1::Point2F(frame.left, frame.top + offset),
                              D2D1::Point2F(frame.right, frame.top + offset), painter.brush(grid), 1);
        }

        float center = static_cast<float>(half * MAGNIFIER_ZOOM);
        context->DrawRectangle(D2D1::RectF(frame.left + center, frame.top + center,
                                           frame.left + center + static_cast<float>(MAGNIFIER_ZOOM),
                                           frame.top + center + static_cast<float>(MAGNIFIER_ZOOM)),
                               painter.brush(color(palette.theme.primary)), 2);
        context->PopLayer();

        painter.strokeRounded(rectOf(x - 0.5, y - 0.5, size + 1, size + 1), 8, rgba(255, 255, 255, 0.8f), 1);

        auto label = std::to_wstring(physical.x + bounds.x) + L", " + std::to_wstring(physical.y + bounds.y);
        drawLabel(painter, label, x, y + size + 6);
    }

    void render(ID2D1DeviceContext* context) {
        Painter painter(context, *resources);
        isAnimatingHover = false;
        double now = clock();
        bool locked = isLocked(now);

        context->Clear(D2D1::ColorF(0, 0, 0, 1));
        for (auto const& display : displays) {
            if (!display.bitmap) continue;
            float left = static_cast<float>(display.physical.x - bounds.x);
            float top = static_cast<float>(display.physical.y - bounds.y);
            context->DrawBitmap(display.bitmap.get(),
                                D2D1::RectF(left, top, left + display.physical.width, top + display.physical.height), 1.0f,
                                D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, nullptr, nullptr);
        }

        auto scale = static_cast<float>(ratio);
        Transform local(context, D2D1::Matrix3x2F::Scale(scale, scale));

        if (scan) revealShapes(now);
        if (hasToolbar()) annotator->render(context, false);
        if (scan) drawRevealContent(context, now);

        if (locked) {
            drawScanDim(painter, now);
        } else if (!isAnnotating()) {
            auto view = viewport();
            auto region = mode == Mode::Drawing ? selection : animatedHover();
            auto dim = rgba(0, 0, 0, 0.45f);

            if (region) {
                com_ptr<ID2D1RectangleGeometry> outer;
                com_ptr<ID2D1RectangleGeometry> inner;
                resources->factory()->CreateRectangleGeometry(view.rect(), outer.put());
                resources->factory()->CreateRectangleGeometry(region->rect(), inner.put());
                ID2D1Geometry* parts[] = {outer.get(), inner.get()};
                com_ptr<ID2D1GeometryGroup> group;
                resources->factory()->CreateGeometryGroup(D2D1_FILL_MODE_ALTERNATE, parts, 2, group.put());
                context->FillGeometry(group.get(), painter.brush(dim));

                bool isDrawing = mode == Mode::Drawing;
                float width = isDrawing ? 2.0f : 1.5f;
                auto style = isDrawing ? nullptr : resources->dashStroke(6 / width, 4 / width);
                context->DrawRectangle(region->rect(), painter.brush(color(palette.theme.primary)), width, style);
            } else {
                painter.fillRect(view.rect(), dim);
            }

            if (mode == Mode::Drawing && selection) {
                auto physical = toPhysical(*selection);
                double labelY = selection->y >= 28 ? selection->y - 26 : selection->y + 6;
                drawLabel(painter,
                          std::to_wstring(physical.width) + L" × " + std::to_wstring(physical.height),
                          selection->x, labelY);
            }

            if (request->showMagnifier && pointer) drawMagnifier(painter, *pointer);
        }

        if (scan) drawScan(painter, now);

        if (hasToolbar() && !locked) {
            layoutToolbar();
            double progress = scan ? progressOf(now, scan->unlockAt, TOOLBAR_IN_MS) : 1;

            if (progress < 1) {
                auto reveal = revealAt(progress, true, toolbarSide(), false, !reducedMotion);
                painter.pushOpacity(static_cast<float>(reveal.opacity));
                {
                    Transform slide(context, D2D1::Matrix3x2F::Translation(reveal.shift.x, reveal.shift.y));
                    toolbar->render(painter, nowMs());
                }
                painter.popOpacity();
            } else {
                toolbar->render(painter, nowMs());
            }
        }
    }

    double clock() const {
        return frozen ? *frozen : nowMs();
    }

    size_t displayIndexAt(Point point) const {
        for (size_t index = 0; index < displays.size(); index++)
            if (displays[index].local.contains(point.x, point.y)) return index;
        for (size_t index = 0; index < displays.size(); index++)
            if (displays[index].isPrimary) return index;
        return 0;
    }

    double zoomOf(size_t index) const {
        return index < displays.size() ? displays[index].scaleFactor / ratio : 1;
    }

    void startScan(uint64_t id, double now) {
        BOOL animations = TRUE;
        SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animations, 0);
        reducedMotion = !animations;

        Scan value;
        value.id = id;
        value.start = now;
        value.pill = displayIndexAt(pointer.value_or(Point{}));
        scan = std::move(value);
    }

    bool isLocked(double now) const {
        return scan && (scan->result == ScanResult::Pending || now < scan->unlockAt);
    }

    static std::wstring phaseText(int index) {
        if (index <= 0) return t(L"annotate.scan.detecting");
        return t(SCAN_PHASES[static_cast<size_t>(index - 1) % std::size(SCAN_PHASES)]);
    }

    std::wstring resultText() const {
        if (scan->result == ScanResult::Failed) return t(L"annotate.scan.failed");
        if (scan->reveals.empty()) return t(L"annotate.scan.none");
        return t(L"annotate.scan.found", {{L"count", std::to_wstring(scan->reveals.size())}});
    }

    std::vector<std::pair<Box, RedactionKind>> readingOrder(std::vector<Redaction> const& redactions) const {
        std::vector<std::pair<Box, RedactionKind>> items;
        for (auto const& redaction : redactions) {
            auto box = intersect(toLocal(redaction.rect), viewport());
            if (box && isLargeEnough(*box)) items.push_back({*box, redaction.kind});
        }

        std::sort(items.begin(), items.end(), [](auto const& a, auto const& b) { return a.first.y < b.first.y; });

        std::vector<std::pair<int, size_t>> keys;
        int line = -1;
        double bottom = 0;
        for (size_t index = 0; index < items.size(); index++) {
            auto const& box = items[index].first;
            if (line < 0 || box.y >= bottom - box.height / 2) {
                line++;
                bottom = box.bottom();
            } else {
                bottom = std::max(bottom, box.bottom());
            }
            keys.push_back({line, index});
        }

        std::stable_sort(keys.begin(), keys.end(), [&](auto const& a, auto const& b) {
            if (a.first != b.first) return a.first < b.first;
            return items[a.second].first.x < items[b.second].first.x;
        });

        std::vector<std::pair<Box, RedactionKind>> result;
        for (auto const& key : keys) result.push_back(items[key.second]);
        return result;
    }

    void prepareReveal(Reveal& reveal, Shape const& shape, size_t index) {
        try {
            auto effect = annotator->effectPixels(shape);
            if (!effect) return;

            auto const& crop = effect->crop;
            int width = crop.right - crop.left;
            int height = crop.bottom - crop.top;
            reveal.crop = crop;
            reveal.block = redactBlock(width, height);
            reveal.columns = (width + reveal.block - 1) / reveal.block;
            reveal.deviation = static_cast<float>(std::clamp(std::min(width, height) * 0.3, 2.0, 20.0));

            int rows = (height + reveal.block - 1) / reveal.block;
            reveal.order.resize(static_cast<size_t>(reveal.columns) * rows);
            std::iota(reveal.order.begin(), reveal.order.end(), 0);
            uint32_t state = static_cast<uint32_t>(index + 1) * 0x9e3779b9U;
            for (size_t item = reveal.order.size(); item > 1; item--) {
                state ^= state << 13;
                state ^= state >> 17;
                state ^= state << 5;
                std::swap(reveal.order[item - 1], reveal.order[state % item]);
            }

            auto context = gpu->context.get();
            auto properties = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_NONE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
            winrt::check_hresult(context->CreateBitmap(D2D1::SizeU(width, height), effect->pixels.data(), width * 4,
                                                       properties, reveal.final.put()));

            auto pixels = readSource(crop);
            com_ptr<ID2D1Bitmap1> sharp;
            winrt::check_hresult(
                context->CreateBitmap(D2D1::SizeU(width, height), pixels.data(), width * 4, properties, sharp.put()));
            winrt::check_hresult(context->CreateEffect(BLUR_EFFECT, reveal.blur.put()));
            reveal.blur->SetInput(0, sharp.get());
            winrt::check_hresult(reveal.blur->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD));
        } catch (winrt::hresult_error const& error) {
            logWarn(L"Could not prepare a redaction animation: " + std::wstring(error.message()));
            reveal.final = nullptr;
            reveal.blur = nullptr;
        }
    }

    void endScan(double now) {
        auto& value = *scan;
        bool isMissing = value.deliveries < displays.size();
        if (value.failed || (isMissing && !value.unavailable)) {
            value.result = ScanResult::Failed;
        } else if (value.unavailable && value.found.empty()) {
            value.result = ScanResult::Unavailable;
        } else {
            value.result = ScanResult::Found;
        }

        double at = std::max(now, value.start + SCAN_MIN_MS);
        double phase = std::fmod(at - value.start, SCAN_PHASE_MS);
        if (at - value.start >= SCAN_PHASE_MS && phase < SCAN_TEXT_MS) at += SCAN_TEXT_MS - phase;
        value.resultAt = at;
        value.from = phaseText(static_cast<int>((at - value.start) / SCAN_PHASE_MS));

        auto findings = readingOrder(value.found);
        std::vector<Shape> shapes;
        for (auto const& [box, kind] : findings) {
            Shape shape;
            shape.type = Tool::Redact;
            shape.x = box.x;
            shape.y = box.y;
            shape.width = box.width;
            shape.height = box.height;
            shapes.push_back(std::move(shape));
        }

        auto ids = annotator->addShapes(shapes);
        for (size_t index = 0; index < ids.size(); index++) {
            auto shape = shapes[index];
            shape.id = ids[index];

            Reveal reveal;
            reveal.id = ids[index];
            reveal.kind = findings[index].second;
            reveal.box = findings[index].first;
            reveal.display = displayIndexAt({reveal.box.x + reveal.box.width / 2, reveal.box.y + reveal.box.height / 2});
            reveal.start = at + REVEAL_DELAY_MS + static_cast<double>(index) * REVEAL_GAP_MS;
            prepareReveal(reveal, shape, index);
            annotator->hidden.insert(reveal.id);
            value.reveals.push_back(std::move(reveal));
        }

        value.unlockAt = at + BEAM_FADE_MS;
        if (!value.reveals.empty()) value.unlockAt = std::max(value.unlockAt, value.reveals.back().start + REVEAL_MS);
        value.holdUntil =
            value.result == ScanResult::Unavailable ? at : std::max(at + SCAN_RESULT_MS, value.unlockAt);
    }

    double scanEnd() const {
        double end = std::max({scan->holdUntil + SCAN_FADE_MS, scan->unlockAt + TOOLBAR_IN_MS,
                               scan->resultAt + BEAM_FADE_MS});
        if (!scan->reveals.empty()) end = std::max(end, scan->reveals.back().start + REVEAL_MS + LABEL_TOTAL_MS);
        return end;
    }

    bool isScanAnimating(double now) const {
        return scan && (scan->result == ScanResult::Pending || now < scanEnd());
    }

    void revealShapes(double now) {
        for (auto const& reveal : scan->reveals)
            if (now >= reveal.start + REVEAL_MS) annotator->hidden.erase(reveal.id);
    }

    std::optional<Beam> beamAt(Display const& display, double now) const {
        if (reducedMotion) return std::nullopt;

        double elapsed = now - scan->start;
        double alpha = ease(Easing::Ease, progressOf(elapsed, 0, SCAN_OPEN_MS));
        if (scan->result != ScanResult::Pending && now >= scan->resultAt) {
            double lastPass = std::floor((scan->resultAt - scan->start) / BEAM_MS);
            if (std::floor(elapsed / BEAM_MS) > lastPass) return std::nullopt;
            alpha *= 1 - progressOf(now, scan->resultAt, BEAM_FADE_MS);
        }
        if (alpha <= 0) return std::nullopt;

        double band = display.local.height * 0.22;
        double travel = easeInOut(std::fmod(elapsed, BEAM_MS) / BEAM_MS);
        return Beam{display.local.y + travel * (display.local.height + band), band, alpha};
    }

    com_ptr<ID2D1LinearGradientBrush> gradient(ID2D1DeviceContext* context, double top, double bottom,
                                               D2D1_COLOR_F const& from, D2D1_COLOR_F const& to) {
        D2D1_GRADIENT_STOP stops[] = {{0, from}, {1, to}};
        com_ptr<ID2D1GradientStopCollection> collection;
        winrt::check_hresult(context->CreateGradientStopCollection(stops, 2, collection.put()));

        com_ptr<ID2D1LinearGradientBrush> brush;
        winrt::check_hresult(context->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(D2D1::Point2F(0, static_cast<float>(top)),
                                                D2D1::Point2F(0, static_cast<float>(bottom))),
            collection.get(), brush.put()));
        return brush;
    }

    void drawScanDim(Painter& painter, double now) {
        auto context = painter.context();
        auto view = viewport();
        auto dim = rgba(0, 0, 0, 0.45f);

        struct Band {
            Display const* display;
            Beam beam;
            Box rect;
        };

        std::vector<Band> bands;
        for (auto const& display : displays) {
            auto beam = beamAt(display, now);
            if (!beam) continue;
            auto rect = intersect({display.local.x, beam->edge - beam->band, display.local.width, beam->band},
                                  display.local);
            if (rect) bands.push_back({&display, *beam, *rect});
        }

        if (bands.empty()) {
            painter.fillRect(view.rect(), dim);
            return;
        }

        std::vector<com_ptr<ID2D1RectangleGeometry>> rects(bands.size() + 1);
        std::vector<ID2D1Geometry*> parts;
        resources->factory()->CreateRectangleGeometry(view.rect(), rects[0].put());
        parts.push_back(rects[0].get());
        for (size_t index = 0; index < bands.size(); index++) {
            resources->factory()->CreateRectangleGeometry(bands[index].rect.rect(), rects[index + 1].put());
            parts.push_back(rects[index + 1].get());
        }

        com_ptr<ID2D1GeometryGroup> group;
        resources->factory()->CreateGeometryGroup(D2D1_FILL_MODE_ALTERNATE, parts.data(),
                                                  static_cast<UINT32>(parts.size()), group.put());
        context->FillGeometry(group.get(), painter.brush(dim));

        auto primary = color(palette.theme.primary);
        auto bright = mix(primary, rgba(255, 255, 255), 0.55f);

        for (auto const& [display, beam, rect] : bands) {
            double zoom = display->scaleFactor / ratio;
            float alpha = static_cast<float>(beam.alpha);
            double top = beam.edge - beam.band;

            auto fade = [&](D2D1_RECT_F const& area, double from, double to, D2D1_COLOR_F const& start,
                            D2D1_COLOR_F const& end) {
                context->FillRectangle(area, gradient(context, from, to, start, end).get());
            };

            double glow = 26 * zoom;
            double left = display->local.x;
            double width = display->local.width;

            context->PushAxisAlignedClip(display->local.rect(), D2D1_ANTIALIAS_MODE_ALIASED);
            fade(rect.rect(), top, beam.edge, dim, rgba(0, 0, 0, 0.45f * (1 - 0.9f * alpha)));
            fade(rect.rect(), top, beam.edge, withAlpha(primary, 0), withAlpha(primary, 0.12f * alpha));
            fade(rectOf(left, beam.edge - glow, width, glow), beam.edge - glow, beam.edge, withAlpha(primary, 0),
                 withAlpha(primary, 0.35f * alpha));
            fade(rectOf(left, beam.edge, width, 6 * zoom), beam.edge, beam.edge + 6 * zoom,
                 withAlpha(primary, 0.3f * alpha), withAlpha(primary, 0));
            painter.fillRect(rectOf(display->local.x, beam.edge - 0.75 * zoom, display->local.width, 1.5 * zoom),
                             withAlpha(bright, alpha));
            context->PopAxisAlignedClip();
        }
    }

    void drawComet(Painter& painter, Display const& display, double now, float alpha) {
        auto context = painter.context();
        double zoom = display.scaleFactor / ratio;
        double inset = 3 * zoom;
        double left = display.local.x + inset;
        double top = display.local.y + inset;
        double right = display.local.right() - inset;
        double bottom = display.local.bottom() - inset;
        double width = right - left;
        double height = bottom - top;
        double perimeter = 2 * (width + height);
        if (perimeter <= 0) return;

        auto edgeOf = [&](double distance) {
            if (distance < width) return 0;
            if (distance < width + height) return 1;
            if (distance < 2 * width + height) return 2;
            return 3;
        };

        auto pointAt = [&](double distance) {
            switch (edgeOf(distance)) {
            case 0:
                return point(left + distance, top);
            case 1:
                return point(right, top + distance - width);
            case 2:
                return point(right - (distance - width - height), bottom);
            default:
                return point(left, bottom - (distance - 2 * width - height));
            }
        };

        auto corner = [&](int edge) {
            switch (edge) {
            case 0:
                return point(left, top);
            case 1:
                return point(right, top);
            case 2:
                return point(right, bottom);
            default:
                return point(left, bottom);
            }
        };

        auto wrap = [&](double distance) {
            double value = std::fmod(distance, perimeter);
            return value < 0 ? value + perimeter : value;
        };

        double head = std::fmod(now - scan->start, COMET_MS) / COMET_MS * perimeter;
        double tail = perimeter * 0.16;
        constexpr int SEGMENTS = 48;
        auto primary = color(palette.theme.primary);
        auto bright = mix(primary, rgba(255, 255, 255), 0.5f);

        for (int pass = 0; pass < 2; pass++) {
            float lineWidth = static_cast<float>((pass ? 3 : 10) * zoom);
            for (int segment = SEGMENTS - 1; segment >= 0; segment--) {
                double from = wrap(head - tail * (segment + 1) / SEGMENTS);
                double to = wrap(head - tail * segment / SEGMENTS);
                float fade = static_cast<float>(std::pow(1 - static_cast<double>(segment) / SEGMENTS, 1.6)) * alpha;
                auto brush = painter.brush(pass ? withAlpha(bright, fade) : withAlpha(primary, 0.35f * fade));

                int start = edgeOf(from);
                int end = edgeOf(to);
                if (start == end && to >= from) {
                    context->DrawLine(pointAt(from), pointAt(to), brush, lineWidth);
                } else {
                    context->DrawLine(pointAt(from), corner(end), brush, lineWidth);
                    context->DrawLine(corner(end), pointAt(to), brush, lineWidth);
                }
            }
        }

        auto tip = pointAt(head);
        painter.fillEllipse(tip.x, tip.y, static_cast<float>(9 * zoom), static_cast<float>(9 * zoom),
                            withAlpha(primary, 0.4f * alpha));
        painter.fillEllipse(tip.x, tip.y, static_cast<float>(3 * zoom), static_cast<float>(3 * zoom),
                            withAlpha(rgba(255, 255, 255), alpha));
    }

    void drawBorder(Painter& painter, Display const& display, double now, float alpha) {
        auto context = painter.context();
        double zoom = display.scaleFactor / ratio;
        auto primary = color(palette.theme.primary);
        auto const& area = display.local;

        context->PushAxisAlignedClip(area.rect(), D2D1_ANTIALIAS_MODE_ALIASED);

        std::pair<double, float> const glows[] = {{14, 0.06f}, {6, 0.12f}, {2, 0.28f}};
        for (auto const& [size, opacity] : glows) {
            double width = size * zoom;
            context->DrawRectangle(
                rectOf(area.x + width / 2, area.y + width / 2, area.width - width, area.height - width),
                painter.brush(withAlpha(primary, opacity * alpha)), static_cast<float>(width));
        }

        double line = 1.5 * zoom;
        context->DrawRectangle(rectOf(area.x + line / 2, area.y + line / 2, area.width - line, area.height - line),
                               painter.brush(withAlpha(primary, 0.9f * alpha)), static_cast<float>(line));

        if (!reducedMotion) drawComet(painter, display, now, alpha);
        context->PopAxisAlignedClip();
    }

    void drawRevealContent(ID2D1DeviceContext* context, double now) {
        D2D1_MATRIX_3X2_F saved{};
        context->GetTransform(&saved);
        context->SetTransform(D2D1::Matrix3x2F::Identity());

        for (auto const& reveal : scan->reveals) {
            double elapsed = now - reveal.start;
            if (!reveal.final || elapsed < LOCK_MS || elapsed >= REVEAL_MS) continue;

            auto area = D2D1::RectF(static_cast<float>(reveal.crop.left), static_cast<float>(reveal.crop.top),
                                    static_cast<float>(reveal.crop.right), static_cast<float>(reveal.crop.bottom));
            context->PushAxisAlignedClip(area, D2D1_ANTIALIAS_MODE_ALIASED);

            double blur = ease(Easing::Standard, progressOf(elapsed, LOCK_MS, BLUR_MS));
            float deviation = reveal.deviation * static_cast<float>(blur);
            reveal.blur->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, deviation);
            context->DrawImage(reveal.blur.get(), D2D1::Point2F(area.left, area.top));

            double drop = elapsed - LOCK_MS - BLUR_MS;
            size_t count = reveal.order.size();
            float block = static_cast<float>(reveal.block);
            for (size_t rank = 0; rank < count; rank++) {
                double progress = progressOf(drop, (BLOCKS_MS - BLOCK_DROP_MS) * rank / count, BLOCK_DROP_MS);
                if (progress <= 0) break;

                double eased = easeOutCubic(progress);
                int index = reveal.order[rank];
                float left = area.left + static_cast<float>(index % reveal.columns) * block;
                float top = area.top + static_cast<float>(index / reveal.columns) * block;
                float right = std::min(left + block, area.right);
                float bottom = std::min(top + block, area.bottom);
                float offset = reducedMotion ? 0 : -static_cast<float>(1 - eased) * block * 0.6f;
                auto source = D2D1::RectF(left - area.left, top - area.top, right - area.left, bottom - area.top);
                context->DrawBitmap(reveal.final.get(), D2D1::RectF(left, top + offset, right, bottom + offset),
                                    static_cast<float>(eased), D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &source,
                                    nullptr);
            }

            context->PopAxisAlignedClip();
        }

        context->SetTransform(saved);
    }

    void drawBrackets(Painter& painter, Box const& box, double zoom, D2D1_COLOR_F const& tint) {
        double arm = std::clamp(std::min(box.width, box.height) * 0.4, 4 * zoom, 14 * zoom);
        com_ptr<ID2D1PathGeometry> geometry;
        winrt::check_hresult(resources->factory()->CreatePathGeometry(geometry.put()));
        com_ptr<ID2D1GeometrySink> sink;
        winrt::check_hresult(geometry->Open(sink.put()));

        auto corner = [&](double x, double y, double dx, double dy) {
            sink->BeginFigure(point(x, y + dy * arm), D2D1_FIGURE_BEGIN_HOLLOW);
            sink->AddLine(point(x, y));
            sink->AddLine(point(x + dx * arm, y));
            sink->EndFigure(D2D1_FIGURE_END_OPEN);
        };

        corner(box.x, box.y, 1, 1);
        corner(box.right(), box.y, -1, 1);
        corner(box.right(), box.bottom(), -1, -1);
        corner(box.x, box.bottom(), 1, -1);
        winrt::check_hresult(sink->Close());

        painter.context()->DrawGeometry(geometry.get(), painter.brush(tint), static_cast<float>(2 * zoom));
    }

    void drawRevealDecorations(Painter& painter, double now) {
        auto primary = color(palette.theme.primary);
        auto bright = mix(primary, rgba(255, 255, 255), 0.3f);

        for (auto const& reveal : scan->reveals) {
            double elapsed = now - reveal.start;
            if (elapsed < 0 || elapsed > REVEAL_MS + LABEL_TOTAL_MS) continue;

            double zoom = zoomOf(reveal.display);
            auto const& box = reveal.box;
            float alpha = static_cast<float>(progressOf(elapsed, 0, 120) * (1 - progressOf(elapsed, REVEAL_MS, 150)));

            if (alpha > 0) {
                double lock = easeOutCubic(progressOf(elapsed, 0, LOCK_MS));
                double scale = reducedMotion ? 1 : 1.25 - 0.25 * lock;
                double width = box.width * scale;
                double height = box.height * scale;
                Box frame{box.x + (box.width - width) / 2, box.y + (box.height - height) / 2, width, height};

                double spread = 2 * zoom;
                auto glow = rectOf(box.x - spread, box.y - spread, box.width + spread * 2, box.height + spread * 2);
                painter.fillRounded(glow, static_cast<float>(3 * zoom),
                                    withAlpha(primary, 0.2f * alpha * static_cast<float>(lock)));
                drawBrackets(painter, frame, zoom, withAlpha(bright, alpha));
            }

            float label = static_cast<float>(progressOf(elapsed, REVEAL_MS, 150) *
                                             (1 - progressOf(elapsed, REVEAL_MS + 150 + LABEL_MS, 250)));
            if (label > 0) drawKindLabel(painter, reveal, zoom, label);
        }
    }

    void drawKindLabel(Painter& painter, Reveal const& reveal, double zoom, float alpha) {
        auto text = t(KIND_KEYS[static_cast<size_t>(reveal.kind)]);
        float width = resources->measure(text, 11, DWRITE_FONT_WEIGHT_SEMI_BOLD) + 12;
        auto const& host = displays[reveal.display].local;
        auto const& box = reveal.box;

        double x = std::clamp(box.x, host.x + 4 * zoom, std::max(host.x + 4 * zoom, host.right() - (width + 4) * zoom));
        double y = box.y - 21 * zoom;
        if (y < host.y + 4 * zoom) y = box.bottom() + 6 * zoom;

        auto primary = color(palette.theme.primary);
        float scale = static_cast<float>(zoom);
        Transform place(painter.context(), D2D1::Matrix3x2F::Scale(scale, scale) *
                                               D2D1::Matrix3x2F::Translation(static_cast<float>(x), static_cast<float>(y)));
        painter.fillRounded(D2D1::RectF(0, 0, width, 18), 9, withAlpha(primary, alpha));
        painter.textAt(text, 6, painter.middleBaseline(9, 11), 11, DWRITE_FONT_WEIGHT_SEMI_BOLD,
                       withAlpha(color(palette.theme.primaryForeground), alpha));
    }

    std::wstring scrambled(std::wstring const& text, double progress, int key, double now) const {
        if (reducedMotion || progress >= 1) return text;

        std::wstring pool;
        for (wchar_t character : text)
            if (!std::iswspace(character) && !IS_HIGH_SURROGATE(character) && !IS_LOW_SURROGATE(character))
                pool += character;
        if (pool.empty()) return text;

        auto result = text;
        auto tick = static_cast<uint32_t>(now / 45);
        for (size_t index = 0; index < result.size(); index++) {
            if (pool.find(result[index]) == std::wstring::npos) continue;
            if (progress >= 0.1 + 0.8 * static_cast<double>(index) / result.size()) continue;

            uint32_t value = static_cast<uint32_t>(index) * 0x9e3779b9U ^ tick * 0x85ebca6bU ^
                             static_cast<uint32_t>(key + 7) * 0xc2b2ae35U;
            value ^= value >> 15;
            value *= 0x2c1b3c6dU;
            value ^= value >> 12;
            result[index] = pool[value % pool.size()];
        }
        return result;
    }

    void drawPill(Painter& painter, double now) {
        constexpr float HEIGHT = 40;
        constexpr float PADDING = 16;
        constexpr float ICON = 16;
        constexpr float GAP = 10;
        constexpr float FONT = 13;

        auto const& display = displays[scan->pill];
        double zoom = zoomOf(scan->pill);
        double elapsed = now - scan->start;
        bool hasResult = scan->result != ScanResult::Pending && now >= scan->resultAt;
        bool isUnavailable = scan->result == ScanResult::Unavailable;

        double opening = ease(Easing::Ease, progressOf(elapsed, 0, SCAN_OPEN_MS));
        double closing = hasResult ? progressOf(now, scan->holdUntil, isUnavailable ? BEAM_FADE_MS : PILL_OUT_MS) : 0;
        float alpha = static_cast<float>(opening * (1 - closing));
        if (alpha <= 0) return;

        double shift = 0;
        if (!reducedMotion) shift = -8 * (1 - opening) + (isUnavailable ? 0 : 8 * ease(Easing::Ease, closing));

        std::wstring current;
        std::wstring previous;
        double transition = 1;
        int key = -1;
        if (hasResult && !isUnavailable) {
            current = resultText();
            previous = scan->from;
            transition = progressOf(now, scan->resultAt, SCAN_TEXT_MS);
        } else if (hasResult) {
            current = scan->from;
        } else {
            key = static_cast<int>(elapsed / SCAN_PHASE_MS);
            current = phaseText(key);
            if (key > 0) {
                previous = phaseText(key - 1);
                transition = progressOf(elapsed, key * SCAN_PHASE_MS, SCAN_TEXT_MS);
            }
        }

        auto widthOf = [&](std::wstring const& text) {
            return PADDING + ICON + GAP + resources->measure(text, FONT, DWRITE_FONT_WEIGHT_MEDIUM) + PADDING + 2;
        };

        double eased = ease(Easing::Standard, transition);
        double width = widthOf(current);
        if (!previous.empty() && transition < 1) width = widthOf(previous) + (width - widthOf(previous)) * eased;

        auto const& theme = palette.theme;
        auto center = Point{display.local.x + display.local.width / 2, display.local.y + display.local.height / 2};
        auto foreground = color(theme.popoverForeground);

        painter.pushOpacity(alpha);
        {
            Transform place(painter.context(),
                            D2D1::Matrix3x2F::Scale(static_cast<float>(zoom), static_cast<float>(zoom)) *
                                D2D1::Matrix3x2F::Translation(static_cast<float>(center.x),
                                                              static_cast<float>(center.y + shift * zoom)));

            float half = static_cast<float>(width / 2);
            auto frame = D2D1::RectF(-half, -HEIGHT / 2, half, HEIGHT / 2);
            painter.boxShadow(frame, HEIGHT / 2, 10, 15, -3, 0.1f);
            painter.boxShadow(frame, HEIGHT / 2, 4, 6, -4, 0.1f);
            painter.fillRounded(frame, HEIGHT / 2, color(theme.popover));
            painter.ring(frame, HEIGHT / 2, 1, color(theme.foreground, palette.dark ? 0.1f : 0.05f));

            float iconX = frame.left + PADDING;
            float iconY = -ICON / 2;
            double swap = hasResult ? progressOf(now, scan->resultAt, 200) : 0;
            if (swap < 1)
                drawSpinner(painter, iconX, iconY, ICON, color(theme.primary, static_cast<float>(1 - swap)), now);

            if (swap > 0 && !isUnavailable) {
                bool isFailed = scan->result == ScanResult::Failed;
                auto warning = palette.dark ? rgba(251, 191, 36) : rgba(217, 119, 6);
                auto tint = isFailed                ? warning
                            : scan->reveals.empty() ? color(theme.mutedForeground)
                                                    : color(theme.primary);
                float grow = static_cast<float>(reducedMotion ? 1 : 0.6 + 0.4 * easeOutCubic(swap));
                Transform pop(painter.context(),
                              D2D1::Matrix3x2F::Scale(grow, grow, D2D1::Point2F(iconX + ICON / 2, 0)));
                painter.icon(isFailed ? icons::shieldAlert : icons::shieldCheck, iconX, iconY, ICON,
                             withAlpha(tint, static_cast<float>(swap)));
            }

            float textX = iconX + ICON + GAP;
            float baseline = painter.middleBaseline(0, FONT);
            auto clip = D2D1::RectF(textX - 2, frame.top + 1, frame.right - PADDING + 2, frame.bottom - 1);
            painter.context()->PushAxisAlignedClip(clip, D2D1_ANTIALIAS_MODE_ALIASED);

            if (!previous.empty() && transition < 1) {
                float distance = reducedMotion ? 0.0f : 10.0f;
                painter.textAt(previous, textX, baseline - distance * static_cast<float>(eased), FONT,
                               DWRITE_FONT_WEIGHT_MEDIUM, withAlpha(foreground, static_cast<float>(1 - eased)));
                painter.textAt(scrambled(current, transition, key, now), textX,
                               baseline + distance * static_cast<float>(1 - eased), FONT, DWRITE_FONT_WEIGHT_MEDIUM,
                               withAlpha(foreground, static_cast<float>(eased)));
            } else {
                painter.textAt(current, textX, baseline, FONT, DWRITE_FONT_WEIGHT_MEDIUM, foreground);
            }

            painter.context()->PopAxisAlignedClip();
        }
        painter.popOpacity();
    }

    void drawScan(Painter& painter, double now) {
        float alpha = static_cast<float>(ease(Easing::Ease, progressOf(now - scan->start, 0, SCAN_OPEN_MS)));
        if (scan->result != ScanResult::Pending)
            alpha *= static_cast<float>(1 - progressOf(now, scan->holdUntil, SCAN_FADE_MS));
        if (alpha > 0)
            for (auto const& display : displays) drawBorder(painter, display, now, alpha);

        drawRevealDecorations(painter, now);
        drawPill(painter, now);
    }

    void unlock() {
        scan->isUnlocked = true;
        annotator->hidden.clear();
        if (pointer) setHover(targetAt(*pointer));

        POINT position;
        GetCursorPos(&position);
        if (WindowFromPoint(position) != window) return;
        ScreenToClient(window, &position);
        SetCursor(cursorAt({position.x / ratio, position.y / ratio}));
    }

    void paint() {
        if (!gpu || !window) return;

        if (scan) {
            double now = clock();
            if (!scan->isUnlocked && !isLocked(now)) unlock();
            if (scan->isUnlocked && !isScanAnimating(now)) scan.reset();
        }

        auto context = surface.begin(*gpu);
        render(context);
        surface.present(*gpu);
    }

    bool isAnimating() {
        double now = nowMs();
        return isAnimatingHover || (hasToolbar() && toolbar->isAnimating(now)) || isScanAnimating(clock());
    }

    OverlayResult exportRegion(Box const& rect) {
        auto screen = toScreen(rect);
        OverlayResult result;
        result.rect = screen;

        if (isRecording()) {
            result.type = OverlayResultType::Area;
            return result;
        }

        annotator->finishTextEdit();
        annotator->hidden.clear();

        auto local = toLocal(screen);
        ScreenRect target{static_cast<int>(jsRound(local.x * ratio)), static_cast<int>(jsRound(local.y * ratio)),
                          std::max(1, static_cast<int>(jsRound(local.width * ratio))),
                          std::max(1, static_cast<int>(jsRound(local.height * ratio)))};

        auto context = gpu->context.get();
        com_ptr<ID2D1Bitmap1> bitmap;
        auto properties =
            D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,
                                    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        winrt::check_hresult(
            context->CreateBitmap(D2D1::SizeU(target.width, target.height), nullptr, 0, properties, bitmap.put()));

        context->SetTarget(bitmap.get());
        context->BeginDraw();
        context->SetTransform(D2D1::Matrix3x2F::Identity());
        context->Clear(D2D1::ColorF(0, 0, 0, 1));
        drawSourcePixels(context, target, rectOf(0, 0, target.width, target.height));

        if (hasToolbar() && !annotator->shapes.empty()) {
            context->SetTransform(
                D2D1::Matrix3x2F::Translation(static_cast<float>(-local.x), static_cast<float>(-local.y)) *
                D2D1::Matrix3x2F::Scale(static_cast<float>(ratio), static_cast<float>(ratio)));
            annotator->render(context, true);
        }

        winrt::check_hresult(context->EndDraw());
        context->SetTarget(nullptr);

        result.type = OverlayResultType::Region;
        result.width = target.width;
        result.height = target.height;
        result.bgra = readBitmap(context, bitmap.get(),
                                 D2D1::RectU(0, 0, static_cast<UINT32>(target.width), static_cast<UINT32>(target.height)));
        for (size_t index = 3; index < result.bgra.size(); index += 4) result.bgra[index] = 255;
        return result;
    }

    void finishRegion(Box const& rect) {
        OverlayResult result;
        try {
            result = exportRegion(rect);
        } catch (winrt::hresult_error const& error) {
            logError(L"Could not export the capture region: " + std::wstring(error.message()));
            result = {};
        }
        finish(std::move(result));
    }

    bool isLargeEnough(Box const& rect) const {
        auto physical = toPhysical(rect);
        return physical.width >= 2 && physical.height >= 2;
    }

    void cancelRegion() {
        regionDrag.reset();
        selection.reset();
        mode = Mode::Idle;
        if (pointer) setHover(targetAt(*pointer));
    }

    std::optional<std::string> pickColor(std::string const& current) {
        static COLORREF custom[16]{};
        auto parsed = parseHex(current);
        CHOOSECOLORW dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = window;
        dialog.rgbResult = RGB(std::lround(parsed.r * 255), std::lround(parsed.g * 255), std::lround(parsed.b * 255));
        dialog.lpCustColors = custom;
        dialog.Flags = CC_FULLOPEN | CC_RGBINIT;

        if (!ChooseColorW(&dialog)) return std::nullopt;
        return toHex(rgba(GetRValue(dialog.rgbResult), GetGValue(dialog.rgbResult), GetBValue(dialog.rgbResult)));
    }

    Point localPoint(LPARAM lParam) const {
        return {GET_X_LPARAM(lParam) / ratio, GET_Y_LPARAM(lParam) / ratio};
    }

    void invalidate() {
        if (window) InvalidateRect(window, nullptr, FALSE);
    }

    void updateIme() {
        bool editing = annotator->isEditingText();
        if (editing == isImeEnabled) return;

        isImeEnabled = editing;
        ImmAssociateContextEx(window, nullptr, editing ? IACE_DEFAULT : 0);
        if (editing) {
            SetTimer(window, CARET_TIMER, 50, nullptr);
        } else {
            KillTimer(window, CARET_TIMER);
        }
    }

    void positionIme() {
        if (!annotator->isEditingText()) return;

        float height = 0;
        auto caret = annotator->editorCaret(height);
        HIMC context = ImmGetContext(window);
        if (!context) return;

        COMPOSITIONFORM composition{};
        composition.dwStyle = CFS_POINT;
        composition.ptCurrentPos = {static_cast<LONG>(caret.x * ratio), static_cast<LONG>(caret.y * ratio)};
        ImmSetCompositionWindow(context, &composition);

        CANDIDATEFORM candidate{};
        candidate.dwIndex = 0;
        candidate.dwStyle = CFS_CANDIDATEPOS;
        candidate.ptCurrentPos = {static_cast<LONG>(caret.x * ratio), static_cast<LONG>((caret.y + height) * ratio)};
        ImmSetCandidateWindow(context, &candidate);
        ImmReleaseContext(window, context);
    }

    std::wstring compositionString(HIMC context, DWORD kind) {
        LONG bytes = ImmGetCompositionStringW(context, kind, nullptr, 0);
        if (bytes <= 0) return {};
        std::wstring text(static_cast<size_t>(bytes) / sizeof(wchar_t), L'\0');
        ImmGetCompositionStringW(context, kind, text.data(), bytes);
        return text;
    }

    void handlePointerMove(Point point, bool shift) {
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

    void handlePointerDown(Point point, bool shift) {
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

    void handlePointerUp(Point point) {
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

    void handlePointerCancel() {
        if (toolbarDrag) endToolbarDrag();
        if (regionDrag) cancelRegion();
        if (hasToolbar() && pointer) annotator->pointerUp(*pointer);
    }

    void handleRightClick(Point point) {
        if (regionDrag) return cancelRegion();
        if (annotator->isEditingText()) return;
        if (hasToolbar() && (toolbar->contains(point) || toolbar->popoverContains(point))) return;
        if (annotator->selectedId) return annotator->select(std::nullopt);
        if (annotator->tool) return annotator->setTool(std::nullopt);
        finish({});
    }

    void handleKeyDown(UINT key) {
        bool ctrl = GetKeyState(VK_CONTROL) < 0;
        bool shift = GetKeyState(VK_SHIFT) < 0;
        bool alt = GetKeyState(VK_MENU) < 0;

        if (annotator->isEditingText()) {
            annotator->editorKey(key, ctrl, shift);
            return;
        }

        if (hasToolbar() && toolbar->key(key, nowMs())) return;

        if (key == VK_ESCAPE) {
            if (regionDrag) return cancelRegion();
            if (annotator->selectedId) return annotator->select(std::nullopt);
            return finish({});
        }

        if (hasToolbar() && annotator->handleKey(key, ctrl, shift)) return;
        if (regionDrag) return;

        if (key == VK_SPACE) return finishRegion(viewport());

        if (key == VK_RETURN) {
            auto rect = pointer ? std::optional<Box>(targetAt(*pointer)) : hover;
            if (rect) finishRegion(*rect);
            return;
        }

        if (ctrl || alt || shift) return;

        int digit = 0;
        if (key >= '1' && key <= '9') digit = static_cast<int>(key - '0');
        if (key >= VK_NUMPAD1 && key <= VK_NUMPAD9) digit = static_cast<int>(key - VK_NUMPAD0);
        if (digit && digit <= static_cast<int>(displays.size())) finishRegion(displays[digit - 1].local);
    }

    void handleChar(wchar_t character) {
        if (annotator->isEditingText()) {
            annotator->editorChar(character);
            return;
        }

        if (!hasToolbar() || character <= 32) return;
        if (GetKeyState(VK_CONTROL) < 0 || GetKeyState(VK_MENU) < 0) return;

        if (std::towlower(character) == L'c') {
            annotator->setTool(std::nullopt);
            return;
        }

        annotator->handleShortcut(character);
    }

    HCURSOR cursorAt(Point point) {
        if (toolbarDrag) return LoadCursorW(nullptr, IDC_HAND);

        if (hasToolbar()) {
            if (toolbar->popoverContains(point)) return LoadCursorW(nullptr, IDC_ARROW);
            if (toolbar->contains(point))
                return LoadCursorW(nullptr, toolbar->isOverGrip(point) ? IDC_HAND : IDC_ARROW);

            if (auto anchor = annotator->anchorAt(point)) {
                switch (*anchor) {
                case Anchor::TopLeft:
                case Anchor::BottomRight:
                    return LoadCursorW(nullptr, IDC_SIZENWSE);
                case Anchor::TopRight:
                case Anchor::BottomLeft:
                    return LoadCursorW(nullptr, IDC_SIZENESW);
                case Anchor::Top:
                case Anchor::Bottom:
                    return LoadCursorW(nullptr, IDC_SIZENS);
                default:
                    return LoadCursorW(nullptr, IDC_SIZEWE);
                }
            }

            if (annotator->isEditingText() && annotator->editorContains(point)) return LoadCursorW(nullptr, IDC_IBEAM);

            if (annotator->tool == Tool::Select) return LoadCursorW(nullptr, IDC_ARROW);
            if (annotator->tool == Tool::Text) return LoadCursorW(nullptr, IDC_IBEAM);
            if (annotator->tool) return LoadCursorW(nullptr, IDC_CROSS);
            if (!regionDrag && annotator->isOverShape(point)) return LoadCursorW(nullptr, IDC_SIZEALL);
        }

        return LoadCursorW(nullptr, IDC_CROSS);
    }

    void destroyWindow() {
        if (window) {
            KillTimer(window, CARET_TIMER);
            SetWindowLongPtrW(window, GWLP_USERDATA, 0);
            DestroyWindow(window);
            window = nullptr;
        }

        scan.reset();
        surface.release();
        for (auto& display : displays) display.bitmap = nullptr;
        if (annotator) annotator->releaseDeviceResources();
        gpu.reset();
    }

    static LRESULT CALLBACK handleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == WM_NCCREATE) {
            auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            reinterpret_cast<Overlay*>(create->lpCreateParams)->window = window;
        }

        auto self = reinterpret_cast<Overlay*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (!self) return DefWindowProcW(window, message, wParam, lParam);

        try {
            return self->handle(message, wParam, lParam);
        } catch (winrt::hresult_error const& error) {
            logError(L"The capture overlay failed: " + std::wstring(error.message()));
            self->finish({});
            return 0;
        }
    }

    LRESULT handle(UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case WM_PAINT: {
            ValidateRect(window, nullptr);
            paint();
            if (window && isAnimating()) invalidate();
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_MOUSEMOVE: {
            if (!isTrackingLeave) {
                TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, window, 0};
                TrackMouseEvent(&track);
                isTrackingLeave = true;
            }
            if (isLocked(clock())) {
                pointer = localPoint(lParam);
                return 0;
            }
            handlePointerMove(localPoint(lParam), wParam & MK_SHIFT);
            invalidate();
            return 0;
        }
        case WM_MOUSELEAVE:
            isTrackingLeave = false;
            if (isLocked(clock())) {
                pointer.reset();
                return 0;
            }
            if (!regionDrag && !toolbarDrag && GetCapture() != window) {
                pointer.reset();
                setHover(std::nullopt);
                if (hasToolbar()) toolbar->pointerLeave(nowMs());
                invalidate();
            }
            return 0;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            if (isLocked(clock())) return 0;
            SetCapture(window);
            handlePointerDown(localPoint(lParam), wParam & MK_SHIFT);
            updateIme();
            invalidate();
            return 0;
        case WM_LBUTTONUP:
            if (isLocked(clock())) return 0;
            isReleasing = true;
            ReleaseCapture();
            isReleasing = false;
            handlePointerUp(localPoint(lParam));
            if (!window) return 0;
            updateIme();
            invalidate();
            return 0;
        case WM_CAPTURECHANGED:
            if (!isReleasing && reinterpret_cast<HWND>(lParam) != window) {
                handlePointerCancel();
                invalidate();
            }
            return 0;
        case WM_RBUTTONUP:
            if (isLocked(clock())) return 0;
            handleRightClick(localPoint(lParam));
            if (!window) return 0;
            invalidate();
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (message == WM_SYSKEYDOWN && wParam == VK_F4) return 0;
            if (isLocked(clock())) {
                if (wParam == VK_ESCAPE) finish({});
                return 0;
            }
            handleKeyDown(static_cast<UINT>(wParam));
            if (!window) return 0;
            updateIme();
            invalidate();
            return 0;
        case WM_CHAR:
            if (isLocked(clock())) return 0;
            handleChar(static_cast<wchar_t>(wParam));
            if (!window) return 0;
            updateIme();
            invalidate();
            return 0;
        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                if (isLocked(clock())) {
                    SetCursor(LoadCursorW(nullptr, IDC_APPSTARTING));
                    return TRUE;
                }
                POINT position;
                GetCursorPos(&position);
                ScreenToClient(window, &position);
                SetCursor(cursorAt({position.x / ratio, position.y / ratio}));
                return TRUE;
            }
            break;
        case WM_TIMER:
            if (wParam == CARET_TIMER) {
                bool visible = annotator->caretVisible();
                if (visible != wasCaretVisible) {
                    wasCaretVisible = visible;
                    invalidate();
                }
            }
            return 0;
        case WM_IME_SETCONTEXT:
            lParam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
            return DefWindowProcW(window, message, wParam, lParam);
        case WM_IME_STARTCOMPOSITION:
            positionIme();
            return 0;
        case WM_IME_COMPOSITION: {
            if (!annotator->isEditingText()) break;
            HIMC context = ImmGetContext(window);
            if (context) {
                if (lParam & GCS_RESULTSTR) annotator->editorCommit(compositionString(context, GCS_RESULTSTR));
                if (lParam & GCS_COMPSTR) annotator->editorComposition(compositionString(context, GCS_COMPSTR));
                ImmReleaseContext(window, context);
            }
            positionIme();
            invalidate();
            return 0;
        }
        case WM_IME_ENDCOMPOSITION:
            annotator->editorComposition(L"");
            invalidate();
            return 0;
        case WM_DPICHANGED:
            return 0;
        case WM_CLOSE:
            finish({});
            return 0;
        case WM_MOUSEACTIVATE:
            return MA_ACTIVATE;
        default:
            break;
        }

        return DefWindowProcW(window, message, wParam, lParam);
    }

    std::shared_ptr<OverlayRequest> request;
    OverlayCallback done;
    std::shared_ptr<Resources> resources;
    std::unique_ptr<Gpu> gpu;
    SwapSurface surface;
    Palette palette;
    ScreenRect bounds;
    double ratio = 1;
    std::vector<Display> displays;
    std::vector<Target> windows;
    OverlayPreferences preferences;
    OverlayPreferences loaded;
    std::unique_ptr<Annotator> annotator;
    std::unique_ptr<AnnotationToolbar> toolbar;
    HWND window = nullptr;

    Mode mode = Mode::Idle;
    std::optional<Box> selection;
    std::optional<Point> pointer;
    std::optional<Box> hover;
    std::optional<std::pair<Box, double>> hoverAnimation;
    std::optional<RegionDrag> regionDrag;
    std::optional<ToolbarDrag> toolbarDrag;
    bool isFinishing = false;
    bool isAnimatingHover = false;
    bool isTrackingLeave = false;
    bool isReleasing = false;
    bool isImeEnabled = false;
    bool wasCaretVisible = false;
    std::optional<Scan> scan;
    std::optional<double> frozen;
    bool reducedMotion = false;

    friend FixtureImage renderFixture(OverlayRequest const& request, FixtureScene const& scene);
    friend FixtureImage renderFixtureRegion(OverlayRequest const& request, FixtureScene const& scene,
                                            Box const& region);
};

}

void setUiResourcesPath(std::filesystem::path const& path) {
    setResourcesPath(path);
}

bool openOverlay(OverlayRequest request, OverlayCallback done) {
    bool expected = false;
    if (!isOpen.compare_exchange_strong(expected, true)) return false;

    auto shared = std::make_shared<OverlayRequest>(std::move(request));
    runOnUi([shared, done = std::move(done)]() mutable {
        Overlay* overlay = nullptr;

        try {
            overlay = new Overlay(shared, done);
            active = overlay;
            if (overlay->open()) return;
            logError(L"Could not create the capture overlay window");
        } catch (winrt::hresult_error const& error) {
            logError(L"Could not open the capture overlay: " + std::wstring(error.message()));
        } catch (std::exception const&) {
            logError(L"Could not open the capture overlay");
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

void deliverScan(uint64_t scanId, std::vector<Redaction> redactions, ScanOutcome outcome) {
    auto shared = std::make_shared<std::vector<Redaction>>(std::move(redactions));
    runOnUi([scanId, shared, outcome] {
        if (active) active->receiveScan(scanId, std::move(*shared), outcome);
    });
}

void finishScan(uint64_t scanId) {
    runOnUi([scanId] {
        if (active) active->completeScan(scanId);
    });
}

FixtureImage renderFixture(OverlayRequest const& request, FixtureScene const& scene) {
    auto shared = std::make_shared<OverlayRequest>(request);
    Overlay overlay(shared, nullptr);
    overlay.initializeGpu();
    return overlay.renderOffscreen(scene);
}

FixtureImage renderFixtureRegion(OverlayRequest const& request, FixtureScene const& scene, Box const& region) {
    auto shared = std::make_shared<OverlayRequest>(request);
    Overlay overlay(shared, nullptr);
    overlay.initializeGpu();
    return overlay.renderRegion(scene, region);
}

}

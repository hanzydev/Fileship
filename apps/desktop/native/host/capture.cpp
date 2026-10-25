#include "app.h"

#include <shared/log.h>

#include <atomic>
#include <mutex>
#include <set>

namespace fileship::host {

namespace {

struct PendingCapture {
    ui::OverlayPurpose purpose = ui::OverlayPurpose::Capture;
    AreaCallback onArea;
    std::function<void(core::Bitmap)> onImage;
};

std::mutex mutex;
std::set<DWORD> windowProcesses;
std::atomic<bool> isOpen = false;
std::optional<core::Rect> lastRegion;

BOOL CALLBACK collectWindow(HWND window, LPARAM parameter) {
    auto& state = *reinterpret_cast<std::pair<std::set<DWORD>, std::vector<uint64_t>>*>(parameter);
    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);

    if (state.first.contains(pid))
        state.second.push_back(static_cast<uint64_t>(reinterpret_cast<uintptr_t>(window)));

    return TRUE;
}

core::Bitmap fromOverlay(ui::OverlayResult const& result) {
    return core::Bitmap{result.bgra, static_cast<uint32_t>(result.width), static_cast<uint32_t>(result.height)};
}

void finishRegionCapture(PendingCapture const& pending, ui::OverlayResult const& result) {
    if (pending.purpose == ui::OverlayPurpose::Ocr) {
        if (result.type != ui::OverlayResultType::Region) return;
        playShutter();
        if (pending.onImage) pending.onImage(fromOverlay(result));
        return;
    }

    if (pending.purpose != ui::OverlayPurpose::Capture) {
        if (!pending.onArea || result.type == ui::OverlayResultType::Cancel) return;

        core::Rect area{result.rect.x, result.rect.y, result.rect.width, result.rect.height};
        auto rect = core::intersectRects(area, core::virtualScreen());
        if (rect && rect->width >= 2 && rect->height >= 2) pending.onArea(*rect);
        return;
    }

    if (result.type != ui::OverlayResultType::Region) return;

    core::Rect area{result.rect.x, result.rect.y, result.rect.width, result.rect.height};
    if (auto rect = core::intersectRects(area, core::virtualScreen())) {
        std::lock_guard lock(mutex);
        lastRegion = rect;
    }

    playShutter();
    handleImage(fromOverlay(result));
}

void recognize(core::Bitmap const& image) {
    BusyScope busy;

    try {
        auto png = core::encodeImage(image.pixels.data(), image.width, image.height, {});
        Multipart form;
        form.file("image", "capture.png", "image/png", reinterpret_cast<char const*>(png.data()), png.size());

        auto response = apiForm(L"/api/ai/ocr", form, 120000);
        auto text = response.ValueType() == JsonValueType::Object ? jsonString(response.GetObject(), L"text") : L"";

        if (text.find_first_not_of(L" \t\r\n") == std::wstring::npos) {
            notify(t(L"tasks.noTextFound"), t(L"tasks.selectLargerArea"));
            return;
        }

        writeClipboardText(text);
        notify(t(L"tasks.textCopied"), text.size() > 200 ? text.substr(0, 200) + L"…" : text);
    } catch (ApiError const& error) {
        if (error.status == 401) handleUnauthorized();

        if (error.status == 403) return notify(t(L"tasks.textCaptureOff"), t(L"tasks.aiOff"));
        if (error.status == 404) return notify(t(L"tasks.textCaptureUnavailable"), t(L"tasks.updateFileship"));
        if (error.status == 429) return notify(t(L"tasks.tooManyTextCaptures"), t(L"tasks.waitAMinute"));

        logWarn(L"Text capture failed: " + errorMessage(error));
        notify(t(L"tasks.textCaptureFailed"), errorMessage(error));
    } catch (std::exception const& error) {
        logWarn(L"Text capture failed: " + errorMessage(error));
        notify(t(L"tasks.textCaptureFailed"), errorMessage(error));
    }
}

}

void addWindowProcess(DWORD pid) {
    std::lock_guard lock(mutex);
    windowProcesses.insert(pid);
}

void removeWindowProcess(DWORD pid) {
    std::lock_guard lock(mutex);
    windowProcesses.erase(pid);
}

std::vector<uint64_t> ignoredWindows() {
    std::pair<std::set<DWORD>, std::vector<uint64_t>> state;

    {
        std::lock_guard lock(mutex);
        state.first = windowProcesses;
    }

    state.first.insert(GetCurrentProcessId());
    EnumWindows(collectWindow, reinterpret_cast<LPARAM>(&state));
    return state.second;
}

ui::ThemeMode themeMode() {
    auto theme = jsonString(settings(), L"theme", L"system");
    if (theme == L"light") return ui::ThemeMode::Light;
    if (theme == L"dark") return ui::ThemeMode::Dark;

    DWORD value = 0;
    DWORD size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                     L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS ||
        value)
        return ui::ThemeMode::Light;

    return ui::ThemeMode::Dark;
}

bool isCapturing() {
    return isOpen;
}

std::optional<Shot> captureRect(core::Rect const& rect, bool sound) {
    auto cursor = jsonBool(jsonObject(settings(), L"capture"), L"showCursor", true);
    auto image = core::captureRect(rect, cursor);
    if (!image) throw std::runtime_error("Could not capture the screen");

    if (sound) playShutter();
    return Shot{std::move(*image), {}};
}

std::optional<Shot> captureFullscreen() {
    return captureRect(core::virtualScreen());
}

std::optional<Shot> captureActiveMonitor() {
    POINT cursor{};
    GetCursorPos(&cursor);
    return captureRect(core::displayNearest(cursor).rect);
}

std::optional<Shot> captureActiveWindow() {
    auto window = core::foregroundWindow(ignoredWindows());
    if (!window) return std::nullopt;

    auto shot = captureRect(window->frame);
    shot->title = window->title;
    return shot;
}

void openRegionCapture(ui::OverlayPurpose purpose, AreaCallback onArea, std::function<void(core::Bitmap)> onImage) {
    if (isOpen.exchange(true)) return;

    PendingCapture pending{purpose, std::move(onArea), std::move(onImage)};

    try {
        auto capture = jsonObject(settings(), L"capture");
        auto displays = core::listDisplays();

        ui::OverlayRequest request;
        request.purpose = purpose;
        auto bounds = core::virtualScreen();
        request.bounds = {bounds.x, bounds.y, bounds.width, bounds.height};
        request.showMagnifier = jsonBool(capture, L"showMagnifier", true);
        request.theme = themeMode();
        GetCursorPos(&request.cursor);

        std::vector<core::Rect> rects;
        for (auto const& display : displays) {
            ui::DisplaySnapshot snapshot;
            snapshot.id = display.id;
            snapshot.rect = {display.rect.x, display.rect.y, display.rect.width, display.rect.height};
            snapshot.scaleFactor = display.scaleFactor;
            snapshot.isPrimary = display.isPrimary;
            snapshot.bgra.resize(static_cast<size_t>(display.rect.width) * display.rect.height * 4);
            request.displays.push_back(std::move(snapshot));
            rects.push_back(display.rect);
        }

        std::vector<uint8_t*> targets;
        for (auto& snapshot : request.displays) targets.push_back(snapshot.bgra.data());
        if (!core::captureRects(rects, false, targets)) throw std::runtime_error("Could not capture the screen");

        if (jsonBool(capture, L"detectWindows", true)) {
            for (auto& window : core::listWindows(ignoredWindows(), jsonBool(capture, L"detectControls", true))) {
                ui::WindowTarget target;
                target.rect = {window.frame.x, window.frame.y, window.frame.width, window.frame.height};
                target.title = std::move(window.title);
                for (auto const& control : window.controls)
                    target.controls.push_back({control.x, control.y, control.width, control.height});
                request.windows.push_back(std::move(target));
            }
        }

        auto shared = std::make_shared<PendingCapture>(std::move(pending));
        auto requestPointer = std::make_shared<ui::OverlayRequest>(std::move(request));

        postToMainThread([shared, requestPointer] {
            bool isOpened = ui::openOverlay(std::move(*requestPointer), [shared](ui::OverlayResult result) {
                isOpen = false;
                notifyIdle();
                runAsync([shared, result = std::move(result)] { finishRegionCapture(*shared, result); });
            });

            if (isOpened) return;

            isOpen = false;
            logError(L"Could not open region capture");
            notify(t(L"app.overlayFailed"), t(L"app.tryAgain"));
        });
    } catch (std::exception const& error) {
        logError(L"Could not open region capture: " + errorMessage(error));
        isOpen = false;
        notify(t(L"app.overlayFailed"), t(L"app.tryAgain"));
    }
}

void pickArea(ui::OverlayPurpose purpose, AreaCallback onArea) {
    openRegionCapture(purpose, std::move(onArea));
}

void captureLastRegion() {
    std::optional<core::Rect> region;

    {
        std::lock_guard lock(mutex);
        region = lastRegion;
    }

    if (!region) return openRegionCapture();
    handleImage(captureRect(*region)->image);
}

void captureText() {
    openRegionCapture(ui::OverlayPurpose::Ocr, {}, [](core::Bitmap image) { recognize(image); });
}

}

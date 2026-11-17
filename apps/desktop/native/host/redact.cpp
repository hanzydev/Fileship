#include "app.h"

#include <shared/log.h>
#include <ui/effects.h>

#include <algorithm>
#include <atomic>
#include <mutex>

namespace fileship::host {

namespace {

constexpr int REQUEST_TIMEOUT_MS = 20000;
constexpr int64_t UNAVAILABLE_MS = 30 * 60 * 1000;
constexpr size_t SCAN_WORKERS = 2;

constexpr std::pair<wchar_t const*, ui::RedactionKind> KINDS[] = {
    {L"secret", ui::RedactionKind::Secret},   {L"email", ui::RedactionKind::Email},
    {L"phone", ui::RedactionKind::Phone},     {L"card", ui::RedactionKind::Card},
    {L"account", ui::RedactionKind::Account}, {L"document", ui::RedactionKind::Document},
    {L"ip", ui::RedactionKind::Ip},
};

std::mutex mutex;
std::optional<int64_t> unavailableAt;
std::atomic<uint64_t> lastScanId = 0;

struct ScanSource {
    ui::ScreenRect rect;
    std::vector<uint8_t> bgra;
};

struct ScanJob {
    uint64_t id = 0;
    std::vector<ScanSource> sources;
    std::atomic<size_t> next = 0;
};

void setAiAvailable(bool available) {
    std::lock_guard lock(mutex);
    if (available) unavailableAt.reset();
    else unavailableAt = nowMs();
}

std::vector<ui::Redaction> parseRegions(IJsonValue const& response) {
    std::vector<ui::Redaction> regions;
    if (response.ValueType() != JsonValueType::Object) return regions;

    auto object = response.GetObject();
    if (!object.HasKey(L"regions") || object.GetNamedValue(L"regions").ValueType() != JsonValueType::Array)
        return regions;

    for (auto const& item : object.GetNamedArray(L"regions")) {
        if (item.ValueType() != JsonValueType::Object) continue;

        auto region = item.GetObject();
        auto kind = jsonString(region, L"kind");
        auto found = std::find_if(std::begin(KINDS), std::end(KINDS), [&](auto const& entry) { return kind == entry.first; });

        ui::Redaction redaction;
        redaction.rect = {static_cast<int>(jsonNumber(region, L"x")), static_cast<int>(jsonNumber(region, L"y")),
                          static_cast<int>(jsonNumber(region, L"width")),
                          static_cast<int>(jsonNumber(region, L"height"))};
        redaction.kind = found == std::end(KINDS) ? ui::RedactionKind::Secret : found->second;
        if (redaction.rect.width > 0 && redaction.rect.height > 0) regions.push_back(redaction);
    }

    return regions;
}

void runScanWorker(std::shared_ptr<ScanJob> const& job) {
    BusyScope busy;

    for (size_t index; (index = job->next++) < job->sources.size();) {
        auto& source = job->sources[index];
        auto png = core::encodeImage(source.bgra.data(), static_cast<uint32_t>(source.rect.width),
                                     static_cast<uint32_t>(source.rect.height), {});
        source.bgra = {};

        auto result = findRedactions(png);
        for (auto& region : result.regions) {
            region.rect.x += source.rect.x;
            region.rect.y += source.rect.y;
        }

        ui::deliverScan(job->id, std::move(result.regions), result.outcome);
    }
}

}

bool isAiAvailable() {
    if (auto user = currentUser(); user && !jsonBool(*user, L"aiEnabled", true)) return false;

    std::lock_guard lock(mutex);
    return !unavailableAt || nowMs() - *unavailableAt > UNAVAILABLE_MS;
}

void noteServerAi(bool enabled) {
    setAiAvailable(enabled);
}

void refreshServerAi() {
    try {
        auto value = api(L"GET", L"/api/settings/public");
        if (value.ValueType() == JsonValueType::Object) setAiAvailable(jsonBool(value.GetObject(), L"aiEnabled", true));
    } catch (std::exception const& error) {
        logWarn(L"Could not read the instance settings: " + errorMessage(error));
    }
}

bool isRedactionEnabled() {
    return isConnected() && jsonBool(jsonObject(settings(), L"capture"), L"redact", true) && isAiAvailable();
}

RedactResult findRedactions(std::vector<uint8_t> const& png) {
    try {
        Multipart form;
        form.file("image", "capture.png", "image/png", reinterpret_cast<char const*>(png.data()), png.size());
        return {ui::ScanOutcome::Found, parseRegions(apiForm(L"/api/ai/redact", form, REQUEST_TIMEOUT_MS))};
    } catch (ApiError const& error) {
        if (error.status == 401) handleUnauthorized();

        if (error.status == 401 || error.status == 403 || error.status == 404) {
            setAiAvailable(false);
            return {ui::ScanOutcome::Unavailable, {}};
        }

        logWarn(L"Could not scan the screenshot for sensitive information: " + errorMessage(error));
        return {ui::ScanOutcome::Failed, {}};
    } catch (std::exception const& error) {
        logWarn(L"Could not scan the screenshot for sensitive information: " + errorMessage(error));
        return {ui::ScanOutcome::Failed, {}};
    }
}

void applyRedactions(core::Bitmap& image, std::vector<ui::Redaction> const& regions) {
    uint32_t seed = static_cast<uint32_t>(nowMs());

    for (auto const& region : regions) {
        RECT rect{region.rect.x, region.rect.y, region.rect.x + region.rect.width, region.rect.y + region.rect.height};
        ui::redactFill(image.pixels.data(), static_cast<int>(image.width), static_cast<int>(image.height),
                       static_cast<int>(image.width) * 4, rect, seed = seed * 1664525u + 1013904223u);
    }
}

std::optional<uint64_t> newScanId() {
    if (!isRedactionEnabled()) return std::nullopt;
    return ++lastScanId;
}

void runScan(uint64_t id, std::vector<ui::DisplaySnapshot> displays, POINT cursor) {
    auto job = std::make_shared<ScanJob>();
    job->id = id;

    for (auto& display : displays) job->sources.push_back({display.rect, std::move(display.bgra)});

    std::stable_partition(job->sources.begin(), job->sources.end(), [&](ScanSource const& source) {
        return cursor.x >= source.rect.x && cursor.x < source.rect.x + source.rect.width &&
               cursor.y >= source.rect.y && cursor.y < source.rect.y + source.rect.height;
    });

    if (job->sources.empty()) return ui::finishScan(id);

    for (size_t index = 0; index < std::min(SCAN_WORKERS, job->sources.size()); index++)
        runAsync([job] { runScanWorker(job); });

    runAfter(REQUEST_TIMEOUT_MS + 2000, [id] { ui::finishScan(id); });
}

}

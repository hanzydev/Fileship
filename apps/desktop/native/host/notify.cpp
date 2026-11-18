#include "app.h"
#include "platform.h"

#include <shared/log.h>
#include <shared/platform.h>

#include <cmath>
#include <map>

namespace fileship::host {

namespace {

constexpr int64_t PROGRESS_DELAY_MS = 1000;
constexpr int64_t PROGRESS_INTERVAL_MS = 250;

std::mutex toastsMutex;
std::map<std::wstring, UploadToast*> toasts;

std::wstring remainingText(double seconds) {
    auto value = static_cast<int64_t>(std::ceil(seconds));
    if (value < 60) return t(L"tasks.uploadingSecondsLeft", {{L"count", std::to_wstring(std::max<int64_t>(value, 1))}});

    auto minutes = (value + 59) / 60;
    if (minutes < 60) return t(L"tasks.uploadingMinutesLeft", {{L"count", std::to_wstring(minutes)}});

    return t(L"tasks.uploadingHoursLeft", {{L"hours", std::to_wstring(minutes / 60)}, {L"minutes", std::to_wstring(minutes % 60)}});
}

}

UploadToast::UploadToast(std::wstring tag, std::wstring name)
    : tag(std::move(tag)), name(std::move(name)),
      isEnabled(jsonBool(jsonObject(settings(), L"afterUpload"), L"notify", true)) {
    std::lock_guard lock(toastsMutex);
    toasts[this->tag] = this;
}

UploadToast::~UploadToast() {
    close();
}

void UploadToast::show() {
    auto now = nowMs();
    auto elapsed = std::max<int64_t>((isPaused ? pausedAt : now) - startedAt, 1);
    auto speed = static_cast<double>(sent) * 1000 / elapsed;

    ProgressNotice progress;
    progress.tag = tag;
    progress.name = name;
    progress.sent = sent;
    progress.total = total;
    progress.isPaused = isPaused;
    progress.remaining = !isPaused && speed > 0 ? remainingText(static_cast<double>(total - sent) / speed)
                                                : std::to_wstring(sent * 100 / total) + L"%";
    progress.status = isPaused ? t(L"tasks.uploadPaused")
                               : t(L"tasks.uploadingStatus",
                                   {{L"sent", formatStorageSize(sent)},
                                    {L"total", formatStorageSize(total)},
                                    {L"speed", formatStorageSize(static_cast<int64_t>(speed))}});

    try {
        isEnabled = showUploadProgress(progress, !isShown || isRebuilt);
        isShown = true;
        isRebuilt = false;
    } catch (...) {
        isEnabled = false;
        logWarn(L"Could not show the upload progress: " + exceptionMessage());
    }
}

void UploadToast::update(int64_t sentBytes, int64_t totalBytes) {
    std::lock_guard lock(mutex);
    if (!isEnabled || isClosed || isPaused || sentBytes >= totalBytes) return;

    auto now = nowMs();
    if (!startedAt) startedAt = now;
    sent = sentBytes;
    total = totalBytes;

    if (now - startedAt < PROGRESS_DELAY_MS || now - updatedAt < PROGRESS_INTERVAL_MS) return;
    updatedAt = now;

    show();
}

void UploadToast::setPaused(bool paused) {
    std::lock_guard lock(mutex);
    if (isPaused == paused) return;

    auto now = nowMs();
    if (paused) pausedAt = now;
    else if (startedAt) startedAt += now - pausedAt;

    isPaused = paused;
    if (!isEnabled || isClosed || !isShown) return;

    isRebuilt = true;
    show();
}

void UploadToast::close() {
    {
        std::lock_guard lock(toastsMutex);
        auto found = toasts.find(tag);
        if (found != toasts.end() && found->second == this) toasts.erase(found);
    }

    std::lock_guard lock(mutex);
    if (isClosed) return;
    isClosed = true;
    if (!isShown) return;

    try {
        hideUploadProgress(tag);
    } catch (...) {
        logWarn(L"Could not remove the upload progress: " + exceptionMessage());
    }
}

void setUploadToastPaused(std::wstring const& tag, bool paused) {
    std::lock_guard lock(toastsMutex);
    auto found = toasts.find(tag);
    if (found != toasts.end()) found->second->setPaused(paused);
}

}

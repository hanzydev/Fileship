#include "app.h"

#include <host/harbor/support.h>
#include <shared/log.h>

#include <winrt/Windows.Data.Xml.Dom.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Notifications.h>

#include <cmath>
#include <deque>
#include <mutex>

namespace fileship::host {

namespace {

using winrt::Windows::Data::Xml::Dom::XmlDocument;
using winrt::Windows::UI::Notifications::NotificationData;
using winrt::Windows::UI::Notifications::NotificationUpdateResult;
using winrt::Windows::UI::Notifications::ToastNotification;
using winrt::Windows::UI::Notifications::ToastNotificationManager;
using winrt::Windows::UI::Notifications::ToastNotifier;

constexpr size_t KEEP_NOTIFICATIONS = 20;
constexpr int64_t PROGRESS_DELAY_MS = 1000;
constexpr int64_t PROGRESS_INTERVAL_MS = 250;
constexpr wchar_t const* PROGRESS_GROUP = L"uploads";
constexpr double SPEED_SMOOTHING = 0.3;

std::mutex mutex;
std::deque<ToastNotification> recent;

ToastNotifier notifier() {
    static ToastNotifier instance = ToastNotificationManager::CreateToastNotifier(appUserModelId());
    return instance;
}

std::wstring remainingText(double seconds) {
    auto value = static_cast<int64_t>(std::ceil(seconds));
    if (value < 60) return t(L"tasks.uploadingSecondsLeft", {{L"count", std::to_wstring(std::max<int64_t>(value, 1))}});

    auto minutes = (value + 59) / 60;
    if (minutes < 60) return t(L"tasks.uploadingMinutesLeft", {{L"count", std::to_wstring(minutes)}});

    return t(L"tasks.uploadingHoursLeft", {{L"hours", std::to_wstring(minutes / 60)}, {L"minutes", std::to_wstring(minutes % 60)}});
}

void registerDevelopmentId() {
    auto key = L"Software\\Classes\\AppUserModelId\\" + appUserModelId();
    std::wstring name = L"Fileship (dev)";
    RegSetKeyValueW(HKEY_CURRENT_USER, key.c_str(), L"DisplayName", REG_SZ, name.c_str(),
                    static_cast<DWORD>((name.size() + 1) * sizeof(wchar_t)));

    auto icon = (resourcesPath().parent_path() / L"icon.ico").wstring();
    RegSetKeyValueW(HKEY_CURRENT_USER, key.c_str(), L"IconUri", REG_SZ, icon.c_str(),
                    static_cast<DWORD>((icon.size() + 1) * sizeof(wchar_t)));
}

}

void initNotifications() {
    if (!isPackaged()) registerDevelopmentId();
}

void notify(std::wstring const& title, std::wstring const& body, std::function<void()> onClick) {
    try {
        XmlDocument document;
        document.LoadXml(L"<toast><visual><binding template=\"ToastGeneric\"><text/><text/></binding></visual></toast>");

        auto texts = document.GetElementsByTagName(L"text");
        texts.Item(0).AppendChild(document.CreateTextNode(title));
        texts.Item(1).AppendChild(document.CreateTextNode(body));

        ToastNotification toast(document);

        if (onClick) {
            toast.Activated([onClick](auto const&, auto const&) { runAsync(onClick); });
        }

        notifier().Show(toast);

        std::lock_guard lock(mutex);
        recent.push_back(toast);
        if (recent.size() > KEEP_NOTIFICATIONS) recent.pop_front();
    } catch (winrt::hresult_error const& error) {
        logWarn(L"Could not show a notification: " + std::wstring(error.message()));
    }
}

UploadToast::UploadToast(std::wstring tag, std::wstring name)
    : tag(std::move(tag)), name(std::move(name)),
      isEnabled(jsonBool(jsonObject(settings(), L"afterUpload"), L"notify", true)), startedAt(nowMs()),
      sampledAt(startedAt) {}

void UploadToast::update(int64_t sent, int64_t total) {
    std::lock_guard lock(mutex);
    if (!isEnabled || isClosed || sent >= total) return;

    auto now = nowMs();
    if (now - startedAt < PROGRESS_DELAY_MS || now - updatedAt < PROGRESS_INTERVAL_MS) return;
    updatedAt = now;

    auto measured = static_cast<double>(sent - sampledSent) * 1000 / std::max<int64_t>(now - sampledAt, 1);
    speed = speed > 0 ? SPEED_SMOOTHING * measured + (1 - SPEED_SMOOTHING) * speed : measured;
    sampledAt = now;
    sampledSent = sent;

    try {
        NotificationData data;
        data.SequenceNumber(++sequence);
        data.Values().Insert(L"progressValue", std::to_wstring(static_cast<double>(sent) / total));
        data.Values().Insert(L"progressValueString",
                             speed > 0 ? remainingText(static_cast<double>(total - sent) / speed)
                                       : std::to_wstring(sent * 100 / total) + L"%");
        data.Values().Insert(L"progressStatus",
                             t(L"tasks.uploadingStatus", {{L"sent", harbor::formatSize(sent)},
                                                          {L"total", harbor::formatSize(total)},
                                                          {L"speed", harbor::formatSize(static_cast<int64_t>(speed))}}));

        if (isShown) {
            if (notifier().Update(data, tag, PROGRESS_GROUP) != NotificationUpdateResult::Succeeded) isEnabled = false;
            return;
        }

        XmlDocument document;
        document.LoadXml(L"<toast><visual><binding template=\"ToastGeneric\"><text/><text/>"
                         L"<progress value=\"{progressValue}\" valueStringOverride=\"{progressValueString}\" "
                         L"status=\"{progressStatus}\"/></binding></visual>"
                         L"<audio silent=\"true\"/></toast>");

        auto texts = document.GetElementsByTagName(L"text");
        texts.Item(0).AppendChild(document.CreateTextNode(t(L"tasks.uploadingTitle")));
        texts.Item(1).AppendChild(document.CreateTextNode(name));

        ToastNotification toast(document);
        toast.Tag(tag);
        toast.Group(PROGRESS_GROUP);
        toast.Data(data);
        notifier().Show(toast);
        isShown = true;
    } catch (winrt::hresult_error const& error) {
        isEnabled = false;
        logWarn(L"Could not show the upload progress: " + std::wstring(error.message()));
    }
}

void UploadToast::close() {
    std::lock_guard lock(mutex);
    if (isClosed) return;
    isClosed = true;
    if (!isShown) return;

    try {
        ToastNotificationManager::History().Remove(tag, PROGRESS_GROUP, appUserModelId());
    } catch (winrt::hresult_error const& error) {
        logWarn(L"Could not remove the upload progress: " + std::wstring(error.message()));
    }
}

}

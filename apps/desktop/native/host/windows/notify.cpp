#include "host.h"

#include <host/platform.h>
#include <shared/log.h>

#include <winrt/Windows.Data.Xml.Dom.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Notifications.h>

#include <atomic>
#include <deque>
#include <mutex>

namespace fileship::host {

namespace {

using winrt::Windows::Data::Xml::Dom::XmlDocument;
using winrt::Windows::Data::Xml::Dom::XmlElement;
using winrt::Windows::UI::Notifications::NotificationData;
using winrt::Windows::UI::Notifications::NotificationUpdateResult;
using winrt::Windows::UI::Notifications::ToastActivatedEventArgs;
using winrt::Windows::UI::Notifications::ToastNotification;
using winrt::Windows::UI::Notifications::ToastNotificationManager;
using winrt::Windows::UI::Notifications::ToastNotifier;

constexpr size_t KEEP_NOTIFICATIONS = 20;
constexpr wchar_t const* PROGRESS_GROUP = L"uploads";

std::mutex mutex;
std::deque<ToastNotification> recent;

ToastNotifier notifier() {
    static ToastNotifier instance = ToastNotificationManager::CreateToastNotifier(appUserModelId());
    return instance;
}

std::wstring fileUri(std::filesystem::path const& path) {
    std::wstring result = L"file:///";
    for (auto character : path.wstring()) {
        if (character == L'\\') result += L'/';
        else if (character == L' ') result += L"%20";
        else if (character == L'#') result += L"%23";
        else if (character == L'%') result += L"%25";
        else result += character;
    }

    return result;
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

void notify(std::wstring const& title, std::wstring const& body, std::function<void()> onClick,
            std::optional<std::filesystem::path> const& image) {
    try {
        XmlDocument document;
        document.LoadXml(L"<toast><visual><binding template=\"ToastGeneric\"><text/><text/></binding></visual></toast>");

        auto texts = document.GetElementsByTagName(L"text");
        texts.Item(0).AppendChild(document.CreateTextNode(title));
        texts.Item(1).AppendChild(document.CreateTextNode(body));

        if (image) {
            auto element = document.CreateElement(L"image");
            element.SetAttribute(L"placement", L"hero");
            element.SetAttribute(L"src", fileUri(*image));
            document.GetElementsByTagName(L"binding").Item(0).AppendChild(element);
        }

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

bool showUploadProgress(ProgressNotice const& progress, bool isNew) {
    static std::atomic<uint32_t> sequence = 0;

    NotificationData data;
    data.SequenceNumber(++sequence);
    data.Values().Insert(L"progressValue", std::to_wstring(static_cast<double>(progress.sent) / progress.total));
    data.Values().Insert(L"progressValueString", progress.remaining);
    data.Values().Insert(L"progressStatus", progress.status);

    if (!isNew) return notifier().Update(data, progress.tag, PROGRESS_GROUP) == NotificationUpdateResult::Succeeded;

    XmlDocument document;
    document.LoadXml(L"<toast scenario=\"reminder\"><visual><binding template=\"ToastGeneric\"><text/><text/>"
                     L"<progress value=\"{progressValue}\" valueStringOverride=\"{progressValueString}\" "
                     L"status=\"{progressStatus}\"/></binding></visual><actions>"
                     L"<action activationType=\"foreground\" content=\"\"/>"
                     L"<action activationType=\"foreground\" arguments=\"cancel\" content=\"\"/>"
                     L"</actions><audio silent=\"true\"/></toast>");

    auto texts = document.GetElementsByTagName(L"text");
    texts.Item(0).AppendChild(document.CreateTextNode(t(L"tasks.uploadingTitle")));
    texts.Item(1).AppendChild(document.CreateTextNode(progress.name));

    auto actions = document.GetElementsByTagName(L"action");
    auto toggle = actions.Item(0).as<XmlElement>();
    toggle.SetAttribute(L"arguments", progress.isPaused ? L"resume" : L"pause");
    toggle.SetAttribute(L"content", t(progress.isPaused ? L"tasks.uploadingResume" : L"tasks.uploadingPause"));
    actions.Item(1).as<XmlElement>().SetAttribute(L"content", t(L"tasks.uploadingCancel"));

    ToastNotification toast(document);
    toast.Tag(progress.tag);
    toast.Group(PROGRESS_GROUP);
    toast.Data(data);
    toast.Activated([id = progress.tag](auto const&, winrt::Windows::Foundation::IInspectable const& args) {
        auto action = std::wstring(args.as<ToastActivatedEventArgs>().Arguments());
        runAsync([id, action] {
            if (action == L"pause") pauseUpload(id);
            else if (action == L"resume") resumeUpload(id);
            else if (action == L"cancel") cancelUpload(id);
        });
    });

    notifier().Show(toast);
    return true;
}

void hideUploadProgress(std::wstring const& tag) {
    ToastNotificationManager::History().Remove(tag, PROGRESS_GROUP, appUserModelId());
}

}

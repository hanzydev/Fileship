#include "app.h"

#include <shared/log.h>

#include <winrt/Windows.Data.Xml.Dom.h>
#include <winrt/Windows.UI.Notifications.h>

#include <deque>
#include <mutex>

namespace fileship::host {

namespace {

using winrt::Windows::Data::Xml::Dom::XmlDocument;
using winrt::Windows::UI::Notifications::ToastNotification;
using winrt::Windows::UI::Notifications::ToastNotificationManager;
using winrt::Windows::UI::Notifications::ToastNotifier;

constexpr size_t KEEP_NOTIFICATIONS = 20;

std::mutex mutex;
std::deque<ToastNotification> recent;

ToastNotifier notifier() {
    static ToastNotifier instance = ToastNotificationManager::CreateToastNotifier(appUserModelId());
    return instance;
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

}

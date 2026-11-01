#include "com.h"
#include "status.h"

#include <shellapi.h>

#include <winrt/Windows.Storage.Provider.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Windows.UI.h>

#include <algorithm>
#include <filesystem>
#include <limits>

using namespace winrt;
using namespace Windows::Data::Json;
using namespace Windows::Foundation;
using namespace Windows::Storage::Provider;
using namespace Windows::UI;
using namespace Windows::UI::ViewManagement;

namespace harbor {

CLSID const CLSID_FileshipStatus{0x5c54a548, 0x3a15, 0x4569, {0xb3, 0xcc, 0x9d, 0x14, 0x72, 0xcc, 0x86, 0x72}};

namespace {

struct Source;

struct StateInfo {
    StorageProviderState value;
    std::wstring label;
    wchar_t const* icon;
};

struct Action {
    wchar_t const* name;
    wchar_t const* label;
    wchar_t const* icon;
    wchar_t const* page;
};

Action const ACTIONS[] = {
    {L"home", L"openApp", L"app-window", nullptr},
    {L"upload", L"upload", L"upload", nullptr},
    {L"pause", L"pause", L"pause", nullptr},
    {L"resume", L"resume", L"play", nullptr},
    {L"retry", L"retry", L"rotate-cw", nullptr},
    {L"freeUpSpace", L"freeUpSpace", L"hard-drive-download", nullptr},
    {L"settings", L"settings", L"settings", nullptr},
    {L"trash", L"trash", L"trash", L"/dashboard/trash"},
    {L"storage", L"storage", L"hard-drive", L"/dashboard/settings/storage"},
};

std::mutex stateMutex;
std::wstring currentStatus;
std::vector<weak_ref<Source>> sources;

Uri iconUri(std::wstring const& name) {
    auto foreground = UISettings().GetColorValue(UIColorType::Foreground);
    auto theme = foreground.R > 128 ? L"dark" : L"light";
    auto path = std::filesystem::path(executablePath()).parent_path() / L"icons" / theme / (name + L".svg");
    return Uri(hstring(path.wstring()));
}

void openUrl(std::wstring const& url) {
    ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

struct Command : implements<Command, IStorageProviderUICommand> {
    hstring label;
    Uri icon;
    std::function<void()> action;

    Command(hstring text, Uri image, std::function<void()> run)
        : label(std::move(text)), icon(std::move(image)), action(std::move(run)) {}

    hstring Label() {
        return label;
    }

    hstring Description() {
        return label;
    }

    Uri Icon() {
        return icon;
    }

    StorageProviderUICommandState State() {
        return StorageProviderUICommandState::Enabled;
    }

    void Invoke() {
        action();
    }
};

IStorageProviderUICommand commandFor(std::wstring const& name) {
    auto found = std::ranges::find_if(ACTIONS, [&](Action const& action) { return name == action.name; });
    if (found == std::end(ACTIONS)) return nullptr;

    std::wstring page = found->page ? found->page : L"";

    return make<Command>(hstring(label(found->label)), iconUri(found->icon), [name, page] {
        if (page.empty()) requestAction(name, {});
        else openUrl(config().instanceUrl + page);
    });
}

StateInfo describe(std::wstring const& state) {
    if (state == L"ready") return {StorageProviderState::InSync, label(L"upToDate"), L"circle-check"};
    if (state == L"syncing" || state == L"starting") {
        return {StorageProviderState::Syncing, label(L"syncing"), L"refresh-cw"};
    }
    if (state == L"paused") return {StorageProviderState::Paused, label(L"paused"), L"circle-pause"};
    if (state == L"error") return {StorageProviderState::Error, label(L"error"), L"circle-alert"};
    return {StorageProviderState::Offline, label(L"offline"), L"cloud-off"};
}

StorageProviderQuotaUI quotaOf(JsonObject const& quota) {
    auto total = quota.GetNamedNumber(L"total");

    StorageProviderQuotaUI ui;
    ui.QuotaUsedInBytes(static_cast<uint64_t>(quota.GetNamedNumber(L"used")));
    ui.QuotaTotalInBytes(total > 0 ? static_cast<uint64_t>(total) : std::numeric_limits<uint64_t>::max());
    ui.QuotaUsedLabel(quota.GetNamedString(L"label"));
    if (quota.GetNamedBoolean(L"isNearlyFull")) ui.QuotaUsedColor(IReference<Color>(Color{255, 0xC4, 0x2B, 0x1C}));
    return ui;
}

struct Source : implements<Source, IStorageProviderStatusUISource> {
    ServerLock lock;
    event<TypedEventHandler<IStorageProviderStatusUISource, IInspectable>> changed;

    StorageProviderStatusUI GetStatusUI() {
        std::wstring text;

        {
            std::lock_guard guard(stateMutex);
            text = currentStatus;
        }

        requestStatus();

        auto status = text.empty() ? JsonObject() : JsonObject::Parse(text);
        std::wstring name(status.GetNamedString(L"state", L""));
        auto state = describe(name);
        auto dashboard = config().instanceUrl + L"/dashboard";

        StorageProviderStatusUI ui;
        ui.ProviderState(state.value);
        ui.ProviderStateLabel(state.label);
        ui.SyncStatusCommand(make<Command>(hstring(state.label), iconUri(state.icon),
                                           [] { requestAction(L"settings", {}); }));
        ui.ProviderPrimaryCommand(
            make<Command>(hstring(label(L"viewOnline")), iconUri(L"globe"), [dashboard] { openUrl(dashboard); }));

        std::vector<IStorageProviderUICommand> commands{commandFor(L"home")};

        if (!text.empty()) {
            commands.push_back(commandFor(L"upload"));
            commands.push_back(commandFor(name == L"paused" ? L"resume" : L"pause"));
        }

        commands.push_back(commandFor(L"trash"));
        commands.push_back(commandFor(L"settings"));
        ui.ProviderSecondaryCommands(single_threaded_vector(std::move(commands)));

        auto quota = status.GetNamedValue(L"quota", JsonValue::CreateNullValue());
        if (quota.ValueType() == JsonValueType::Object) ui.QuotaUI(quotaOf(quota.GetObject()));

        std::wstring message = text.empty() ? label(L"offlineMessage")
                                            : std::wstring(status.GetNamedString(L"message", L""));
        std::wstring action = text.empty() ? L"home" : std::wstring(status.GetNamedString(L"action", L""));

        if (!message.empty()) {
            StorageProviderMoreInfoUI moreInfo;
            moreInfo.Message(message);
            if (!action.empty()) moreInfo.Command(commandFor(action));
            ui.MoreInfoUI(moreInfo);
        }

        return ui;
    }

    event_token StatusUIChanged(TypedEventHandler<IStorageProviderStatusUISource, IInspectable> const& handler) {
        return changed.add(handler);
    }

    void StatusUIChanged(event_token const& token) noexcept {
        changed.remove(token);
    }
};

struct Factory : implements<Factory, IStorageProviderStatusUISourceFactory> {
    ServerLock lock;

    IStorageProviderStatusUISource GetStatusUISource(hstring const&) {
        auto source = make_self<Source>();

        {
            std::lock_guard guard(stateMutex);
            std::erase_if(sources, [](auto const& item) { return !item.get(); });
            sources.push_back(source->get_weak());
        }

        return *source;
    }
};

}

com_ptr<IClassFactory> statusFactory() {
    return classFactory<Factory>();
}

void setHarborStatus(JsonObject const& status) {
    std::wstring text(status.Stringify());
    std::vector<com_ptr<Source>> live;

    {
        std::lock_guard guard(stateMutex);
        if (text == currentStatus) return;

        currentStatus = text;
        for (auto const& item : sources) {
            if (auto source = item.get()) live.push_back(source);
        }
    }

    for (auto const& source : live) {
        try {
            source->changed(*source, nullptr);
        } catch (hresult_error const& error) {
            log(L"Could not refresh the status: " + std::wstring(error.message()));
        }
    }
}

}

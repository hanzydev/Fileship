#include "com.h"
#include "status.h"

#include <shellapi.h>

#include <winrt/Windows.Storage.Provider.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Storage::Provider;

namespace drive {

CLSID const CLSID_FileshipStatus{0x5c54a548, 0x3a15, 0x4569, {0xb3, 0xcc, 0x9d, 0x14, 0x72, 0xcc, 0x86, 0x72}};

namespace {

struct Source;

std::mutex stateMutex;
std::wstring currentState;
std::vector<weak_ref<Source>> sources;

struct Command : implements<Command, IStorageProviderUICommand> {
    hstring label;
    std::function<void()> action;

    Command(hstring text, std::function<void()> run) : label(std::move(text)), action(std::move(run)) {}

    hstring Label() {
        return label;
    }

    hstring Description() {
        return {};
    }

    Uri Icon() {
        return nullptr;
    }

    StorageProviderUICommandState State() {
        return StorageProviderUICommandState::Enabled;
    }

    void Invoke() {
        action();
    }
};

std::pair<StorageProviderState, std::wstring> describe(std::wstring const& state) {
    if (state == L"ready") return {StorageProviderState::InSync, label(L"upToDate")};
    if (state == L"syncing" || state == L"starting") return {StorageProviderState::Syncing, label(L"syncing")};
    if (state == L"error") return {StorageProviderState::Error, label(L"error")};
    return {StorageProviderState::Offline, label(L"offline")};
}

void openUrl(std::wstring const& url) {
    ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

struct Source : implements<Source, IStorageProviderStatusUISource> {
    ServerLock lock;
    event<TypedEventHandler<IStorageProviderStatusUISource, IInspectable>> changed;

    StorageProviderStatusUI GetStatusUI() {
        std::wstring state;

        {
            std::lock_guard guard(stateMutex);
            state = currentState;
        }

        auto [providerState, text] = describe(state);
        auto dashboard = config().instanceUrl + L"/dashboard";

        StorageProviderStatusUI ui;
        ui.ProviderState(providerState);
        ui.ProviderStateLabel(text);
        ui.ProviderPrimaryCommand(make<Command>(hstring(label(L"viewOnline")), [dashboard] { openUrl(dashboard); }));
        ui.ProviderSecondaryCommands(single_threaded_vector<IStorageProviderUICommand>(
            {make<Command>(hstring(label(L"settings")), [] { requestAction(L"settings", {}); })}));
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

void setDriveState(std::wstring const& state) {
    std::vector<com_ptr<Source>> live;

    {
        std::lock_guard guard(stateMutex);
        if (state == currentState) return;

        currentState = state;
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

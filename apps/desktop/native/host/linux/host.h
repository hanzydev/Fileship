#pragma once

#include <host/app.h>
#include <host/platform.h>

#include <shared/linux/portal.h>

namespace fileship::host {

constexpr char const* APP_ID = "dev.hanzy.fileship";

bool isWayland();
bool isHostQuitting();

struct Notice {
    std::wstring title;
    std::wstring body;
    std::vector<std::pair<std::string, std::wstring>> actions;
};

void initNotifications();
void registerShellMenu();
uint32_t showNotice(uint32_t replaces, Notice const& notice, std::function<void(std::string const&)> onAction);
void closeNotice(uint32_t id);

std::optional<uint32_t> keysymOf(std::wstring const& key);
std::vector<std::wstring> bindX11Hotkeys(std::vector<Hotkey> const& hotkeys);
std::vector<std::wstring> bindPortalHotkeys(std::vector<Hotkey> const& hotkeys);
uint32_t portalHotkeysVersion();
JsonObject portalHotkeyTriggers();
void configurePortalHotkeys();

}

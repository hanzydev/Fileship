#pragma once

#include <host/app.h>
#include <host/platform.h>

#include <gio/gio.h>

namespace fileship::host {

constexpr char const* APP_ID = "dev.hanzy.fileship";

GDBusConnection* sessionBus();
bool isWayland();
bool isHostQuitting();

void initNotifications();

std::optional<uint32_t> keysymOf(std::wstring const& key);
std::vector<std::wstring> bindX11Hotkeys(std::vector<Hotkey> const& hotkeys);
std::vector<std::wstring> bindPortalHotkeys(std::vector<Hotkey> const& hotkeys);
bool hasPortalHotkeys();
JsonObject portalHotkeyTriggers();
void configurePortalHotkeys();

}

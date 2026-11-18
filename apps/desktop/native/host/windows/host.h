#pragma once

#include <host/app.h>

#include <windows.h>

namespace fileship::host {

HWND mainWindow();
std::wstring mutexName();
std::wstring appUserModelId();

void migrateElectronStore();

void initNotifications();

void applyMenuTheme();
bool handleTrayMessage(UINT message, WPARAM wParam, LPARAM lParam);

void handleHotkey(int id);
bool isSnippingToolKeyEnabled();
JsonObject setSnippingToolKey(bool enabled);

void registerShellMenu();
void applyStartAtLogin(bool enabled);

namespace recording {

void downloadFfmpeg();

}

}

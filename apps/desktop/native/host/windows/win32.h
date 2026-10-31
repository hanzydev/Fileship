#pragma once

#include <windows.h>

#include <string>

namespace fileship::host {

HWND mainWindow();
std::wstring appUserModelId();
bool handleTrayMessage(UINT message, WPARAM wParam, LPARAM lParam);
void handleHotkey(int id);

}

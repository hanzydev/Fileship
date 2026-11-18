#pragma once

#include <functional>
#include <string>
#include <vector>

namespace fileship::host {

struct TrayItem {
    std::wstring label;
    std::function<void()> click;
    bool enabled = true;
    bool separator = false;
    std::vector<TrayItem> submenu;
};

std::vector<TrayItem> trayMenu();
std::wstring trayTooltip();
void handleTrayClick();

}

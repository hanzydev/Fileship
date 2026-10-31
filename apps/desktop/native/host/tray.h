#pragma once

#include <functional>
#include <string>
#include <vector>

namespace fileship::host {

struct MenuItem {
    std::wstring label;
    std::function<void()> click;
    bool enabled = true;
    bool separator = false;
    std::vector<MenuItem> submenu;
};

std::vector<MenuItem> trayMenu();
std::wstring trayTooltip();
void handleTrayClick();

}

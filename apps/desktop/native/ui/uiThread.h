#pragma once

#include <functional>

namespace fileship::ui {

void runOnUi(std::function<void()> task);
bool isUiThread();

}

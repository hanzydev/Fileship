#pragma once

#include <ui/recorderUi.h>

namespace fileship::ui {

void showRecorderWindows(ScreenRect area, ScreenRect toolbar, double scale, ThemeMode theme,
                         RecorderUiCallbacks callbacks, bool showBorder);

}

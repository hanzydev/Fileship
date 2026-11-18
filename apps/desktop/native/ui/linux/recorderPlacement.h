#pragma once

#include <ui/overlay.h>

#include <optional>
#include <vector>

namespace fileship::ui {

std::optional<ScreenRect> placeRecorderToolbar(ScreenRect const& area, ScreenRect const& monitor, double scale);
std::vector<ScreenRect> recorderBorders(ScreenRect const& area, ScreenRect const& screen, double scale);

}

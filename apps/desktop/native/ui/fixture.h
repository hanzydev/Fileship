#pragma once

#include "annotator.h"
#include "overlay.h"
#include "recorderUi.h"

#include <optional>
#include <string>
#include <vector>

namespace fileship::ui {

struct FixtureScene {
    std::optional<Point> pointer;
    std::optional<Tool> tool;
    std::vector<Shape> shapes;
    std::optional<std::string> selectedId;
    int popover = -1;
    std::optional<Box> selection;
};

struct FixtureImage {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> bgra;
};

FixtureImage renderFixture(OverlayRequest const& request, FixtureScene const& scene);
FixtureImage renderRecorderFixture(RecorderViewState const& state, ThemeMode theme, double scale);
FixtureImage renderFixtureRegion(OverlayRequest const& request, FixtureScene const& scene, Box const& region);

}

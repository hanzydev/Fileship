#pragma once

#include "annotator.h"
#include "widgets.h"

#include <cstdint>
#include <optional>

namespace fileship::ui {

enum class ToolbarEdge { None, Top, Bottom, Left, Right };

struct ToolbarPlacement {
    std::optional<int64_t> display;
    ToolbarEdge edge = ToolbarEdge::Top;
    double cx = 0.5;
    double cy = 0;
};

struct OverlayPreferences {
    ToolbarPlacement toolbar;
    AnnotationOptions annotations = defaultAnnotationOptions();
};

OverlayPreferences loadPreferences();
void savePreferences(OverlayPreferences const& preferences);

}

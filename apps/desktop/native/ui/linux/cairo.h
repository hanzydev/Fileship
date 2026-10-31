#pragma once

#include <ui/gfx.h>

#include <cairo.h>

namespace fileship::ui {

std::unique_ptr<Painter> createPainter(cairo_t* context, Resources& resources);
cairo_surface_t* surfaceOf(Image const& image);

}

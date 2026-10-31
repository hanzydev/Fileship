#pragma once

#include <core/core.h>

#include <X11/Xlib.h>

#include <functional>
#include <string>
#include <vector>

namespace fileship::core::x11 {

Display* openDisplay();
void closeDisplay(Display* display);
int syncErrors(Display* display);
bool withDisplay(std::function<void(Display*)> const& work);

Atom atom(Display* display, char const* name);
std::string atomName(Display* display, Atom value);
std::vector<long> longProperty(Display* display, Window window, Atom property, Atom type);
std::string textProperty(Display* display, Window window, Atom property, Atom type);
Rect rootRect(Display* display);

}

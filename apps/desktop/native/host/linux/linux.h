#pragma once

#include <gio/gio.h>

#include <filesystem>
#include <string>

namespace fileship::host {

GMainLoop* mainLoop();
GDBusConnection* sessionBus();
bool ensureGtk();
bool isFlatpak();
std::filesystem::path runtimeFolder();
std::filesystem::path xdgFolder(char const* variable, char const* fallback);
std::filesystem::path iconPath();
std::string shellQuote(std::string const& argument);

}

#pragma once

#include <gio/gio.h>

#include <filesystem>
#include <optional>
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
std::string packageType();
std::optional<std::filesystem::path> appImagePath();
void runOutsideAppImage(char** argv);
std::wstring hotkeyBackend();

}

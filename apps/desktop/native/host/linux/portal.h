#pragma once

#include <gio/gio.h>

#include <cstdint>
#include <functional>
#include <string>

namespace fileship::host {

constexpr char const* PORTAL_NAME = "org.freedesktop.portal.Desktop";
constexpr char const* PORTAL_PATH = "/org/freedesktop/portal/desktop";

using PortalParameters = std::function<GVariant*(char const* token)>;
using PortalResponse = std::function<void(uint32_t response, GVariant* results)>;

void registerPortalApp();
uint32_t portalVersion(char const* interface);
std::string portalToken();
void portalRequest(char const* interface, char const* method, PortalParameters const& parameters,
                   PortalResponse onResponse);

}

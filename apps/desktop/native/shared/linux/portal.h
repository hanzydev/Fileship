#pragma once

#include <gio/gio.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace fileship {

struct PortalReply {
    uint32_t response = 2;
    GVariant* results = nullptr;

    PortalReply() = default;
    PortalReply(PortalReply&& other) noexcept : response(other.response), results(other.results) {
        other.results = nullptr;
    }

    ~PortalReply() {
        if (results) g_variant_unref(results);
    }
};

constexpr char const* PORTAL_NAME = "org.freedesktop.portal.Desktop";
constexpr char const* PORTAL_PATH = "/org/freedesktop/portal/desktop";

using PortalParameters = std::function<GVariant*(char const* token)>;
using PortalResponse = std::function<void(uint32_t response, GVariant* results)>;

GDBusConnection* sessionBus();
void registerPortalApp(char const* appId);
uint32_t portalVersion(char const* interface);
std::string portalToken();
void portalRequest(char const* interface, char const* method, PortalParameters const& parameters,
                   PortalResponse onResponse);
PortalReply portalWait(char const* interface, char const* method, PortalParameters const& parameters);

}

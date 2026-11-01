#include "linux.h"

#include <host/app.h>
#include <host/tray.h>
#include <shared/log.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <unistd.h>

#include <map>

namespace fileship::host {

namespace {

constexpr char const* ITEM_PATH = "/StatusNotifierItem";
constexpr char const* MENU_PATH = "/MenuBar";
constexpr char const* ITEM_INTERFACE = "org.kde.StatusNotifierItem";
constexpr char const* MENU_INTERFACE = "com.canonical.dbusmenu";
constexpr char const* WATCHER = "org.kde.StatusNotifierWatcher";

constexpr char const* INTROSPECTION = R"xml(<node>
  <interface name="org.kde.StatusNotifierItem">
    <property name="Category" type="s" access="read"/>
    <property name="Id" type="s" access="read"/>
    <property name="Title" type="s" access="read"/>
    <property name="Status" type="s" access="read"/>
    <property name="WindowId" type="i" access="read"/>
    <property name="IconName" type="s" access="read"/>
    <property name="IconPixmap" type="a(iiay)" access="read"/>
    <property name="ToolTip" type="(sa(iiay)ss)" access="read"/>
    <property name="ItemIsMenu" type="b" access="read"/>
    <property name="Menu" type="o" access="read"/>
    <method name="ContextMenu"><arg name="x" type="i" direction="in"/><arg name="y" type="i" direction="in"/></method>
    <method name="Activate"><arg name="x" type="i" direction="in"/><arg name="y" type="i" direction="in"/></method>
    <method name="SecondaryActivate"><arg name="x" type="i" direction="in"/><arg name="y" type="i" direction="in"/></method>
    <method name="Scroll"><arg name="delta" type="i" direction="in"/><arg name="orientation" type="s" direction="in"/></method>
    <signal name="NewTitle"/>
    <signal name="NewIcon"/>
    <signal name="NewToolTip"/>
    <signal name="NewStatus"><arg name="status" type="s"/></signal>
  </interface>
  <interface name="com.canonical.dbusmenu">
    <property name="Version" type="u" access="read"/>
    <property name="TextDirection" type="s" access="read"/>
    <property name="Status" type="s" access="read"/>
    <property name="IconThemePath" type="as" access="read"/>
    <method name="GetLayout">
      <arg type="i" name="parentId" direction="in"/>
      <arg type="i" name="recursionDepth" direction="in"/>
      <arg type="as" name="propertyNames" direction="in"/>
      <arg type="u" name="revision" direction="out"/>
      <arg type="(ia{sv}av)" name="layout" direction="out"/>
    </method>
    <method name="GetGroupProperties">
      <arg type="ai" name="ids" direction="in"/>
      <arg type="as" name="propertyNames" direction="in"/>
      <arg type="a(ia{sv})" name="properties" direction="out"/>
    </method>
    <method name="GetProperty">
      <arg type="i" name="id" direction="in"/>
      <arg type="s" name="name" direction="in"/>
      <arg type="v" name="value" direction="out"/>
    </method>
    <method name="Event">
      <arg type="i" name="id" direction="in"/>
      <arg type="s" name="eventId" direction="in"/>
      <arg type="v" name="data" direction="in"/>
      <arg type="u" name="timestamp" direction="in"/>
    </method>
    <method name="EventGroup">
      <arg type="a(isvu)" name="events" direction="in"/>
      <arg type="ai" name="idErrors" direction="out"/>
    </method>
    <method name="AboutToShow">
      <arg type="i" name="id" direction="in"/>
      <arg type="b" name="needUpdate" direction="out"/>
    </method>
    <method name="AboutToShowGroup">
      <arg type="ai" name="ids" direction="in"/>
      <arg type="ai" name="updatesNeeded" direction="out"/>
      <arg type="ai" name="idErrors" direction="out"/>
    </method>
    <signal name="ItemsPropertiesUpdated">
      <arg type="a(ia{sv})" name="updatedProps"/>
      <arg type="a(ias)" name="removedProps"/>
    </signal>
    <signal name="LayoutUpdated">
      <arg type="u" name="revision"/>
      <arg type="i" name="parent"/>
    </signal>
    <signal name="ItemActivationRequested">
      <arg type="i" name="id"/>
      <arg type="u" name="timestamp"/>
    </signal>
  </interface>
</node>)xml";

struct Entry {
    MenuItem item;
    std::vector<int> children;
};

GDBusNodeInfo* node = nullptr;
guint itemRegistration = 0;
guint menuRegistration = 0;
guint ownership = 0;
guint watcher = 0;
std::string busName;
std::map<int, Entry> entries;
uint32_t revision = 1;
std::string tooltip;
bool hasWatcher = false;

int addEntries(std::vector<MenuItem> const& items, int& next) {
    int id = next++;
    for (auto const& item : items) {
        int child = item.submenu.empty() ? next++ : addEntries(item.submenu, next);
        entries[id].children.push_back(child);
        entries[child].item = item;
    }
    return id;
}

void buildMenu() {
    entries.clear();
    int next = 0;
    addEntries(trayMenu(), next);
    revision++;
}

GVariant* propertiesOf(int id) {
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

    auto const& entry = entries[id];
    if (id != 0) {
        auto const& item = entry.item;

        if (item.separator) {
            g_variant_builder_add(&builder, "{sv}", "type", g_variant_new_string("separator"));
        } else {
            std::string label;
            for (auto character : toUtf8(item.label)) {
                if (character == '_') label += '_';
                label += character;
            }

            g_variant_builder_add(&builder, "{sv}", "label", g_variant_new_string(label.c_str()));
            g_variant_builder_add(&builder, "{sv}", "enabled", g_variant_new_boolean(item.enabled));
        }
    }

    if (!entry.children.empty())
        g_variant_builder_add(&builder, "{sv}", "children-display", g_variant_new_string("submenu"));

    return g_variant_builder_end(&builder);
}

GVariant* layoutOf(int id, int depth) {
    GVariantBuilder children;
    g_variant_builder_init(&children, G_VARIANT_TYPE("av"));

    if (depth != 0) {
        for (int child : entries[id].children)
            g_variant_builder_add(&children, "v", layoutOf(child, depth < 0 ? depth : depth - 1));
    }

    return g_variant_new("(i@a{sv}av)", id, propertiesOf(id), &children);
}

GVariant* iconPixmaps() {
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a(iiay)"));

    auto source = gdk_pixbuf_new_from_file(iconPath().c_str(), nullptr);
    if (!source) return g_variant_builder_end(&builder);

    for (int size : {16, 22, 24, 32, 48, 64}) {
        auto scaled = gdk_pixbuf_scale_simple(source, size, size, GDK_INTERP_HYPER);
        auto channels = gdk_pixbuf_get_n_channels(scaled);
        auto stride = gdk_pixbuf_get_rowstride(scaled);
        auto pixels = gdk_pixbuf_read_pixels(scaled);

        std::vector<uint8_t> argb(static_cast<size_t>(size) * size * 4);
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                auto from = pixels + y * stride + x * channels;
                auto to = argb.data() + (static_cast<size_t>(y) * size + x) * 4;
                to[0] = channels == 4 ? from[3] : 255;
                to[1] = from[0];
                to[2] = from[1];
                to[3] = from[2];
            }
        }

        g_variant_builder_add(&builder, "(ii@ay)", size, size,
                              g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE, argb.data(), argb.size(), 1));
        g_object_unref(scaled);
    }

    g_object_unref(source);
    return g_variant_builder_end(&builder);
}

GVariant* cachedPixmaps() {
    static GVariant* pixmaps = g_variant_ref_sink(iconPixmaps());
    return pixmaps;
}

GVariant* emptyIds() {
    return g_variant_new_array(G_VARIANT_TYPE_INT32, nullptr, 0);
}

void runClick(int id) {
    auto found = entries.find(id);
    if (found == entries.end() || !found->second.item.click) return;
    postToMainThread(found->second.item.click);
}

GVariant* itemProperty(gchar const* name) {
    std::string_view property(name);
    if (property == "Category") return g_variant_new_string("ApplicationStatus");
    if (property == "Id") return g_variant_new_string("fileship");
    if (property == "Title") return g_variant_new_string("Fileship");
    if (property == "Status") return g_variant_new_string("Active");
    if (property == "WindowId") return g_variant_new_int32(0);
    if (property == "IconName") return g_variant_new_string("");
    if (property == "IconPixmap") return g_variant_ref(cachedPixmaps());
    if (property == "ItemIsMenu") return g_variant_new_boolean(FALSE);
    if (property == "Menu") return g_variant_new_object_path(MENU_PATH);
    if (property == "ToolTip") {
        return g_variant_new("(s@a(iiay)ss)", "", g_variant_new_array(G_VARIANT_TYPE("(iiay)"), nullptr, 0),
                             "Fileship", tooltip.c_str());
    }
    return nullptr;
}

GVariant* menuProperty(gchar const* name) {
    std::string_view property(name);
    if (property == "Version") return g_variant_new_uint32(3);
    if (property == "TextDirection") return g_variant_new_string("ltr");
    if (property == "Status") return g_variant_new_string("normal");
    if (property == "IconThemePath") return g_variant_new_strv(nullptr, 0);
    return nullptr;
}

void handleItemCall(GDBusConnection*, gchar const*, gchar const*, gchar const*, gchar const* method, GVariant*,
                    GDBusMethodInvocation* invocation, gpointer) {
    std::string_view name(method);
    if (name == "Activate") handleTrayClick();
    if (name == "SecondaryActivate") runAction(L"uploadClipboard");
    g_dbus_method_invocation_return_value(invocation, nullptr);
}

void handleMenuCall(GDBusConnection*, gchar const*, gchar const*, gchar const*, gchar const* method,
                    GVariant* parameters, GDBusMethodInvocation* invocation, gpointer) {
    std::string_view name(method);

    if (name == "GetLayout") {
        gint32 parent = 0;
        gint32 depth = -1;
        g_variant_get(parameters, "(ii@as)", &parent, &depth, nullptr);
        if (parent == 0 && entries.empty()) buildMenu();
        if (!entries.contains(parent)) parent = 0;
        g_dbus_method_invocation_return_value(invocation,
                                              g_variant_new("(u@(ia{sv}av))", revision, layoutOf(parent, depth)));
        return;
    }

    if (name == "GetGroupProperties") {
        GVariantIter* ids = nullptr;
        g_variant_get(parameters, "(ai@as)", &ids, nullptr);

        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("a(ia{sv})"));
        gint32 id = 0;
        while (g_variant_iter_next(ids, "i", &id)) {
            if (entries.contains(id)) g_variant_builder_add(&builder, "(i@a{sv})", id, propertiesOf(id));
        }
        g_variant_iter_free(ids);

        g_dbus_method_invocation_return_value(invocation, g_variant_new("(a(ia{sv}))", &builder));
        return;
    }

    if (name == "GetProperty") {
        gint32 id = 0;
        gchar const* property = nullptr;
        g_variant_get(parameters, "(i&s)", &id, &property);

        GVariant* value = nullptr;
        if (entries.contains(id)) {
            auto properties = g_variant_ref_sink(propertiesOf(id));
            value = g_variant_lookup_value(properties, property, nullptr);
            g_variant_unref(properties);
        }
        if (!value) value = g_variant_new_string("");
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(v)", value));
        return;
    }

    if (name == "Event") {
        gint32 id = 0;
        gchar const* event = nullptr;
        g_variant_get(parameters, "(i&s@vu)", &id, &event, nullptr, nullptr);
        if (std::string_view(event) == "clicked") runClick(id);
        g_dbus_method_invocation_return_value(invocation, nullptr);
        return;
    }

    if (name == "EventGroup") {
        GVariantIter* events = nullptr;
        g_variant_get(parameters, "(a(isvu))", &events);

        gint32 id = 0;
        gchar const* event = nullptr;
        while (g_variant_iter_next(events, "(i&svu)", &id, &event, nullptr, nullptr)) {
            if (std::string_view(event) == "clicked") runClick(id);
        }
        g_variant_iter_free(events);

        g_dbus_method_invocation_return_value(invocation, g_variant_new("(@ai)", emptyIds()));
        return;
    }

    if (name == "AboutToShow") {
        gint32 id = 0;
        g_variant_get(parameters, "(i)", &id);
        if (id == 0) buildMenu();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", id == 0));
        return;
    }

    if (name == "AboutToShowGroup") {
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(@ai@ai)", emptyIds(), emptyIds()));
        return;
    }

    g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD, "Unknown method");
}

GVariant* getItemProperty(GDBusConnection*, gchar const*, gchar const*, gchar const*, gchar const* name, GError**,
                          gpointer) {
    return itemProperty(name);
}

GVariant* getMenuProperty(GDBusConnection*, gchar const*, gchar const*, gchar const*, gchar const* name, GError**,
                          gpointer) {
    return menuProperty(name);
}

void registerWithWatcher() {
    g_dbus_connection_call(sessionBus(), WATCHER, "/StatusNotifierWatcher", WATCHER, "RegisterStatusNotifierItem",
                           g_variant_new("(s)", busName.c_str()), nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr,
                           nullptr, nullptr);
}

void emitItemSignal(char const* signal, GVariant* parameters = nullptr) {
    g_dbus_connection_emit_signal(sessionBus(), nullptr, ITEM_PATH, ITEM_INTERFACE, signal, parameters, nullptr);
}

}

void applyMenuTheme() {
    refreshTray();
}

void createTray() {
    if (!sessionBus() || itemRegistration) return;

    node = g_dbus_node_info_new_for_xml(INTROSPECTION, nullptr);
    tooltip = toUtf8(trayTooltip());
    busName = isFlatpak() ? g_dbus_connection_get_unique_name(sessionBus())
                          : "org.kde.StatusNotifierItem-" + std::to_string(getpid()) + "-1";

    static GDBusInterfaceVTable const itemTable{handleItemCall, getItemProperty, nullptr, {}};
    static GDBusInterfaceVTable const menuTable{handleMenuCall, getMenuProperty, nullptr, {}};

    itemRegistration = g_dbus_connection_register_object(sessionBus(), ITEM_PATH, node->interfaces[0], &itemTable,
                                                         nullptr, nullptr, nullptr);
    menuRegistration = g_dbus_connection_register_object(sessionBus(), MENU_PATH, node->interfaces[1], &menuTable,
                                                         nullptr, nullptr, nullptr);
    if (!isFlatpak()) {
        ownership = g_bus_own_name_on_connection(sessionBus(), busName.c_str(), G_BUS_NAME_OWNER_FLAGS_NONE, nullptr,
                                                 nullptr, nullptr, nullptr);
    }

    watcher = g_bus_watch_name_on_connection(
        sessionBus(), WATCHER, G_BUS_NAME_WATCHER_FLAGS_NONE,
        [](GDBusConnection*, gchar const*, gchar const*, gpointer) {
            hasWatcher = true;
            registerWithWatcher();
        },
        [](GDBusConnection*, gchar const*, gpointer) {
            if (hasWatcher) logWarn(L"The status notifier watcher went away");
            else logWarn(L"No status notifier watcher is running, the tray icon is not shown");
            hasWatcher = false;
        },
        nullptr, nullptr);
}

void removeTray() {
    if (!itemRegistration) return;

    g_bus_unwatch_name(watcher);
    if (ownership) g_bus_unown_name(ownership);
    ownership = 0;
    g_dbus_connection_unregister_object(sessionBus(), itemRegistration);
    g_dbus_connection_unregister_object(sessionBus(), menuRegistration);
    g_dbus_node_info_unref(node);
    itemRegistration = 0;
    menuRegistration = 0;
}

void refreshTray() {
    postToMainThread([] {
        if (!itemRegistration) return;

        tooltip = toUtf8(trayTooltip());
        emitItemSignal("NewToolTip");

        buildMenu();
        g_dbus_connection_emit_signal(sessionBus(), nullptr, MENU_PATH, MENU_INTERFACE, "LayoutUpdated",
                                      g_variant_new("(ui)", revision, 0), nullptr);
    });
}

}

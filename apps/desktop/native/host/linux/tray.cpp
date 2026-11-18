#include <host/app.h>
#include <host/tray.h>
#include <shared/log.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>

#include <unistd.h>

namespace fileship::host {

namespace {

constexpr char const* ITEM_PATH = "/StatusNotifierItem";
constexpr char const* ITEM_INTERFACE = "org.kde.StatusNotifierItem";
constexpr char const* MENU_PATH = "/MenuBar";
constexpr char const* MENU_INTERFACE = "com.canonical.dbusmenu";
constexpr char const* WATCHER = "org.kde.StatusNotifierWatcher";
constexpr int ICON_SIZE = 64;

constexpr char const* ITEM_XML = R"xml(<node><interface name="org.kde.StatusNotifierItem">
    <property name="Category" type="s" access="read"/>
    <property name="Id" type="s" access="read"/>
    <property name="Title" type="s" access="read"/>
    <property name="Status" type="s" access="read"/>
    <property name="IconName" type="s" access="read"/>
    <property name="IconPixmap" type="a(iiay)" access="read"/>
    <property name="ToolTip" type="(sa(iiay)ss)" access="read"/>
    <property name="ItemIsMenu" type="b" access="read"/>
    <property name="Menu" type="o" access="read"/>
    <method name="Activate"><arg type="i" direction="in"/><arg type="i" direction="in"/></method>
    <method name="SecondaryActivate"><arg type="i" direction="in"/><arg type="i" direction="in"/></method>
    <method name="ContextMenu"><arg type="i" direction="in"/><arg type="i" direction="in"/></method>
    <method name="Scroll"><arg type="i" direction="in"/><arg type="s" direction="in"/></method>
    <signal name="NewToolTip"/>
</interface></node>)xml";

constexpr char const* MENU_XML = R"xml(<node><interface name="com.canonical.dbusmenu">
    <property name="Version" type="u" access="read"/>
    <property name="TextDirection" type="s" access="read"/>
    <property name="Status" type="s" access="read"/>
    <property name="IconThemePath" type="as" access="read"/>
    <method name="GetLayout">
        <arg type="i" direction="in"/><arg type="i" direction="in"/><arg type="as" direction="in"/>
        <arg type="u" direction="out"/><arg type="(ia{sv}av)" direction="out"/>
    </method>
    <method name="GetGroupProperties">
        <arg type="ai" direction="in"/><arg type="as" direction="in"/><arg type="a(ia{sv})" direction="out"/>
    </method>
    <method name="GetProperty"><arg type="i" direction="in"/><arg type="s" direction="in"/><arg type="v" direction="out"/></method>
    <method name="Event">
        <arg type="i" direction="in"/><arg type="s" direction="in"/><arg type="v" direction="in"/><arg type="u" direction="in"/>
    </method>
    <method name="EventGroup"><arg type="a(isvu)" direction="in"/><arg type="ai" direction="out"/></method>
    <method name="AboutToShow"><arg type="i" direction="in"/><arg type="b" direction="out"/></method>
    <method name="AboutToShowGroup">
        <arg type="ai" direction="in"/><arg type="ai" direction="out"/><arg type="ai" direction="out"/>
    </method>
    <signal name="ItemsPropertiesUpdated"><arg type="a(ia{sv})"/><arg type="a(ias)"/></signal>
    <signal name="LayoutUpdated"><arg type="u"/><arg type="i"/></signal>
</interface></node>)xml";

struct MenuNode {
    int id = 0;
    TrayItem item;
    std::vector<MenuNode> children;
};

GDBusConnection* bus = nullptr;
std::string busName;
guint itemRegistration = 0;
guint menuRegistration = 0;
guint nameOwner = 0;
guint watcherWatch = 0;
MenuNode root;
GVariant* layout = nullptr;
uint32_t revision = 0;
GVariant* iconPixmap = nullptr;

std::vector<MenuNode> numbered(std::vector<TrayItem> const& items, int& nextId) {
    std::vector<MenuNode> nodes;

    for (auto const& item : items) {
        MenuNode node{nextId++, item, {}};
        node.children = numbered(item.submenu, nextId);
        nodes.push_back(std::move(node));
    }

    return nodes;
}

MenuNode const* findNode(MenuNode const& node, int id) {
    if (node.id == id) return &node;

    for (auto const& child : node.children) {
        if (auto found = findNode(child, id)) return found;
    }

    return nullptr;
}

std::string menuLabel(std::wstring const& label) {
    std::string result;
    for (char character : toUtf8(label)) result += character == '_' ? std::string("__") : std::string(1, character);
    return result;
}

void addProperties(GVariantBuilder& properties, MenuNode const& node) {
    if (node.item.separator) {
        g_variant_builder_add(&properties, "{sv}", "type", g_variant_new_string("separator"));
        return;
    }

    if (node.id) g_variant_builder_add(&properties, "{sv}", "label", g_variant_new_string(menuLabel(node.item.label).c_str()));
    if (!node.item.enabled) g_variant_builder_add(&properties, "{sv}", "enabled", g_variant_new_boolean(FALSE));
    if (!node.children.empty()) {
        g_variant_builder_add(&properties, "{sv}", "children-display", g_variant_new_string("submenu"));
    }
}

GVariant* layoutOf(MenuNode const& node, int depth) {
    GVariantBuilder properties;
    g_variant_builder_init(&properties, G_VARIANT_TYPE("a{sv}"));
    addProperties(properties, node);

    GVariantBuilder children;
    g_variant_builder_init(&children, G_VARIANT_TYPE("av"));
    if (depth) {
        for (auto const& child : node.children) g_variant_builder_add(&children, "v", layoutOf(child, depth - 1));
    }

    return g_variant_new("(ia{sv}av)", node.id, &properties, &children);
}

bool rebuildMenu() {
    int nextId = 1;
    root.children = numbered(trayMenu(), nextId);

    GVariant* current = g_variant_ref_sink(layoutOf(root, -1));
    bool isChanged = !layout || !g_variant_equal(layout, current);

    g_clear_pointer(&layout, g_variant_unref);
    layout = current;
    if (isChanged) revision++;

    return isChanged;
}

GVariant* propertiesOf(GVariant* ids) {
    GVariantBuilder result;
    g_variant_builder_init(&result, G_VARIANT_TYPE("a(ia{sv})"));

    GVariantIter iterator;
    gint32 id = 0;
    g_variant_iter_init(&iterator, ids);

    while (g_variant_iter_next(&iterator, "i", &id)) {
        auto node = findNode(root, id);
        if (!node) continue;

        GVariantBuilder properties;
        g_variant_builder_init(&properties, G_VARIANT_TYPE("a{sv}"));
        addProperties(properties, *node);
        g_variant_builder_add(&result, "(ia{sv})", id, &properties);
    }

    return g_variant_builder_end(&result);
}

void click(int id) {
    auto node = findNode(root, id);
    if (node && node->item.enabled && node->item.click) postToMainThread(node->item.click);
}

GVariant* tooltip() {
    GVariantBuilder pixmaps;
    g_variant_builder_init(&pixmaps, G_VARIANT_TYPE("a(iiay)"));
    return g_variant_new("(sa(iiay)ss)", "", &pixmaps, toUtf8(trayTooltip()).c_str(), "");
}

GVariant* loadIcon() {
    auto path = resourcesPath() / "icon.png";
    GdkPixbuf* image = gdk_pixbuf_new_from_file_at_size(path.c_str(), ICON_SIZE, ICON_SIZE, nullptr);
    int stride = gdk_pixbuf_get_rowstride(image);
    auto pixels = gdk_pixbuf_read_pixels(image);

    std::vector<uint8_t> argb;
    for (int y = 0; y < ICON_SIZE; y++) {
        for (int x = 0; x < ICON_SIZE; x++) {
            auto pixel = pixels + y * stride + x * 4;
            argb.insert(argb.end(), {pixel[3], pixel[0], pixel[1], pixel[2]});
        }
    }

    GVariantBuilder pixmaps;
    g_variant_builder_init(&pixmaps, G_VARIANT_TYPE("a(iiay)"));
    g_variant_builder_add(&pixmaps, "(ii@ay)", ICON_SIZE, ICON_SIZE,
                          g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE, argb.data(), argb.size(), 1));

    g_object_unref(image);
    return g_variant_ref_sink(g_variant_builder_end(&pixmaps));
}

GVariant* itemProperty(GDBusConnection*, gchar const*, gchar const*, gchar const*, gchar const* name, GError**, gpointer) {
    std::string_view property(name);

    if (property == "Category") return g_variant_new_string("ApplicationStatus");
    if (property == "Id") return g_variant_new_string("fileship");
    if (property == "Title") return g_variant_new_string("Fileship");
    if (property == "Status") return g_variant_new_string("Active");
    if (property == "IconName") return g_variant_new_string("");
    if (property == "IconPixmap") return g_variant_ref(iconPixmap);
    if (property == "ToolTip") return tooltip();
    if (property == "ItemIsMenu") return g_variant_new_boolean(FALSE);

    return g_variant_new_object_path(MENU_PATH);
}

void itemCall(GDBusConnection*, gchar const*, gchar const*, gchar const*, gchar const* name, GVariant*,
              GDBusMethodInvocation* invocation, gpointer) {
    std::string_view method(name);

    if (method == "Activate") handleTrayClick();
    if (method == "SecondaryActivate") runAction(L"uploadClipboard");

    g_dbus_method_invocation_return_value(invocation, nullptr);
}

GVariant* menuProperty(GDBusConnection*, gchar const*, gchar const*, gchar const*, gchar const* name, GError**, gpointer) {
    std::string_view property(name);

    if (property == "Version") return g_variant_new_uint32(3);
    if (property == "TextDirection") return g_variant_new_string("ltr");
    if (property == "Status") return g_variant_new_string("normal");

    return g_variant_new_strv(nullptr, 0);
}

void menuCall(GDBusConnection*, gchar const*, gchar const*, gchar const*, gchar const* name, GVariant* parameters,
              GDBusMethodInvocation* invocation, gpointer) {
    std::string_view method(name);
    GVariantBuilder none;
    g_variant_builder_init(&none, G_VARIANT_TYPE("ai"));

    if (method == "GetLayout") {
        gint32 parent = 0;
        gint32 depth = 0;
        g_variant_get(parameters, "(ii@as)", &parent, &depth, nullptr);

        auto node = findNode(root, parent);
        g_dbus_method_invocation_return_value(
            invocation, g_variant_new("(u@(ia{sv}av))", revision, layoutOf(node ? *node : root, depth)));
    } else if (method == "GetGroupProperties") {
        GVariant* ids = g_variant_get_child_value(parameters, 0);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(@a(ia{sv}))", propertiesOf(ids)));
        g_variant_unref(ids);
    } else if (method == "GetProperty") {
        g_dbus_method_invocation_return_dbus_error(invocation, "org.freedesktop.DBus.Error.InvalidArgs",
                                                   "Use GetGroupProperties");
    } else if (method == "Event") {
        gint32 id = 0;
        gchar const* event = nullptr;
        g_variant_get(parameters, "(i&s@vu)", &id, &event, nullptr, nullptr);

        if (std::string_view(event) == "clicked") click(id);
        g_dbus_method_invocation_return_value(invocation, nullptr);
    } else if (method == "EventGroup") {
        GVariantIter* events = nullptr;
        gint32 id = 0;
        gchar const* event = nullptr;
        g_variant_get(parameters, "(a(isvu))", &events);

        while (g_variant_iter_next(events, "(i&s@vu)", &id, &event, nullptr, nullptr)) {
            if (std::string_view(event) == "clicked") click(id);
        }

        g_variant_iter_free(events);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(ai)", &none));
    } else if (method == "AboutToShow") {
        gint32 id = 0;
        g_variant_get(parameters, "(i)", &id);

        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", !id && rebuildMenu()));
    } else {
        GVariantBuilder updates;
        g_variant_builder_init(&updates, G_VARIANT_TYPE("ai"));
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(aiai)", &updates, &none));
    }
}

GDBusInterfaceVTable const ITEM_CALLS{itemCall, itemProperty, nullptr, {}};
GDBusInterfaceVTable const MENU_CALLS{menuCall, menuProperty, nullptr, {}};

guint exportObject(char const* xml, char const* path, GDBusInterfaceVTable const& calls) {
    GDBusNodeInfo* info = g_dbus_node_info_new_for_xml(xml, nullptr);
    guint registration = g_dbus_connection_register_object(bus, path, info->interfaces[0], &calls, nullptr, nullptr, nullptr);
    g_dbus_node_info_unref(info);
    return registration;
}

void handleWatcher(GDBusConnection*, gchar const*, gchar const*, gpointer) {
    g_dbus_connection_call(bus, WATCHER, "/StatusNotifierWatcher", WATCHER, "RegisterStatusNotifierItem",
                           g_variant_new("(s)", busName.c_str()), nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr,
                           nullptr);
}

}

void createTray() {
    bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
    if (!bus) return logWarn(L"There is no session bus, the tray icon is not shown");

    busName = "org.kde.StatusNotifierItem-" + std::to_string(getpid()) + "-1";
    iconPixmap = loadIcon();
    rebuildMenu();

    itemRegistration = exportObject(ITEM_XML, ITEM_PATH, ITEM_CALLS);
    menuRegistration = exportObject(MENU_XML, MENU_PATH, MENU_CALLS);
    nameOwner = g_bus_own_name_on_connection(bus, busName.c_str(), G_BUS_NAME_OWNER_FLAGS_NONE, nullptr, nullptr, nullptr,
                                             nullptr);
    watcherWatch = g_bus_watch_name_on_connection(bus, WATCHER, G_BUS_NAME_WATCHER_FLAGS_NONE, handleWatcher, nullptr,
                                                  nullptr, nullptr);
}

void removeTray() {
    if (!bus) return;

    g_dbus_connection_flush_sync(bus, nullptr, nullptr);
    g_bus_unwatch_name(watcherWatch);
    g_bus_unown_name(nameOwner);
    g_dbus_connection_unregister_object(bus, itemRegistration);
    g_dbus_connection_unregister_object(bus, menuRegistration);
    g_clear_object(&bus);
}

void refreshTray() {
    postToMainThread([] {
        if (!bus) return;

        g_dbus_connection_emit_signal(bus, nullptr, ITEM_PATH, ITEM_INTERFACE, "NewToolTip", nullptr, nullptr);

        if (rebuildMenu()) {
            g_dbus_connection_emit_signal(bus, nullptr, MENU_PATH, MENU_INTERFACE, "LayoutUpdated",
                                          g_variant_new("(ui)", revision, 0), nullptr);
        }
    });
}

}

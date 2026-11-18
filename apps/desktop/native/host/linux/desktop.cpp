#include "host.h"
#include "portal.h"

#include <shared/log.h>

#include <fstream>

namespace fileship::host {

namespace {

std::filesystem::path autostartFile() {
    return std::filesystem::path(g_get_user_config_dir()) / "autostart" / (std::string(APP_ID) + ".desktop");
}

void applyStartAtLogin(bool enabled) {
    if (!isPackaged()) return;

    std::error_code error;
    auto file = autostartFile();

    if (!enabled) {
        std::filesystem::remove(file, error);
        return;
    }

    std::filesystem::create_directories(file.parent_path(), error);
    std::ofstream output(file, std::ios::trunc);
    output << "[Desktop Entry]\nType=Application\nName=Fileship\nExec=\"" << hostExecutable().string()
           << "\" --hidden\nIcon=" << (resourcesPath() / "icon.png").string() << "\nTerminal=false\n";

    if (!output.flush()) logError(L"Could not write the startup entry " + file.wstring());
}

}

void applyDesktopIntegration(JsonObject const& settings, bool) {
    applyStartAtLogin(jsonBool(settings, L"startAtLogin", true));
}

std::vector<std::filesystem::path> pickFiles() {
    std::vector<std::filesystem::path> paths;
    bool isDone = false;

    portalRequest(
        "org.freedesktop.portal.FileChooser", "OpenFile",
        [](char const* token) {
            GVariantBuilder options;
            g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
            g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string(token));
            g_variant_builder_add(&options, "{sv}", "multiple", g_variant_new_boolean(TRUE));

            return g_variant_new("(ssa{sv})", "", toUtf8(t(L"app.uploadDialogTitle")).c_str(), &options);
        },
        [&](uint32_t response, GVariant* results) {
            isDone = true;
            if (response) return;

            gchar const** uris = nullptr;
            g_variant_lookup(results, "uris", "^a&s", &uris);

            for (auto uri = uris; *uri; uri++) {
                gchar* path = g_filename_from_uri(*uri, nullptr, nullptr);
                paths.emplace_back(path);
                g_free(path);
            }

            g_free(uris);
        });

    while (!isDone && !isHostQuitting()) g_main_context_iteration(nullptr, TRUE);
    return paths;
}

}

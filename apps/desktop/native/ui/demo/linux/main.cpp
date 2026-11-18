#include <ui/demo/fixtures.h>
#include <ui/overlay.h>

#include <shared/i18n.h>
#include <shared/log.h>
#include <shared/utf8.h>

#include <cairo.h>

#include <algorithm>

using namespace fileship;
using namespace fileship::ui;
using namespace fileship::ui::demo;

void fileship::ui::demo::savePng(std::filesystem::path const& path, int width, int height,
                                 std::vector<uint8_t> const& bgra) {
    auto surface = cairo_image_surface_create_for_data(const_cast<unsigned char*>(bgra.data()), CAIRO_FORMAT_ARGB32,
                                                       width, height, width * 4);
    cairo_surface_write_to_png(surface, path.c_str());
    cairo_surface_destroy(surface);
}

int main(int count, char** arguments) {
    std::vector<std::string> args(arguments + 1, arguments + count);
    auto option = [&](std::string const& name, std::string const& fallback) {
        for (size_t index = 0; index + 1 < args.size(); index++)
            if (args[index] == name) return args[index + 1];
        return fallback;
    };

    std::filesystem::path resources = option("--resources", "../host");
    initLog(std::filesystem::current_path() / "overlay-demo.log");
    initI18n(resources / "i18n");
    setLanguage(toWide(option("--language", "en")));
    setUiResourcesPath(resources);
    setOverlayPreferencesStore({[] { return std::string(); }, [](std::string const&) {}});

    writeOverlayFixtures(option("--fixture", "fixtures"));
    return 0;
}

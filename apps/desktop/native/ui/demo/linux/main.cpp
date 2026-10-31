#include <ui/demo/fixtures.h>

#include <ui/fixture.h>
#include <ui/linux/gtk.h>
#include <ui/overlay.h>
#include <ui/recorderUi.h>

#include <shared/i18n.h>
#include <shared/log.h>
#include <shared/utf8.h>

#include <gtk/gtk.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

using namespace fileship;
using namespace fileship::ui;
using namespace fileship::ui::demo;

namespace fileship::ui::demo {

void print(std::string const& text) {
    static std::mutex mutex;
    std::lock_guard lock(mutex);
    fputs(text.c_str(), stdout);
    fputs("\n", stdout);
    fflush(stdout);
}

void savePng(std::filesystem::path const& path, int width, int height, std::vector<uint8_t> const& bgra) {
    std::vector<uint8_t> rgba(bgra.size());
    for (size_t index = 0; index + 3 < bgra.size(); index += 4) {
        rgba[index] = bgra[index + 2];
        rgba[index + 1] = bgra[index + 1];
        rgba[index + 2] = bgra[index];
        rgba[index + 3] = bgra[index + 3];
    }

    auto pixbuf = gdk_pixbuf_new_from_data(rgba.data(), GDK_COLORSPACE_RGB, TRUE, 8, width, height, width * 4, nullptr,
                                           nullptr);
    GError* error = nullptr;
    if (!gdk_pixbuf_save(pixbuf, path.c_str(), "png", &error, nullptr)) {
        print(std::string("could not save ") + path.string() + ": " + error->message);
        g_error_free(error);
    }
    g_object_unref(pixbuf);
}

}

namespace {

std::string memory() {
    std::ifstream file("/proc/self/status");
    std::string line;
    std::string result;
    while (std::getline(file, line)) {
        if (line.starts_with("VmRSS:") || line.starts_with("RssAnon:")) {
            if (!result.empty()) result += ", ";
            result += line.substr(0, line.find(':')) + " " + line.substr(line.find_first_not_of(" \t", line.find(':') + 1));
        }
    }
    return result;
}

OverlayPurpose parsePurpose(std::string const& value) {
    if (value == "ocr") return OverlayPurpose::Ocr;
    if (value == "video") return OverlayPurpose::Video;
    if (value == "gif") return OverlayPurpose::Gif;
    return OverlayPurpose::Capture;
}

OverlayRequest syntheticRequest(OverlayPurpose purpose, ThemeMode theme) {
    OverlayRequest request;
    request.purpose = purpose;
    request.theme = theme;

    auto monitors = gdk_display_get_monitors(gdk_display_get_default());
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;

    for (guint index = 0; index < g_list_model_get_n_items(monitors); index++) {
        auto monitor = GDK_MONITOR(g_list_model_get_item(monitors, index));
        GdkRectangle geometry{};
        gdk_monitor_get_geometry(monitor, &geometry);
        double scale = gdk_monitor_get_scale(monitor);
        g_object_unref(monitor);

        DisplaySnapshot display;
        display.id = static_cast<int64_t>(index) + 1;
        display.rect = {static_cast<int>(geometry.x * scale), static_cast<int>(geometry.y * scale),
                        static_cast<int>(geometry.width * scale), static_cast<int>(geometry.height * scale)};
        display.scaleFactor = scale;
        display.isPrimary = index == 0;
        display.bgra = fixtureSnapshot(display.rect.width, display.rect.height);

        if (index == 0 || display.rect.x < left) left = display.rect.x;
        if (index == 0 || display.rect.y < top) top = display.rect.y;
        right = std::max(right, display.rect.x + display.rect.width);
        bottom = std::max(bottom, display.rect.y + display.rect.height);
        request.displays.push_back(std::move(display));
    }

    request.bounds = {left, top, right - left, bottom - top};
    request.cursor = {left + 300, top + 330};

    WindowTarget first;
    first.rect = {left + 120, top + 100, 640, 460};
    first.controls = {{left + 160, top + 170, 260, 40}};
    WindowTarget second;
    second.rect = {left + 860, top + 180, 620, 580};
    request.windows = {first, second};
    return request;
}

int runOverlay(OverlayPurpose purpose, ThemeMode theme, std::filesystem::path const& output) {
    print("before capture: " + memory());

    auto request = syntheticRequest(purpose, theme);
    if (request.displays.empty()) {
        print("no monitors");
        return 1;
    }

    auto loop = g_main_loop_new(nullptr, FALSE);
    OverlayResult result;
    if (!openOverlay(std::move(request), [&](OverlayResult value) {
            result = std::move(value);
            g_main_loop_quit(loop);
        })) {
        print("openOverlay refused");
        return 1;
    }

    g_timeout_add_once(
        300, [](gpointer) { print("overlay open: " + memory()); }, nullptr);
    print("overlay requested");
    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    print("after close: " + memory());

    switch (result.type) {
    case OverlayResultType::Cancel:
        print("result: cancel");
        break;
    case OverlayResultType::Area:
        print("result: area " + std::to_string(result.rect.x) + "," + std::to_string(result.rect.y) + " " +
              std::to_string(result.rect.width) + "x" + std::to_string(result.rect.height));
        break;
    case OverlayResultType::Region:
        print("result: region " + std::to_string(result.rect.x) + "," + std::to_string(result.rect.y) + " " +
              std::to_string(result.width) + "x" + std::to_string(result.height));
        savePng(output, result.width, result.height, result.bgra);
        print("saved " + output.string());
        break;
    }

    return 0;
}

}

int main(int argc, char** argv) {
    std::locale::global(std::locale("C.UTF-8"));

    std::vector<std::string> args(argv + 1, argv + argc);
    auto option = [&](std::string const& name, std::string const& fallback = "") {
        for (size_t index = 0; index + 1 < args.size(); index++)
            if (args[index] == name) return args[index + 1];
        return fallback;
    };
    auto flag = [&](std::string const& name) { return std::find(args.begin(), args.end(), name) != args.end(); };

    std::filesystem::path resources = option("--resources", (executableDirectory() / ".." / "host").string());
    initLog(std::filesystem::current_path() / "overlay-demo.log");
    initI18n(resources / "i18n");
    setLanguage(toWide(option("--language", "en")));
    setUiResourcesPath(resources);

    auto preferences = executableDirectory() / "preferences.json";
    setOverlayPreferencesStore({
        [preferences] {
            std::ifstream file(preferences, std::ios::binary);
            std::stringstream content;
            content << file.rdbuf();
            return content.str();
        },
        [preferences](std::string const& value) {
            std::ofstream file(preferences, std::ios::binary);
            file << value;
            print("saved preferences " + value);
        },
    });

    auto theme = option("--theme", "dark") == "light" ? ThemeMode::Light : ThemeMode::Dark;

    try {
        if (flag("--fixture")) return runFixture(option("--fixture"));

        if (!initGtk()) {
            print("no display");
            return 1;
        }

        auto output = std::filesystem::path(option("--out", (executableDirectory() / "capture.png").string()));
        return runOverlay(parsePurpose(option("--purpose", "capture")), theme, output);
    } catch (std::exception const& error) {
        print(std::string("error: ") + error.what());
        return 1;
    }
}

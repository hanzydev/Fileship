#include "fixtures.h"

#include <ui/fixture.h>
#include <ui/overlay.h>
#include <ui/recorderUi.h>

#include <shared/utf8.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>

namespace fileship::ui::demo {

namespace {

uint8_t channel(double value) {
    return static_cast<uint8_t>(std::clamp(value, 0.0, 255.0));
}

std::vector<Shape> fixtureShapes() {
    std::vector<Shape> shapes;
    int id = 0;
    auto base = [&](Tool type, double x, double y) {
        Shape shape;
        shape.id = "f" + std::to_string(++id);
        shape.type = type;
        shape.x = x;
        shape.y = y;
        shape.color = "#ef4444";
        shape.shadow = true;
        return shape;
    };

    auto rectangle = base(Tool::Rectangle, 150, 160);
    rectangle.width = 300;
    rectangle.height = 70;
    rectangle.size = 4;
    shapes.push_back(rectangle);

    auto ellipse = base(Tool::Ellipse, 480, 330);
    ellipse.width = 180;
    ellipse.height = 110;
    ellipse.size = 6;
    ellipse.fill = true;
    ellipse.color = "#3b82f6";
    shapes.push_back(ellipse);

    auto arrow = base(Tool::Arrow, 900, 600);
    arrow.points = {0, 0, 220, -140};
    arrow.size = 4;
    shapes.push_back(arrow);

    auto line = base(Tool::Line, 900, 680);
    line.points = {0, 0, 300, 20};
    line.size = 8;
    line.color = "#22c55e";
    shapes.push_back(line);

    auto freehand = base(Tool::Freehand, 1250, 560);
    freehand.points = {0, 0, 20, -30, 50, -40, 80, -20, 100, 10, 130, 30, 160, 20, 180, -10};
    freehand.size = 5;
    freehand.color = "#a855f7";
    shapes.push_back(freehand);

    auto highlight = base(Tool::Highlight, 155, 235);
    highlight.width = 420;
    highlight.height = 22;
    highlight.color = "#facc15";
    highlight.shadow = false;
    shapes.push_back(highlight);

    auto text = base(Tool::Text, 170, 600);
    text.text = L"Fileship";
    text.size = 28;
    shapes.push_back(text);

    auto label = base(Tool::Text, 420, 600);
    label.text = L"Two lines\nwith background";
    label.size = 22;
    label.background = true;
    label.color = "#06b6d4";
    shapes.push_back(label);

    auto step = base(Tool::Step, 800, 140);
    step.size = 18;
    step.value = 1;
    shapes.push_back(step);

    auto second = base(Tool::Step, 800, 240);
    second.size = 24;
    second.value = 2;
    second.color = "#facc15";
    shapes.push_back(second);

    auto blur = base(Tool::Blur, 930, 310);
    blur.width = 120;
    blur.height = 140;
    blur.size = 12;
    shapes.push_back(blur);

    auto pixelate = base(Tool::Pixelate, 1070, 310);
    pixelate.width = 120;
    pixelate.height = 140;
    pixelate.size = 12;
    shapes.push_back(pixelate);

    auto eraser = base(Tool::Eraser, 180, 440);
    eraser.width = 100;
    eraser.height = 60;
    eraser.color = "#e65040";
    eraser.fill = true;
    eraser.shadow = false;
    shapes.push_back(eraser);

    auto emoji = base(Tool::Emoji, 1300, 230);
    emoji.text = L"\U0001F680";
    emoji.size = 64;
    emoji.color.clear();
    emoji.shadow = false;
    shapes.push_back(emoji);

    return shapes;
}

char const* toolName(Tool tool) {
    return TOOLS[static_cast<size_t>(tool)].name;
}

std::string escape(std::wstring const& value) {
    std::string result;
    for (char character : toUtf8(value)) {
        if (character == '\n') {
            result += "\\n";
        } else if (character == '"' || character == '\\') {
            result += '\\';
            result += character;
        } else {
            result += character;
        }
    }
    return result;
}

std::string shapeJson(Shape const& shape) {
    std::ostringstream out;
    out << "{\"id\":\"" << shape.id << "\",\"type\":\"" << toolName(shape.type) << "\",\"x\":" << shape.x
        << ",\"y\":" << shape.y << ",\"scaleX\":" << shape.scaleX << ",\"scaleY\":" << shape.scaleY;

    if (isBoxShape(shape.type))
        out << ",\"width\":" << shape.width << ",\"height\":" << shape.height << ",\"fill\":" << (shape.fill ? "true" : "false");
    if (isLineShape(shape.type)) {
        out << ",\"points\":[";
        for (size_t index = 0; index < shape.points.size(); index++) out << (index ? "," : "") << shape.points[index];
        out << "]";
    }
    if (shape.type == Tool::Text || shape.type == Tool::Emoji) out << ",\"text\":\"" << escape(shape.text) << "\"";
    if (shape.type == Tool::Text) out << ",\"background\":" << (shape.background ? "true" : "false");
    if (shape.type == Tool::Step) out << ",\"value\":" << shape.value;
    if (shape.type != Tool::Emoji)
        out << ",\"color\":\"" << shape.color << "\",\"shadow\":" << (shape.shadow ? "true" : "false");
    out << ",\"size\":" << shape.size << "}";
    return out.str();
}

}

std::vector<uint8_t> fixtureSnapshot(int width, int height) {
    std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
    auto set = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        size_t index = (static_cast<size_t>(y) * width + x) * 4;
        pixels[index] = b;
        pixels[index + 1] = g;
        pixels[index + 2] = r;
        pixels[index + 3] = 255;
    };

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double fx = static_cast<double>(x) / width;
            double fy = static_cast<double>(y) / height;
            set(x, y, channel(40 + 120 * fx), channel(70 + 90 * fy), channel(140 + 80 * (1 - fx)));
        }
    }

    auto box = [&](int left, int top, int right, int bottom, uint8_t r, uint8_t g, uint8_t b) {
        for (int y = top; y < bottom; y++)
            for (int x = left; x < right; x++) set(x, y, r, g, b);
    };

    box(120, 100, 760, 560, 245, 245, 247);
    box(120, 100, 760, 136, 225, 228, 235);
    box(160, 170, 420, 210, 66, 133, 244);
    box(160, 240, 720, 252, 180, 180, 190);
    box(160, 270, 680, 282, 180, 180, 190);
    box(160, 300, 700, 312, 180, 180, 190);
    box(860, 180, 1480, 760, 32, 34, 40);
    box(860, 180, 1480, 214, 52, 55, 64);

    for (int y = 420; y < 540; y++)
        for (int x = 160; x < 400; x++)
            if (((x / 12) + (y / 12)) % 2) set(x, y, 230, 80, 60);

    for (int y = 300; y < 460; y++)
        for (int x = 920; x < 1200; x++) {
            double value = 128 + 127 * std::sin(x * 0.11) * std::cos(y * 0.07);
            set(x, y, channel(value), channel(255 - value), channel(180));
        }

    return pixels;
}

int runFixture(std::filesystem::path const& directory) {
    std::filesystem::create_directories(directory);

    constexpr int WIDTH = 1600;
    constexpr int HEIGHT = 900;

    OverlayRequest request;
    request.purpose = OverlayPurpose::Capture;
    request.bounds = {0, 0, WIDTH, HEIGHT};
    request.cursor = {300, 330};

    DisplaySnapshot display;
    display.id = 1;
    display.rect = {0, 0, WIDTH, HEIGHT};
    display.scaleFactor = 1;
    display.isPrimary = true;
    display.bgra = fixtureSnapshot(WIDTH, HEIGHT);
    savePng(directory / L"snapshot.png", WIDTH, HEIGHT, display.bgra);
    request.displays.push_back(std::move(display));

    WindowTarget first;
    first.rect = {120, 100, 640, 460};
    first.controls = {{160, 170, 260, 40}};
    WindowTarget second;
    second.rect = {860, 180, 620, 580};
    request.windows = {first, second};

    auto shapes = fixtureShapes();

    struct Scene {
        char const* name;
        FixtureScene scene;
    };

    std::vector<Scene> scenes;

    FixtureScene hover;
    hover.pointer = Point{300, 330};
    hover.shapes = shapes;
    hover.selectedId = shapes[0].id;
    scenes.push_back({"hover", hover});

    FixtureScene drawing;
    drawing.pointer = Point{1000, 500};
    drawing.shapes = shapes;
    drawing.selection = Box{700, 380, 300, 120};
    scenes.push_back({"drawing", drawing});

    FixtureScene options;
    options.tool = Tool::Rectangle;
    options.shapes = shapes;
    options.popover = 6;
    scenes.push_back({"options", options});

    FixtureScene colors;
    colors.tool = Tool::Arrow;
    colors.shapes = shapes;
    colors.popover = 4;
    scenes.push_back({"colors", colors});

    FixtureScene emoji;
    emoji.tool = Tool::Emoji;
    emoji.shapes = shapes;
    emoji.popover = 5;
    scenes.push_back({"emoji", emoji});

    for (auto theme : {ThemeMode::Light, ThemeMode::Dark}) {
        request.theme = theme;
        auto suffix = theme == ThemeMode::Light ? "light" : "dark";

        for (auto const& [name, scene] : scenes) {
            auto image = renderFixture(request, scene);
            auto file = directory / (std::string("native-") + name + "-" + suffix + ".png");
            savePng(file, image.width, image.height, image.bgra);
            print("wrote " + toUtf8(file.wstring()));
        }
    }

    for (auto theme : {ThemeMode::Light, ThemeMode::Dark}) {
        auto suffix = theme == ThemeMode::Light ? "light" : "dark";
        RecorderViewState recording;
        recording.phase = RecorderPhase::Recording;
        recording.elapsedMs = 83000;
        recording.sources = {AudioSource::Microphone, AudioSource::System};
        RecorderViewState paused = recording;
        paused.phase = RecorderPhase::Paused;
        paused.muted = {AudioSource::Microphone};
        RecorderViewState preparing;
        preparing.phase = RecorderPhase::Preparing;

        std::pair<char const*, RecorderViewState> states[] = {
            {"recording", recording}, {"paused", paused}, {"preparing", preparing}};
        for (auto const& [name, state] : states) {
            auto image = renderRecorderFixture(state, theme, 1);
            savePng(directory / (std::string("native-recorder-") + name + "-" + suffix + ".png"), image.width,
                    image.height, image.bgra);
        }
    }

    auto region = renderFixtureRegion(request, hover, Box{140, 120, 700, 560});
    savePng(directory / L"native-region.png", region.width, region.height, region.bgra);

    std::ofstream json(directory / L"scene.json");
    json << "{\"width\":" << WIDTH << ",\"height\":" << HEIGHT << ",\"cursor\":{\"x\":300,\"y\":330},";
    json << "\"windows\":[{\"x\":120,\"y\":100,\"width\":640,\"height\":460,\"title\":\"\",\"controls\":[{\"x\":160,"
            "\"y\":170,\"width\":260,\"height\":40}]},{\"x\":860,\"y\":180,\"width\":620,\"height\":580,\"title\":\"\","
            "\"controls\":[]}],";
    json << "\"region\":{\"x\":140,\"y\":120,\"width\":700,\"height\":560},";
    json << "\"shapes\":[";
    for (size_t index = 0; index < shapes.size(); index++) json << (index ? "," : "") << shapeJson(shapes[index]);
    json << "]}";
    print("wrote scene.json");
    return 0;
}

}

#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace fileship::ui {

struct ScreenPoint {
    int x = 0;
    int y = 0;
};

struct ScreenRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

enum class ThemeMode { Light, Dark };

enum class OverlayPurpose { Capture, Ocr, Video, Gif };

struct DisplaySnapshot {
    int64_t id = 0;
    ScreenRect rect;
    double scaleFactor = 1;
    bool isPrimary = false;
    std::vector<uint8_t> bgra;
};

struct WindowTarget {
    ScreenRect rect;
    std::wstring title;
    std::vector<ScreenRect> controls;
};

struct OverlayRequest {
    OverlayPurpose purpose = OverlayPurpose::Capture;
    ScreenRect bounds;
    std::vector<DisplaySnapshot> displays;
    std::vector<WindowTarget> windows;
    ScreenPoint cursor;
    bool showMagnifier = true;
    ThemeMode theme = ThemeMode::Dark;
};

enum class OverlayResultType { Cancel, Area, Region };

struct OverlayResult {
    OverlayResultType type = OverlayResultType::Cancel;
    ScreenRect rect;
    int width = 0;
    int height = 0;
    std::vector<uint8_t> bgra;
};

using OverlayCallback = std::function<void(OverlayResult)>;

struct OverlayPreferencesStore {
    std::function<std::string()> load;
    std::function<void(std::string const&)> save;
};

void setOverlayPreferencesStore(OverlayPreferencesStore store);
void setUiResourcesPath(std::filesystem::path const& path);
bool openOverlay(OverlayRequest request, OverlayCallback done);
bool isOverlayOpen();
void closeOverlay();

}

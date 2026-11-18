#include "../test.h"

#include <ui/linux/recorderPlacement.h>

using namespace fileship::ui;

namespace {

constexpr ScreenRect SCREEN{0, 0, 1920, 1080};

std::wstring describe(ScreenRect const& rect) {
    return std::to_wstring(rect.x) + L"," + std::to_wstring(rect.y) + L" " + std::to_wstring(rect.width) + L"x" +
           std::to_wstring(rect.height);
}

std::wstring describe(std::optional<ScreenRect> const& rect) {
    return rect ? describe(*rect) : L"none";
}

std::wstring describe(std::vector<ScreenRect> const& rects) {
    std::wstring text;
    for (auto const& rect : rects) text += (text.empty() ? L"" : L" | ") + describe(rect);
    return text;
}

}

TEST(recorderToolbarSitsBelowTheArea) {
    CHECK_EQUAL(describe(placeRecorderToolbar({100, 100, 800, 600}, SCREEN, 1)), L"280,708 440x64");
}

TEST(recorderToolbarMovesAboveWithoutRoomBelow) {
    CHECK_EQUAL(describe(placeRecorderToolbar({100, 500, 800, 560}, SCREEN, 1)), L"280,428 440x64");
}

TEST(recorderToolbarIsLeftOutWhenItWouldCoverTheArea) {
    CHECK_EQUAL(describe(placeRecorderToolbar(SCREEN, SCREEN, 1)), L"none");
    CHECK_EQUAL(describe(placeRecorderToolbar({100, 40, 800, 1000}, SCREEN, 1)), L"none");
}

TEST(recorderToolbarStaysOnItsMonitor) {
    CHECK_EQUAL(describe(placeRecorderToolbar({0, 100, 200, 200}, SCREEN, 1)), L"0,308 440x64");
    CHECK_EQUAL(describe(placeRecorderToolbar({1800, 100, 120, 200}, SCREEN, 1)), L"1480,308 440x64");
    CHECK_EQUAL(describe(placeRecorderToolbar({2020, 100, 800, 600}, {1920, 0, 1280, 1024}, 1)), L"2200,708 440x64");
}

TEST(recorderToolbarFollowsTheScale) {
    CHECK_EQUAL(describe(placeRecorderToolbar({100, 100, 800, 600}, SCREEN, 2)), L"60,716 880x128");
}

TEST(recorderBordersSurroundTheArea) {
    CHECK_EQUAL(describe(recorderBorders({100, 100, 800, 600}, SCREEN, 1)),
                L"97,97 806x2 | 97,701 806x2 | 97,97 2x606 | 901,97 2x606");
}

TEST(recorderBordersFollowTheScale) {
    CHECK_EQUAL(describe(recorderBorders({100, 100, 800, 600}, SCREEN, 2)),
                L"94,94 812x4 | 94,702 812x4 | 94,94 4x612 | 902,94 4x612");
}

TEST(recorderBordersSkipTheSidesOffTheScreen) {
    CHECK_EQUAL(describe(recorderBorders(SCREEN, SCREEN, 1)), L"");
    CHECK_EQUAL(describe(recorderBorders({0, 100, 800, 600}, SCREEN, 1)),
                L"0,97 803x2 | 0,701 803x2 | 801,97 2x606");
}

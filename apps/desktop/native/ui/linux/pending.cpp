#include <ui/overlay.h>

namespace fileship::ui {

void setOverlayPreferencesStore(OverlayPreferencesStore) {}

void setUiResourcesPath(std::filesystem::path const&) {}

bool openOverlay(OverlayRequest, OverlayCallback) {
    return false;
}

bool isOverlayOpen() {
    return false;
}

void closeOverlay() {}

}

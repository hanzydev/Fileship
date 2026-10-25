#include "overlay.h"
#include "recorderUi.h"

namespace fileship::ui {

void setOverlayPreferencesStore(OverlayPreferencesStore) {}

bool openOverlay(OverlayRequest, OverlayCallback done) {
    done({});
    return true;
}

bool isOverlayOpen() {
    return false;
}

void closeOverlay() {}

void showRecorderUi(ScreenRect, ThemeMode, RecorderUiCallbacks) {}
void updateRecorderUi(RecorderViewState const&) {}
void hideRecorderUi() {}

}

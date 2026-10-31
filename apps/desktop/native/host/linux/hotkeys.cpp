#include <host/app.h>

namespace fileship::host {

JsonObject registerHotkeys() {
    return JsonObject();
}

JsonObject hotkeyStatus() {
    return JsonObject();
}

void pauseHotkeys() {}

void unregisterHotkeys() {}

bool isSnippingToolKeyEnabled() {
    return false;
}

JsonObject setSnippingToolKey(bool) {
    return registerHotkeys();
}

}

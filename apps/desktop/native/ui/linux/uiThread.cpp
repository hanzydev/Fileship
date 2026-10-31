#include "gtk.h"

#include <ui/uiThread.h>

#include <gtk/gtk.h>

namespace fileship::ui {

void runOnUi(std::function<void()> task) {
    g_idle_add_full(
        G_PRIORITY_DEFAULT_IDLE,
        [](gpointer data) {
            (*static_cast<std::function<void()>*>(data))();
            return G_SOURCE_REMOVE;
        },
        new std::function<void()>(std::move(task)), [](gpointer data) { delete static_cast<std::function<void()>*>(data); });
}

bool isUiThread() {
    return g_main_context_is_owner(g_main_context_default());
}

bool initGtk() {
    static bool const isReady = [] {
        g_setenv("GSK_RENDERER", "cairo", FALSE);
        return gtk_init_check();
    }();
    return isReady;
}

}

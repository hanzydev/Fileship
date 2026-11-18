#include <ui/uiThread.h>

#include <glib.h>

#include <memory>

namespace fileship::ui {

void runOnUi(std::function<void()> task) {
    g_idle_add_full(
        G_PRIORITY_DEFAULT,
        [](gpointer data) -> gboolean {
            std::unique_ptr<std::function<void()>> run(static_cast<std::function<void()>*>(data));
            (*run)();
            return G_SOURCE_REMOVE;
        },
        new std::function<void()>(std::move(task)), nullptr);
}

bool isUiThread() {
    return g_main_context_is_owner(g_main_context_default());
}

}

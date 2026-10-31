#include "linux.h"

#include <host/app.h>

#include <gtk/gtk.h>

namespace fileship::host {

std::vector<std::filesystem::path> pickFiles() {
    std::vector<std::filesystem::path> paths;
    if (!ensureGtk()) return paths;

    struct State {
        GMainLoop* loop = g_main_loop_new(nullptr, FALSE);
        std::vector<std::filesystem::path>* paths;
    } state{.paths = &paths};

    auto dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, toUtf8(t(L"app.uploadDialogTitle")).c_str());

    gtk_file_dialog_open_multiple(
        dialog, nullptr, nullptr,
        [](GObject* source, GAsyncResult* result, gpointer data) {
            auto& state = *static_cast<State*>(data);
            auto files = gtk_file_dialog_open_multiple_finish(GTK_FILE_DIALOG(source), result, nullptr);

            if (files) {
                for (guint index = 0; index < g_list_model_get_n_items(files); index++) {
                    auto file = G_FILE(g_list_model_get_item(files, index));
                    if (auto path = g_file_get_path(file)) {
                        state.paths->emplace_back(path);
                        g_free(path);
                    }
                    g_object_unref(file);
                }
                g_object_unref(files);
            }

            g_main_loop_quit(state.loop);
        },
        &state);

    g_main_loop_run(state.loop);
    g_main_loop_unref(state.loop);
    g_object_unref(dialog);
    return paths;
}

}

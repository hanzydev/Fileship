#include "linux.h"

#include <host/app.h>

#include <gtk/gtk.h>

namespace fileship::host {

bool confirmDialog(std::wstring const& message, std::wstring const& detail, std::wstring const& accept,
                   std::wstring const& cancel) {
    return runOnMainThread<bool>([&] {
        if (!ensureGtk()) return false;

        struct State {
            GMainLoop* loop = g_main_loop_new(nullptr, FALSE);
            int pressed = 1;
        } state;

        auto dialog = gtk_alert_dialog_new("%s", toUtf8(message).c_str());
        gtk_alert_dialog_set_detail(dialog, toUtf8(detail).c_str());
        auto acceptLabel = toUtf8(accept);
        auto cancelLabel = toUtf8(cancel);
        char const* buttons[] = {acceptLabel.c_str(), cancelLabel.c_str(), nullptr};
        gtk_alert_dialog_set_buttons(dialog, buttons);
        gtk_alert_dialog_set_default_button(dialog, 0);
        gtk_alert_dialog_set_cancel_button(dialog, 1);

        gtk_alert_dialog_choose(
            dialog, nullptr, nullptr,
            [](GObject* source, GAsyncResult* result, gpointer data) {
                auto& state = *static_cast<State*>(data);
                state.pressed = gtk_alert_dialog_choose_finish(GTK_ALERT_DIALOG(source), result, nullptr);
                g_main_loop_quit(state.loop);
            },
            &state);

        g_main_loop_run(state.loop);
        g_main_loop_unref(state.loop);
        g_object_unref(dialog);
        return state.pressed == 0;
    });
}

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

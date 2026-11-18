#include <host/harbor/platform.h>

#include <host/app.h>
#include <host/platform.h>
#include <shared/i18n.h>
#include <shared/utf8.h>
#include <ui/uiThread.h>

#include <gtk/gtk.h>
#include <sys/stat.h>

#include <future>

namespace fileship::host::harbor {

std::filesystem::path helperPath() {
    return resourcesPath().parent_path() / "harbor" / "fileship-harbor";
}

std::filesystem::path homeFolder() {
    return g_get_home_dir();
}

std::wstring stampOf(std::filesystem::path const& path) {
    struct stat info {};
    if (stat(path.c_str(), &info)) return L"";

    auto time = static_cast<int64_t>(info.st_mtim.tv_sec) * 1000000000 + info.st_mtim.tv_nsec;
    return std::to_wstring(info.st_size) + L":" + std::to_wstring(time);
}

bool confirmPublic(std::wstring const& message, std::wstring const& detail) {
    std::promise<bool> answer;
    auto result = answer.get_future();

    ui::runOnUi([&] {
        if (!gtk_init_check()) return answer.set_value(false);

        auto makePublic = toUtf8(t(L"harbor.makePublic"));
        auto cancel = toUtf8(t(L"harbor.cancel"));
        char const* buttons[] = {makePublic.c_str(), cancel.c_str(), nullptr};

        auto dialog = gtk_alert_dialog_new("%s", toUtf8(message).c_str());
        gtk_alert_dialog_set_detail(dialog, toUtf8(detail).c_str());
        gtk_alert_dialog_set_buttons(dialog, buttons);
        gtk_alert_dialog_set_default_button(dialog, 0);
        gtk_alert_dialog_set_cancel_button(dialog, 1);
        gtk_alert_dialog_choose(
            dialog, nullptr, nullptr,
            [](GObject* source, GAsyncResult* chosen, gpointer data) {
                int pressed = gtk_alert_dialog_choose_finish(GTK_ALERT_DIALOG(source), chosen, nullptr);
                static_cast<std::promise<bool>*>(data)->set_value(pressed == 0);
                g_object_unref(source);
            },
            &answer);
    });

    return result.get();
}

void openFolder(std::filesystem::path const& folder) {
    gchar* uri = g_filename_to_uri(folder.c_str(), nullptr, nullptr);
    openExternal(toWide(uri));
    g_free(uri);
}

void forgetHelperSession() {}

}

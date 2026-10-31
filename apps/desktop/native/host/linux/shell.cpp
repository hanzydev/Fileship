#include "linux.h"

#include <host/app.h>
#include <shared/log.h>

#include <fstream>

namespace fileship::host {

namespace {

std::filesystem::path autostartFile() {
    return xdgFolder("XDG_CONFIG_HOME", ".config") / "autostart" / "dev.hanzy.fileship.desktop";
}

}

void registerShellMenu() {}

void setShellHarborRoot(std::optional<std::filesystem::path> const&) {}

void applyStartAtLogin(bool enabled) {
    if (!isPackaged()) return;

    std::error_code error;
    if (!enabled) {
        std::filesystem::remove(autostartFile(), error);
        return;
    }

    std::filesystem::create_directories(autostartFile().parent_path(), error);
    std::ofstream file(autostartFile(), std::ios::trunc);
    file << "[Desktop Entry]\n"
         << "Type=Application\n"
         << "Name=Fileship\n"
         << "Icon=dev.hanzy.fileship\n"
         << "Exec=" << shellQuote(hostExecutable().string()) << " --hidden\n"
         << "NoDisplay=true\n"
         << "X-GNOME-Autostart-enabled=true\n";

    if (!file) logError(L"Could not update the startup entry");
}

}

#include "linux.h"

#include <host/app.h>
#include <shared/log.h>

#include <glib.h>
#include <sys/stat.h>

#include <fstream>
#include <mutex>

namespace fileship::host {

namespace {

struct ShellAction {
    char const* action;
    wchar_t const* label;
    bool isFolder;
};

constexpr ShellAction HARBOR_ACTIONS[] = {
    {"copyLink", L"shell.copyLink", false},
    {"open", L"shell.openInBrowser", false},
    {"show", L"shell.showInFileship", false},
    {"folderLink", L"shell.copyFolderLink", true},
    {"inboxLink", L"shell.copyInboxLink", true},
};

std::mutex mutex;
std::optional<std::filesystem::path> shellRoot;

std::filesystem::path autostartFile() {
    return xdgFolder("XDG_CONFIG_HOME", ".config") / "autostart" / "dev.hanzy.fileship.desktop";
}

std::filesystem::path dataFolder() {
    return xdgFolder("XDG_DATA_HOME", ".local/share");
}

bool hasProgram(char const* name) {
    auto path = g_find_program_in_path(name);
    g_free(path);
    return path != nullptr;
}

std::string command(std::string const& arguments) {
    return shellQuote(hostExecutable().string()) + " " + arguments;
}

std::string entryCommand(std::string const& arguments) {
    std::string quoted = "\"";
    for (auto character : hostExecutable().string()) {
        if (character == '"' || character == '`' || character == '$' || character == '\\') quoted += '\\';
        quoted += character;
    }
    return quoted + "\" " + arguments;
}

void writeFile(std::filesystem::path const& path, std::string const& content, bool isExecutable) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    std::ofstream file(path, std::ios::trunc);
    file << content;
    file.close();
    if (!file) throw std::runtime_error("Could not write " + path.string());
    if (isExecutable) chmod(path.c_str(), 0755);
}

std::string safeFileName(std::wstring const& label) {
    auto name = toUtf8(label);
    for (auto& character : name) {
        if (character == '/') character = '-';
    }
    return name;
}

void writeNautilus(bool hasHarbor) {
    auto folder = dataFolder() / "nautilus" / "scripts" / "Fileship";
    std::error_code error;
    std::filesystem::remove_all(folder, error);

    auto script = [](std::string const& arguments) {
        return "#!/bin/sh\nset -f\nIFS='\n'\nexec " + command(arguments) + " $NAUTILUS_SCRIPT_SELECTED_FILE_PATHS\n";
    };

    writeFile(folder / safeFileName(t(L"shell.upload")), script("--upload"), true);
    if (!hasHarbor) return;

    for (auto const& item : HARBOR_ACTIONS) {
        writeFile(folder / safeFileName(t(item.label)), script(std::string("--harbor-action ") + item.action), true);
    }
}

void writeNemo(bool hasHarbor) {
    auto folder = dataFolder() / "nemo" / "actions";
    std::error_code error;
    for (auto const& entry : std::filesystem::directory_iterator(folder, error)) {
        if (entry.path().filename().string().starts_with("fileship-")) std::filesystem::remove(entry.path(), error);
    }

    auto action = [&](std::string const& name, std::wstring const& label, std::string const& arguments,
                      bool isFolder) {
        writeFile(folder / ("fileship-" + name + ".nemo_action"),
                  "[Nemo Action]\nName=" + toUtf8(label) + "\nIcon-Name=dev.hanzy.fileship\nExec=" +
                      entryCommand(arguments + " %F") + "\nSelection=" + (isFolder ? "s" : "notnone") +
                      "\nExtensions=" + (isFolder ? "dir;" : "nodirs;") + "\n",
                  false);
    };

    action("upload", t(L"shell.upload"), "--upload", false);
    if (!hasHarbor) return;

    for (auto const& item : HARBOR_ACTIONS) {
        action(item.action, t(item.label), std::string("--harbor-action ") + item.action, item.isFolder);
    }
}

void writeDolphin(bool hasHarbor) {
    auto folder = dataFolder() / "kio" / "servicemenus";
    std::error_code error;
    std::filesystem::remove(folder / "fileship.desktop", error);
    std::filesystem::remove(folder / "fileship-folders.desktop", error);

    auto menu = [&](std::string const& file, std::string const& mimeTypes, bool isFolder) {
        std::string actions;
        std::string sections;
        auto add = [&](std::string const& name, std::wstring const& label, std::string const& arguments) {
            actions += name + ";";
            sections += "\n[Desktop Action " + name + "]\nName=" + toUtf8(label) +
                        "\nIcon=dev.hanzy.fileship\nExec=" + entryCommand(arguments + " %F") + "\n";
        };

        if (!isFolder) add("upload", t(L"shell.upload"), "--upload");
        if (hasHarbor) {
            for (auto const& item : HARBOR_ACTIONS) {
                if (item.isFolder == isFolder) {
                    add(item.action, t(item.label), std::string("--harbor-action ") + item.action);
                }
            }
        }
        if (actions.empty()) return;

        writeFile(folder / file,
                  "[Desktop Entry]\nType=Service\nMimeType=" + mimeTypes +
                      "\nX-KDE-Protocols=file\nX-KDE-Submenu=Fileship\nActions=" + actions + "\n" + sections,
                  true);
    };

    menu("fileship.desktop", "all/allfiles;", false);
    menu("fileship-folders.desktop", "inode/directory;", true);
}

}

void registerShellMenu() {
    if (!isPackaged() || isFlatpak()) return;

    bool hasHarbor = false;
    {
        std::lock_guard lock(mutex);
        hasHarbor = shellRoot.has_value();
    }

    try {
        if (hasProgram("nautilus")) writeNautilus(hasHarbor);
        if (hasProgram("nemo")) writeNemo(hasHarbor);
        if (hasProgram("dolphin")) writeDolphin(hasHarbor);
    } catch (std::exception const& error) {
        logError(L"Shell menu registration failed: " + toWide(error.what()));
    }
}

void setShellHarborRoot(std::optional<std::filesystem::path> const& root) {
    {
        std::lock_guard lock(mutex);
        shellRoot = root;
    }

    registerShellMenu();
}

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
         << "Exec=" << entryCommand("--hidden") << "\n"
         << "NoDisplay=true\n"
         << "X-GNOME-Autostart-enabled=true\n";

    if (!file) logError(L"Could not update the startup entry");
}

}

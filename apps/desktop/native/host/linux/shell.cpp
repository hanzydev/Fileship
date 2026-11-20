#include "host.h"

#include <shared/log.h>

#include <fstream>
#include <iterator>
#include <mutex>

namespace fileship::host {

namespace {

namespace fs = std::filesystem;

struct HarborVerb {
    char const* action;
    wchar_t const* label;
    bool isForFolders;
    bool many;
};

constexpr HarborVerb HARBOR_VERBS[] = {
    {"copyLink", L"shell.copyLink", false, true},
    {"open", L"shell.openInBrowser", false, true},
    {"show", L"shell.showInFileship", false, false},
    {"folderLink", L"shell.copyFolderLink", true, false},
    {"inboxLink", L"shell.copyInboxLink", true, false},
};

std::mutex mutex;
std::optional<fs::path> shellRoot;

fs::path dataFolder(char const* name) {
    return fs::path(g_get_user_data_dir()) / name;
}

void writeFile(fs::path const& file, std::string const& text, bool isExecutable = false) {
    fs::create_directories(file.parent_path());

    std::ofstream output(file, std::ios::trunc | std::ios::binary);
    output << text;
    if (!output.flush()) throw std::runtime_error("Could not write " + file.string());

    if (isExecutable) fs::permissions(file, fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec |
                                                fs::perms::others_read | fs::perms::others_exec);
}

std::string labelOf(wchar_t const* key) {
    return toUtf8(t(key));
}

void writeNautilus(fs::path const& exe, std::optional<fs::path> const& root) {
    JsonArray items;

    for (auto const& verb : HARBOR_VERBS) {
        JsonObject item;
        item.SetNamedValue(L"action", JsonValue::CreateStringValue(toWide(verb.action)));
        item.SetNamedValue(L"label", JsonValue::CreateStringValue(t(verb.label)));
        item.SetNamedValue(L"folders", JsonValue::CreateBooleanValue(verb.isForFolders));
        item.SetNamedValue(L"many", JsonValue::CreateBooleanValue(verb.many));
        items.Append(item);
    }

    JsonObject config;
    config.SetNamedValue(L"host", JsonValue::CreateStringValue(exe.wstring()));
    config.SetNamedValue(L"root", root ? JsonValue::CreateStringValue(root->wstring()) : JsonValue::CreateNullValue());
    config.SetNamedValue(L"upload", JsonValue::CreateStringValue(t(L"shell.upload")));
    config.SetNamedValue(L"items", items);

    writeFile(fs::path(g_get_user_config_dir()) / "fileship" / "menus.json", toUtf8(config.Stringify()));

    auto extension = dataFolder("nautilus-python") / "extensions" / "fileship.py";
    fs::create_directories(extension.parent_path());
    fs::copy_file(resourcesPath() / "integration" / "nautilus.py", extension, fs::copy_options::overwrite_existing);
}

std::string shellQuoted(std::string const& text) {
    std::string quoted = "'";

    for (char character : text) {
        if (character == '\'') {
            quoted += "'\\''";
        } else {
            quoted += character;
        }
    }

    return quoted + "'";
}

std::string nemoAction(fs::path const& exe, std::string const& name, std::string const& command, bool isForFolders,
                       bool many, char const* scope) {
    return "[Nemo Action]\nName=" + name + "\nExec=\"" + exe.string() + "\" " + command + " %F\nIcon-Name=fileship\nSelection=" +
           (many ? "notnone" : "s") + "\nExtensions=" + (isForFolders ? "dir" : "nodirs") + ";\nDependencies=" + exe.string() +
           ";\nConditions=exec <fileship-harbor.sh " + scope + " %F>;\nQuote=double\n";
}

void writeNemo(fs::path const& exe, std::optional<fs::path> const& root) {
    auto folder = dataFolder("nemo") / "actions";
    auto place = root ? "case \"$2\" in\n    " + shellQuoted(root->string()) + "/*) place=in ;;\n    *) place=out ;;\nesac\n"
                      : std::string("place=out\n");

    writeFile(folder / "fileship-harbor.sh", "#!/bin/sh\n" + place + "[ \"$1\" = \"$place\" ]\n", true);
    writeFile(folder / "fileship-upload.nemo_action", nemoAction(exe, labelOf(L"shell.upload"), "--upload", false, true, "out"));

    for (auto const& verb : HARBOR_VERBS) {
        auto file = folder / (std::string("fileship-") + verb.action + ".nemo_action");

        if (root) {
            writeFile(file, nemoAction(exe, labelOf(verb.label), std::string("--harbor-action ") + verb.action, verb.isForFolders,
                                       verb.many, "in"));
        } else {
            fs::remove(file);
        }
    }
}

std::string dolphinAction(fs::path const& exe, std::string const& id, std::string const& name, std::string const& command,
                          bool many) {
    return "\n[Desktop Action " + id + "]\nName=" + name + "\nIcon=" + (resourcesPath() / "icon.png").string() + "\nExec=\"" +
           exe.string() + "\" " + command + (many ? " %F\n" : " %f\n");
}

std::string dolphinMenu(fs::path const& exe, char const* types, std::string const& actions, std::string const& sections) {
    return std::string("[Desktop Entry]\nType=Service\nMimeType=") + types + ";\nActions=" + actions + "\nTryExec=" + exe.string() +
           "\nX-KDE-Priority=TopLevel\n" + sections;
}

void writeDolphin(fs::path const& exe, std::optional<fs::path> const& root) {
    auto folder = dataFolder("kio") / "servicemenus";
    std::string fileActions = "upload;";
    std::string fileSections = dolphinAction(exe, "upload", labelOf(L"shell.upload"), "--upload", true);
    std::string folderActions;
    std::string folderSections;

    for (auto const& verb : HARBOR_VERBS) {
        if (!root) break;

        auto section = dolphinAction(exe, verb.action, labelOf(verb.label), std::string("--harbor-action ") + verb.action, verb.many);
        (verb.isForFolders ? folderActions : fileActions) += std::string(verb.action) + ";";
        (verb.isForFolders ? folderSections : fileSections) += section;
    }

    writeFile(folder / "fileship.desktop", dolphinMenu(exe, "all/allfiles", fileActions, fileSections), true);

    if (root) {
        writeFile(folder / "fileship-folder.desktop", dolphinMenu(exe, "inode/directory", folderActions, folderSections), true);
    } else {
        fs::remove(folder / "fileship-folder.desktop");
    }
}

std::string xmlEscaped(std::string const& text) {
    std::string escaped;

    for (char character : text) {
        if (character == '&') {
            escaped += "&amp;";
        } else if (character == '<') {
            escaped += "&lt;";
        } else if (character == '>') {
            escaped += "&gt;";
        } else {
            escaped += character;
        }
    }

    return escaped;
}

std::string thunarAction(fs::path const& exe, std::string const& id, std::string const& name, std::string const& command,
                         bool isForFolders) {
    return "\t<action>\n\t\t<icon>" + xmlEscaped((resourcesPath() / "icon.png").string()) + "</icon>\n\t\t<name>" +
           xmlEscaped(name) + "</name>\n\t\t<submenu></submenu>\n\t\t<unique-id>fileship-" + id +
           "</unique-id>\n\t\t<command>" + xmlEscaped("\"" + exe.string() + "\" " + command + " %F") +
           "</command>\n\t\t<description></description>\n\t\t<range></range>\n\t\t<patterns>*</patterns>\n" +
           (isForFolders ? "\t\t<directories/>\n"
                         : "\t\t<audio-files/>\n\t\t<image-files/>\n\t\t<other-files/>\n\t\t<text-files/>\n\t\t<video-files/>\n") +
           "\t</action>\n";
}

std::optional<std::string> readText(fs::path const& file) {
    std::ifstream input(file, std::ios::binary);
    if (!input) return std::nullopt;
    return std::string(std::istreambuf_iterator<char>(input), {});
}

std::optional<std::string> thunarActions() {
    auto own = fs::path(g_get_user_config_dir()) / "Thunar" / "uca.xml";
    if (auto text = readText(own)) return text;

    gchar* thunar = g_find_program_in_path("thunar");
    if (!thunar) return std::nullopt;
    g_free(thunar);

    for (auto folder = g_get_system_config_dirs(); *folder; folder++) {
        if (auto text = readText(fs::path(*folder) / "Thunar" / "uca.xml")) return text;
    }

    return std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<actions>\n</actions>\n");
}

void writeThunar(fs::path const& exe, std::optional<fs::path> const& root) {
    auto current = thunarActions();
    if (!current) return;

    auto end = current->rfind("</actions>");
    if (end == std::string::npos) {
        logWarn(L"The Thunar custom actions are not readable, so Fileship leaves them alone");
        return;
    }

    std::string kept;
    size_t position = 0;

    while (position < end) {
        auto start = current->find("<action>", position);
        if (start == std::string::npos || start >= end) break;

        auto finish = current->find("</action>", start);
        if (finish == std::string::npos || finish >= end) break;
        finish += std::string("</action>").size();

        auto block = current->substr(start, finish - start);
        kept += current->substr(position, start - position);
        if (block.find("<unique-id>fileship-") == std::string::npos) {
            kept += block;
        } else {
            while (!kept.empty() && (kept.back() == '\t' || kept.back() == ' ')) kept.pop_back();
            if (finish < current->size() && (*current)[finish] == '\n') finish++;
        }

        position = finish;
    }

    kept += current->substr(position, end - position);

    std::string actions = thunarAction(exe, "upload", labelOf(L"shell.upload"), "--upload", false);
    for (auto const& verb : HARBOR_VERBS) {
        if (!root) break;
        actions += thunarAction(exe, verb.action, labelOf(verb.label), std::string("--harbor-action ") + verb.action,
                                verb.isForFolders);
    }

    auto text = kept + actions + current->substr(end);
    if (text == *current) return;

    auto file = fs::path(g_get_user_config_dir()) / "Thunar" / "uca.xml";
    auto temporary = fs::path(file).concat(".fileship");
    writeFile(temporary, text);
    fs::rename(temporary, file);
}

}

void registerShellMenu() {
    if (!isPackaged()) return;

    auto exe = hostExecutable();
    std::lock_guard lock(mutex);

    try {
        writeNautilus(exe, shellRoot);
        writeNemo(exe, shellRoot);
        writeDolphin(exe, shellRoot);
        writeThunar(exe, shellRoot);
    } catch (std::exception const& error) {
        logError(L"Could not write the file manager menus: " + toWide(error.what()));
    }
}

void setShellHarborRoot(std::optional<std::filesystem::path> const& root) {
    {
        std::lock_guard lock(mutex);
        shellRoot = root;
    }

    registerShellMenu();
}

}

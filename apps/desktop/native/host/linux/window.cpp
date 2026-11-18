#include <host/app.h>
#include <shared/log.h>

#include <glib.h>

namespace fileship::host {

bool launchElectron(std::vector<std::wstring> const& arguments) {
    auto const& options = launchOptions();
    std::vector<std::string> command;
    std::filesystem::path directory = std::filesystem::current_path();

    if (isPackaged()) {
        auto executable = resourcesPath().parent_path().parent_path() / "fileship";
        command.push_back(executable.string());
        directory = executable.parent_path();
    } else {
        if (options.electronCommand.empty()) {
            logWarn(L"No Electron command was given, cannot open a window");
            return true;
        }

        gchar** parts = nullptr;
        if (!g_shell_parse_argv(toUtf8(options.electronCommand).c_str(), nullptr, &parts, nullptr)) {
            logError(L"Could not read the Electron command");
            return false;
        }

        for (gchar** part = parts; *part; part++) command.emplace_back(*part);
        g_strfreev(parts);
    }

    command.push_back("--host-pipe");
    command.push_back(toUtf8(pipeName()));
    if (!options.userData.empty()) command.push_back("--user-data-dir=" + userDataPath().string());
    for (auto const& argument : arguments) command.push_back(toUtf8(argument));

    std::vector<gchar*> argv;
    for (auto& part : command) argv.push_back(part.data());
    argv.push_back(nullptr);

    gchar** environment = g_get_environ();
    if (!options.rendererUrl.empty()) {
        environment = g_environ_setenv(environment, "DESKTOP_RENDERER_URL", toUtf8(options.rendererUrl).c_str(), TRUE);
    }

    logInfo(L"Starting Electron: " + toWide(command[0]));

    GError* error = nullptr;
    bool isStarted = g_spawn_async(directory.c_str(), argv.data(), environment, G_SPAWN_SEARCH_PATH, nullptr, nullptr,
                                   nullptr, &error);
    g_strfreev(environment);

    if (!isStarted) {
        logError(L"Could not start Electron: " + toWide(error->message));
        g_error_free(error);
    }

    return isStarted;
}

}

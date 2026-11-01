#include "linux.h"

#include <host/app.h>
#include <shared/log.h>

#include <glib.h>

#include <cstdlib>

namespace fileship::host {

bool launchElectron(std::vector<std::wstring> const& arguments) {
    auto const& options = launchOptions();
    std::vector<std::string> argv;

    if (isPackaged()) {
        if (auto image = appImagePath()) argv.push_back(image->string());
        else argv.push_back((resourcesPath().parent_path().parent_path() / "fileship").string());
    } else if (options.electronCommand.empty()) {
        logWarn(L"No Electron command was given, cannot open a window");
        return false;
    }

    std::vector<std::string> extra{"--host-pipe", toUtf8(pipeName())};
    if (!options.userData.empty()) extra.push_back("--user-data-dir=" + userDataPath().string());
    for (auto const& argument : arguments) extra.push_back(toUtf8(argument));

    if (argv.empty()) {
        auto command = toUtf8(options.electronCommand);
        for (auto const& argument : extra) command += " " + shellQuote(argument);
        argv = {"/bin/sh", "-c", command};
    } else {
        argv.insert(argv.end(), extra.begin(), extra.end());
    }

    std::vector<char*> pointers;
    for (auto& argument : argv) pointers.push_back(argument.data());
    pointers.push_back(nullptr);

    auto environment = g_get_environ();
    if (!options.rendererUrl.empty())
        environment = g_environ_setenv(environment, "DESKTOP_RENDERER_URL", toUtf8(options.rendererUrl).c_str(), TRUE);

    std::string log;
    for (auto const& argument : argv) log += (log.empty() ? "" : " ") + argument;
    logInfo(L"Starting Electron: " + toWide(log));

    GError* error = nullptr;
    auto directory = isPackaged() ? argv.front() : std::filesystem::current_path().string();
    bool isStarted = g_spawn_async(isPackaged() ? nullptr : directory.c_str(), pointers.data(), environment,
                                   G_SPAWN_SEARCH_PATH, nullptr, nullptr, nullptr, &error);
    g_strfreev(environment);

    if (!isStarted) {
        logError(L"Could not start Electron: " + toWide(error ? error->message : "unknown error"));
        if (error) g_error_free(error);
    }

    return isStarted;
}

}

#include "linux.h"

#include <host/app.h>
#include <shared/log.h>

#include <glib.h>
#include <sys/stat.h>

#include <cstdlib>
#include <fstream>

namespace fileship::host {

namespace {

std::string packageType() {
    std::ifstream file(resourcesPath().parent_path() / "package-type");
    std::string type;
    std::getline(file, type);
    return type;
}

bool spawn(std::vector<std::string> arguments) {
    std::vector<char*> argv;
    for (auto& argument : arguments) argv.push_back(argument.data());
    argv.push_back(nullptr);

    GError* error = nullptr;
    if (g_spawn_async(nullptr, argv.data(), nullptr, G_SPAWN_SEARCH_PATH, nullptr, nullptr, nullptr, &error))
        return true;

    logError(L"Could not start the update: " + toWide(error ? error->message : "unknown error"));
    if (error) g_error_free(error);
    return false;
}

}

std::wstring updateManifestName() {
    return L"latest-linux.yml";
}

void startInstaller(std::filesystem::path const& path, bool forceRun) {
    if (auto image = std::getenv("APPIMAGE"); image && *image) {
        std::filesystem::path target(image);
        auto staged = std::filesystem::path(target).concat(".update");

        std::error_code error;
        std::filesystem::copy_file(path, staged, std::filesystem::copy_options::overwrite_existing, error);
        if (!error) {
            chmod(staged.c_str(), 0755);
            std::filesystem::rename(staged, target, error);
        }

        if (error) {
            logError(L"Could not replace the AppImage: " + toWide(error.message()));
            return;
        }

        if (forceRun) spawn({target.string(), "--hidden"});
        return;
    }

    auto type = packageType();
    auto file = shellQuote(path.string());
    std::string install = type == "deb"   ? "apt-get install -y --allow-downgrades " + file
                          : type == "rpm" ? "if command -v dnf >/dev/null; then dnf install -y " + file +
                                                "; else zypper --non-interactive install --allow-unsigned-rpm " +
                                                file + "; fi"
                                          : "";
    if (install.empty()) {
        logWarn(L"This installation cannot update itself");
        return;
    }

    auto command = "pkexec /bin/sh -c " + shellQuote(install);
    if (forceRun) command += " && " + shellQuote((resourcesPath() / "fileship-host").string()) + " --hidden";

    spawn({"/bin/sh", "-c", command});
}

}

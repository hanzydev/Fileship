#include "host.h"

#include <host/updater.h>
#include <shared/base64.h>
#include <shared/log.h>

#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

namespace fileship::host {

namespace {

constexpr char const* DPKG_ENTRY = "/var/lib/dpkg/info/fileship.list";
constexpr int DISMISSED = 126;
constexpr int NOT_AUTHORIZED = 127;

class GlibSha512 : public Sha512 {
public:
    ~GlibSha512() override {
        g_checksum_free(checksum);
    }

    void update(char const* data, size_t size) override {
        g_checksum_update(checksum, reinterpret_cast<guchar const*>(data), static_cast<gssize>(size));
    }

    std::wstring finish() override {
        guint8 digest[64]{};
        gsize size = sizeof(digest);
        g_checksum_get_digest(checksum, digest, &size);
        return base64(digest, size);
    }

private:
    GChecksum* checksum = g_checksum_new(G_CHECKSUM_SHA512);
};

std::optional<pid_t> spawn(std::vector<std::string> arguments, bool isDetached) {
    std::vector<char*> pointers;
    for (auto& argument : arguments) pointers.push_back(argument.data());
    pointers.push_back(nullptr);

    posix_spawnattr_t attributes;
    posix_spawnattr_init(&attributes);
    if (isDetached) posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETSID);

    pid_t process = 0;
    int error = posix_spawnp(&process, pointers[0], nullptr, &attributes, pointers.data(), environ);
    posix_spawnattr_destroy(&attributes);
    return error ? std::nullopt : std::optional(process);
}

}

std::unique_ptr<Sha512> Sha512::create() {
    return std::make_unique<GlibSha512>();
}

std::wstring updateFeedFile() {
    return L"latest-linux.yml";
}

std::wstring updatePackageSuffix() {
    return std::filesystem::exists(DPKG_ENTRY) ? L".deb" : L".rpm";
}

std::filesystem::path updateDownloadFolder() {
    return std::filesystem::path(g_get_user_cache_dir()) / "fileship" / "updates";
}

InstallResult runUpdateInstaller(std::filesystem::path const& file, bool) {
    auto manager = file.extension() == ".rpm" ? "dnf" : "apt-get";
    auto installer = spawn({"pkexec", manager, "install", "-y", file.string()}, false);

    if (!installer) {
        logError(L"Could not start pkexec");
        return InstallResult::Failed;
    }

    int status = 0;
    waitpid(*installer, &status, 0);
    int code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    if (code == NOT_AUTHORIZED || code == DISMISSED) return InstallResult::Refused;

    if (code) {
        logError(L"The update installer failed with " + std::to_wstring(code));
        return InstallResult::Failed;
    }

    auto restart = "while kill -0 " + std::to_string(getpid()) + " 2>/dev/null; do sleep 0.2; done; exec \"$0\"";
    spawn({"sh", "-c", restart, hostExecutable().string()}, true);
    return InstallResult::Started;
}

}

#include "linux.h"

#include <host/app.h>
#include <host/rpc.h>
#include <shared/log.h>

#include <fcntl.h>
#include <signal.h>
#include <glib-unix.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <clocale>
#include <locale>

int main(int argc, char** argv) {
    using namespace fileship;
    using namespace fileship::host;

    std::setlocale(LC_ALL, "");
    signal(SIGPIPE, SIG_IGN);
    try {
        std::locale::global(std::locale("C.UTF-8"));
    } catch (std::runtime_error const&) {
    }

    std::vector<std::wstring> arguments;
    for (int index = 1; index < argc; index++) arguments.push_back(toWide(argv[index]));
    parseOptions(arguments);

    auto folder = runtimeFolder() / "fileship";
    std::filesystem::create_directories(folder);
    chmod(folder.c_str(), 0700);

    auto lockPath = folder / ("host" + toUtf8(profileSuffix()) + ".lock");
    int lock = open(lockPath.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    if (lock < 0 || flock(lock, LOCK_EX | LOCK_NB) != 0) {
        if (shouldForwardLaunch()) forwardToRunningHost(launchOptions().arguments);
        return 0;
    }

    initHost();

    for (int signal : {SIGINT, SIGTERM, SIGHUP}) {
        g_unix_signal_add(
            signal,
            [](gpointer) -> gboolean {
                quitApp();
                return G_SOURCE_CONTINUE;
            },
            nullptr);
    }

    startHost();
    g_main_loop_run(mainLoop());

    stopRpcServer();
    logInfo(L"Stopped");
    close(lock);
    return 0;
}

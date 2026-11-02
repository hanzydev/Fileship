#include "linux.h"

#include <host/app.h>
#include <host/rpc.h>
#include <shared/log.h>

#include <execinfo.h>
#include <fcntl.h>
#include <signal.h>
#include <glib-unix.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <clocale>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <locale>

namespace {

constexpr int CRASH_SIGNALS[] = {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT};
constexpr std::uintmax_t MAX_OUTPUT_LOG = 1024 * 1024;

char alternateStack[64 * 1024];

void writeText(char const* text) {
    [[maybe_unused]] auto written = ::write(STDERR_FILENO, text, std::strlen(text));
}

void writeNumber(int value) {
    char digits[16];
    int index = sizeof(digits) - 1;
    digits[index] = '\0';

    do {
        digits[--index] = static_cast<char>('0' + value % 10);
        value /= 10;
    } while (value && index);

    writeText(digits + index);
}

void handleCrash(int signal) {
    writeText("Fileship host crashed with signal ");
    writeNumber(signal);
    writeText(", backtrace:\n");

    void* frames[64];
    int count = backtrace(frames, 64);
    backtrace_symbols_fd(frames, count, STDERR_FILENO);
    raise(signal);
}

void installCrashHandler() {
    stack_t stack{};
    stack.ss_sp = alternateStack;
    stack.ss_size = sizeof(alternateStack);
    sigaltstack(&stack, nullptr);

    void* warmUp[1];
    backtrace(warmUp, 1);

    struct sigaction action{};
    action.sa_handler = handleCrash;
    action.sa_flags = SA_ONSTACK | SA_RESETHAND;
    sigemptyset(&action.sa_mask);
    for (int signal : CRASH_SIGNALS) sigaction(signal, &action, nullptr);
}

void redirectOutput(std::filesystem::path const& folder) {
    if (isatty(STDERR_FILENO)) return;

    std::error_code error;
    std::filesystem::create_directories(folder, error);

    auto path = folder / "host-stderr.log";
    if (std::filesystem::file_size(path, error) > MAX_OUTPUT_LOG && !error)
        std::filesystem::rename(path, folder / "host-stderr.old.log", error);

    int output = open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0600);
    if (output < 0) return;

    dup2(output, STDOUT_FILENO);
    dup2(output, STDERR_FILENO);
    close(output);

    char started[64];
    auto now = std::time(nullptr);
    std::strftime(started, sizeof(started), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    std::fprintf(stderr, "--- Fileship host %s started %s\n", fileship::toUtf8(fileship::host::appVersion()).c_str(),
                 started);
    std::fflush(stderr);
}

}

int main(int argc, char** argv) {
    using namespace fileship;
    using namespace fileship::host;

    std::setlocale(LC_ALL, "");
    signal(SIGPIPE, SIG_IGN);
    try {
        std::locale::global(std::locale("C.UTF-8"));
    } catch (std::runtime_error const&) {
    }

    runOutsideAppImage(argv);

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

    redirectOutput(userDataPath() / "logs");
    installCrashHandler();
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
    std::fflush(stdout);
    std::fflush(stderr);
    _exit(0);
}

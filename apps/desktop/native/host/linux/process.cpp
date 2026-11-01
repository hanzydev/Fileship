#include <host/process.h>

#include <shared/utf8.h>

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <mutex>

extern char** environ;

namespace fileship::host {

struct ChildProcess::Handles {
    pid_t pid = -1;
    int pidfd = -1;
    int input = -1;
    int output = -1;
    int errors = -1;
    std::mutex mutex;
    bool isReaped = false;
    int code = 0;
};

namespace {

struct Spawned {
    pid_t pid = -1;
    int input = -1;
    int output = -1;
    int errors = -1;
};

void closeFd(int& fd) {
    if (fd >= 0) close(fd);
    fd = -1;
}

int statusCode(int status) {
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 1;
}

Spawned spawn(std::wstring const& path, std::vector<std::wstring> const& args, bool withInput, bool withOutput) {
    int input[2]{-1, -1};
    int output[2]{-1, -1};
    int errors[2]{-1, -1};

    auto cleanup = [&] {
        for (auto* pair : {input, output, errors}) {
            closeFd(pair[0]);
            closeFd(pair[1]);
        }
    };

    if ((withInput && pipe2(input, O_CLOEXEC)) || (withOutput && pipe2(output, O_CLOEXEC)) ||
        pipe2(errors, O_CLOEXEC)) {
        cleanup();
        throw ProcessError("pipe2 failed");
    }

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    if (withInput) {
        posix_spawn_file_actions_adddup2(&actions, input[0], STDIN_FILENO);
    } else {
        posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    }
    if (withOutput) {
        posix_spawn_file_actions_adddup2(&actions, output[1], STDOUT_FILENO);
    } else {
        posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
    }
    posix_spawn_file_actions_adddup2(&actions, errors[1], STDERR_FILENO);

    posix_spawnattr_t attributes;
    posix_spawnattr_init(&attributes);
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGPIPE);
    posix_spawnattr_setsigdefault(&attributes, &signals);
    sigemptyset(&signals);
    posix_spawnattr_setsigmask(&attributes, &signals);
    posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK);

    auto program = toUtf8(path);
    std::vector<std::string> values{program};
    for (auto const& arg : args) values.push_back(toUtf8(arg));
    std::vector<char*> argv;
    for (auto& value : values) argv.push_back(value.data());
    argv.push_back(nullptr);

    pid_t pid = -1;
    int error = posix_spawn(&pid, program.c_str(), &actions, &attributes, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    posix_spawnattr_destroy(&attributes);

    closeFd(input[0]);
    closeFd(output[1]);
    closeFd(errors[1]);

    if (error) {
        cleanup();
        throw ProcessError("Could not start " + program + " (" + std::strerror(error) + ")");
    }

    return {pid, input[1], output[0], errors[0]};
}

void readLines(int fd, ChildProcess::LineHandler const& onLine) {
    std::string partial;
    char buffer[16 * 1024];

    for (;;) {
        auto count = read(fd, buffer, sizeof(buffer));
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) break;
        splitLines(partial, buffer, static_cast<size_t>(count), onLine);
    }

    if (!partial.empty()) onLine(partial);
}

void joinOrDetach(std::thread& thread) {
    if (thread.get_id() == std::this_thread::get_id()) {
        thread.detach();
    } else if (thread.joinable()) {
        thread.join();
    }
}

std::string readAll(int fd) {
    std::string text;
    char buffer[16 * 1024];

    for (;;) {
        auto count = read(fd, buffer, sizeof(buffer));
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) break;
        text.append(buffer, static_cast<size_t>(count));
    }

    return text;
}

int waitFor(pid_t pid) {
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) return 1;
    }
    return statusCode(status);
}

}

ProcessResult runProcess(std::wstring const& path, std::vector<std::wstring> const& args) {
    auto spawned = spawn(path, args, false, true);
    ProcessResult result;

    std::thread errors([&] { result.err = readAll(spawned.errors); });
    result.out = readAll(spawned.output);
    errors.join();

    result.code = waitFor(spawned.pid);
    closeFd(spawned.output);
    closeFd(spawned.errors);

    if (result.code) {
        auto message = lastLine(result.err);
        throw ProcessError(message.empty() ? "Exited with code " + std::to_string(result.code) : message);
    }

    return result;
}

ChildProcess::ChildProcess(std::wstring const& path, std::vector<std::wstring> const& args, Handlers handlers)
    : handles(std::make_unique<Handles>()) {
    bool withOutput = static_cast<bool>(handlers.onOutput);
    auto spawned = spawn(path, args, true, withOutput);
    handles->pid = spawned.pid;
    handles->input = spawned.input;
    handles->output = spawned.output;
    handles->errors = spawned.errors;
    handles->pidfd = static_cast<int>(syscall(SYS_pidfd_open, spawned.pid, 0));

    if (!handlers.onError) handlers.onError = [](std::string const&) {};
    reader = std::thread([errors = handles->errors, onLine = std::move(handlers.onError)] { readLines(errors, onLine); });

    if (withOutput) {
        outputReader = std::thread([output = handles->output, onLine = std::move(handlers.onOutput),
                                    onClosed = std::move(handlers.onOutputClosed)] {
            readLines(output, onLine);
            if (onClosed) onClosed();
        });
    }
}

ChildProcess::~ChildProcess() {
    closeFd(handles->input);
    if (!wait(0)) {
        kill();
        wait(WAIT_UNTIL_EXIT);
    }
    joinOrDetach(reader);
    joinOrDetach(outputReader);
    closeFd(handles->output);
    closeFd(handles->errors);
    closeFd(handles->pidfd);
}

void ChildProcess::write(std::string const& text) {
    if (handles->input < 0) return;

    size_t offset = 0;
    while (offset < text.size()) {
        auto count = ::write(handles->input, text.data() + offset, text.size() - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return;
        offset += static_cast<size_t>(count);
    }
}

void ChildProcess::closeInput() {
    closeFd(handles->input);
}

void ChildProcess::kill() {
    std::lock_guard lock(handles->mutex);
    if (!handles->isReaped) ::kill(handles->pid, SIGKILL);
}

bool ChildProcess::wait(uint32_t timeoutMs) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);

    for (;;) {
        {
            std::lock_guard lock(handles->mutex);
            if (handles->isReaped) return true;

            int status = 0;
            pid_t result = waitpid(handles->pid, &status, WNOHANG);
            if (result != 0 && !(result < 0 && errno == EINTR)) {
                handles->isReaped = true;
                handles->code = result < 0 ? 1 : statusCode(status);
                return true;
            }
        }

        int remaining = -1;
        if (timeoutMs != WAIT_UNTIL_EXIT) {
            auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
            if (left.count() <= 0) return false;
            remaining = static_cast<int>(left.count());
        }

        if (handles->pidfd >= 0) {
            pollfd item{handles->pidfd, POLLIN, 0};
            poll(&item, 1, remaining);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(remaining < 0 ? 20 : std::min(remaining, 20)));
        }
    }
}

int ChildProcess::exitCode() const {
    std::lock_guard lock(handles->mutex);
    return handles->code;
}

}

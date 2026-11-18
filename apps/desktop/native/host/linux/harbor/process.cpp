#include <host/harbor/process.h>

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace fileship::host::harbor {

namespace {

template <typename Handler>
void readLines(int pipe, Handler const& handle) {
    std::string buffer;
    char chunk[65536];
    ssize_t count = 0;

    while ((count = read(pipe, chunk, sizeof(chunk))) > 0) {
        buffer.append(chunk, static_cast<size_t>(count));
        takeLines(buffer, handle);
    }

    close(pipe);
}

class PosixHelperProcess : public HelperProcess {
public:
    PosixHelperProcess(pid_t process, int input, int output, int errors)
        : process(process), input(input), output(output), errors(errors) {}

    ~PosixHelperProcess() override {
        closeInput();
        wait(0);
    }

    void listen(LineHandler onOutput, LineHandler onError, std::function<void()> onClosed) override {
        std::thread([pipe = output, onOutput = std::move(onOutput), onClosed = std::move(onClosed)] {
            readLines(pipe, onOutput);
            onClosed();
        }).detach();
        std::thread([pipe = errors, onError = std::move(onError)] { readLines(pipe, onError); }).detach();
    }

    void write(std::string const& text) override {
        std::lock_guard lock(mutex);
        size_t sent = 0;

        while (input >= 0 && sent < text.size()) {
            ssize_t count = ::write(input, text.data() + sent, text.size() - sent);
            if (count <= 0) return;
            sent += static_cast<size_t>(count);
        }
    }

    void closeInput() override {
        std::lock_guard lock(mutex);
        if (input >= 0) close(input);
        input = -1;
    }

    std::optional<uint32_t> wait(int timeoutMs) override {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);

        while (true) {
            {
                std::lock_guard lock(exitMutex);
                if (code) return code;

                int status = 0;
                if (waitpid(process, &status, WNOHANG) == process) {
                    code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
                    return code;
                }
            }

            if (std::chrono::steady_clock::now() >= deadline) return std::nullopt;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }

    void terminate() override {
        std::lock_guard lock(exitMutex);
        if (!code) kill(process, SIGKILL);
    }

private:
    std::mutex mutex;
    std::mutex exitMutex;
    std::optional<uint32_t> code;
    pid_t process;
    int input;
    int output;
    int errors;
};

}

std::unique_ptr<HelperProcess> HelperProcess::start(std::filesystem::path const& executable) {
    int input[2];
    int output[2];
    int errors[2];

    if (pipe2(input, O_CLOEXEC) || pipe2(output, O_CLOEXEC) || pipe2(errors, O_CLOEXEC)) {
        throw std::runtime_error("Could not create a pipe");
    }

    pid_t process = fork();
    if (process < 0) throw std::runtime_error("Could not start " + executable.string());

    if (process == 0) {
        dup2(input[0], STDIN_FILENO);
        dup2(output[1], STDOUT_FILENO);
        dup2(errors[1], STDERR_FILENO);
        execl(executable.c_str(), executable.c_str(), nullptr);
        _exit(127);
    }

    close(input[0]);
    close(output[1]);
    close(errors[1]);
    return std::make_unique<PosixHelperProcess>(process, input[1], output[0], errors[0]);
}

}

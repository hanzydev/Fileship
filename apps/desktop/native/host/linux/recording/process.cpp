#include <host/recording/process.h>
#include <shared/utf8.h>

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <optional>
#include <thread>

namespace fileship::host::recording {

namespace {

struct Spawned {
    pid_t process = -1;
    int input = -1;
    int output = -1;
    int errors = -1;
};

Spawned spawn(std::wstring const& path, std::vector<std::wstring> const& args, bool withInput, bool withOutput) {
    int input[2] = {-1, -1};
    int output[2] = {-1, -1};
    int errors[2] = {-1, -1};

    if ((withInput && pipe2(input, O_CLOEXEC)) || (withOutput && pipe2(output, O_CLOEXEC)) ||
        pipe2(errors, O_CLOEXEC)) {
        throw ProcessError("Could not create a pipe");
    }

    auto program = toUtf8(path);
    std::vector<std::string> values{program};
    for (auto const& arg : args) values.push_back(toUtf8(arg));

    std::vector<char*> pointers;
    for (auto& value : values) pointers.push_back(value.data());
    pointers.push_back(nullptr);

    pid_t process = fork();
    if (process < 0) throw ProcessError("Could not start " + program);

    if (process == 0) {
        int null = open("/dev/null", O_RDWR);
        dup2(withInput ? input[0] : null, STDIN_FILENO);
        dup2(withOutput ? output[1] : null, STDOUT_FILENO);
        dup2(errors[1], STDERR_FILENO);
        execv(program.c_str(), pointers.data());
        _exit(127);
    }

    if (withInput) close(input[0]);
    if (withOutput) close(output[1]);
    close(errors[1]);

    return {process, input[1], output[0], errors[0]};
}

std::string readAll(int descriptor) {
    std::string text;
    char buffer[16 * 1024];

    for (ssize_t count; (count = read(descriptor, buffer, sizeof(buffer))) > 0;) text.append(buffer, count);
    return text;
}

uint32_t codeOf(int status) {
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}

class PosixProcess : public ChildProcess {
public:
    PosixProcess(std::wstring const& path, std::vector<std::wstring> const& args, LineHandler onLine) {
        auto spawned = spawn(path, args, true, false);
        process = spawned.process;
        input = spawned.input;
        errors = spawned.errors;

        reader = std::thread([this, onLine = std::move(onLine)] {
            std::string partial;
            char buffer[8 * 1024];

            for (ssize_t count; (count = read(errors, buffer, sizeof(buffer))) > 0;) {
                partial.append(buffer, count);

                size_t start = 0;
                for (size_t end; (end = partial.find('\n', start)) != std::string::npos; start = end + 1) {
                    auto line = partial.substr(start, end - start);
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    onLine(line);
                }
                partial.erase(0, start);
            }

            if (!partial.empty()) onLine(partial);
        });

        waiter = std::thread([this] {
            int status = 0;
            waitpid(process, &status, 0);

            std::lock_guard lock(mutex);
            code = codeOf(status);
            exited.notify_all();
        });
    }

    ~PosixProcess() override {
        if (input >= 0) close(input);
        kill();
        waiter.join();

        if (reader.get_id() == std::this_thread::get_id()) {
            reader.detach();
        } else {
            reader.join();
        }

        close(errors);
    }

    void write(std::string const& text) override {
        if (::write(input, text.data(), text.size()) < 0) return;
    }

    void kill() override {
        std::lock_guard lock(mutex);
        if (!code) ::kill(process, SIGKILL);
    }

    void wait() override {
        std::unique_lock lock(mutex);
        exited.wait(lock, [this] { return code.has_value(); });
    }

    bool wait(int64_t timeoutMs) override {
        std::unique_lock lock(mutex);
        return exited.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this] { return code.has_value(); });
    }

    uint32_t exitCode() const override {
        std::lock_guard lock(mutex);
        return code.value_or(0);
    }

private:
    pid_t process = -1;
    int input = -1;
    int errors = -1;
    mutable std::mutex mutex;
    std::condition_variable exited;
    std::optional<uint32_t> code;
    std::thread reader;
    std::thread waiter;
};

}

ProcessResult runProcess(std::wstring const& path, std::vector<std::wstring> const& args) {
    auto spawned = spawn(path, args, false, true);
    ProcessResult result;

    std::thread errors([&] { result.err = readAll(spawned.errors); });
    result.out = readAll(spawned.output);
    errors.join();

    int status = 0;
    waitpid(spawned.process, &status, 0);
    result.code = codeOf(status);

    close(spawned.output);
    close(spawned.errors);

    if (result.code) {
        auto message = lastLine(result.err);
        throw ProcessError(message.empty() ? "Exited with code " + std::to_string(result.code) : message);
    }

    return result;
}

std::unique_ptr<ChildProcess> ChildProcess::start(std::wstring const& path, std::vector<std::wstring> const& args,
                                                  LineHandler onLine) {
    return std::make_unique<PosixProcess>(path, args, std::move(onLine));
}

}

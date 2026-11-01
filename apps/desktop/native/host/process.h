#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace fileship::host {

constexpr uint32_t WAIT_UNTIL_EXIT = UINT32_MAX;

struct ProcessError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct ProcessResult {
    int code = 0;
    std::string out;
    std::string err;
};

ProcessResult runProcess(std::wstring const& path, std::vector<std::wstring> const& args);
std::string lastLine(std::string const& text);

class ChildProcess {
public:
    using LineHandler = std::function<void(std::string const& line)>;

    struct Handlers {
        LineHandler onError;
        LineHandler onOutput;
        std::function<void()> onOutputClosed;
    };

    ChildProcess(std::wstring const& path, std::vector<std::wstring> const& args, LineHandler onError);
    ChildProcess(std::wstring const& path, std::vector<std::wstring> const& args, Handlers handlers);
    ~ChildProcess();

    ChildProcess(ChildProcess const&) = delete;
    ChildProcess& operator=(ChildProcess const&) = delete;

    void write(std::string const& text);
    void closeInput();
    void kill();
    bool wait(uint32_t timeoutMs);
    int exitCode() const;

private:
    struct Handles;

    std::unique_ptr<Handles> handles;
    std::thread reader;
    std::thread outputReader;
};

void splitLines(std::string& partial, char const* data, size_t size, ChildProcess::LineHandler const& onLine);

}

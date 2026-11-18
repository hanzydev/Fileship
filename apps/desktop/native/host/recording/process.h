#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace fileship::host::recording {

struct ProcessError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct ProcessResult {
    uint32_t code = 0;
    std::string out;
    std::string err;
};

ProcessResult runProcess(std::wstring const& path, std::vector<std::wstring> const& args);
std::string lastLine(std::string const& text);

class ChildProcess {
public:
    using LineHandler = std::function<void(std::string const& line)>;

    static std::unique_ptr<ChildProcess> start(std::wstring const& path, std::vector<std::wstring> const& args,
                                               LineHandler onLine);

    virtual ~ChildProcess() = default;

    virtual void write(std::string const& text) = 0;
    virtual void kill() = 0;
    virtual void wait() = 0;
    virtual bool wait(int64_t timeoutMs) = 0;
    virtual uint32_t exitCode() const = 0;
};

}

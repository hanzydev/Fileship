#pragma once

#include <windows.h>

#include <functional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace fileship::host::recording {

struct ProcessError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct ProcessResult {
    DWORD code = 0;
    std::string out;
    std::string err;
};

std::wstring commandLine(std::wstring const& path, std::vector<std::wstring> const& args);
ProcessResult runProcess(std::wstring const& path, std::vector<std::wstring> const& args);
std::string lastLine(std::string const& text);

class ChildProcess {
public:
    using LineHandler = std::function<void(std::string const& line)>;

    ChildProcess(std::wstring const& path, std::vector<std::wstring> const& args, LineHandler onLine);
    ~ChildProcess();

    ChildProcess(ChildProcess const&) = delete;
    ChildProcess& operator=(ChildProcess const&) = delete;

    void write(std::string const& text);
    void kill();
    bool wait(DWORD timeoutMs);
    DWORD exitCode() const;
    HANDLE handle() const { return process; }

private:
    HANDLE process = nullptr;
    HANDLE input = nullptr;
    HANDLE errors = nullptr;
    std::thread reader;
};

}

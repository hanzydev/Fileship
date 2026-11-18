#include <host/harbor/process.h>

#include <windows.h>

#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace fileship::host::harbor {

namespace {

[[noreturn]] void throwLastError(char const* what) {
    throw std::runtime_error(std::string(what) + " failed with " + std::to_string(GetLastError()));
}

template <typename Handler>
void readLines(HANDLE pipe, size_t chunkSize, Handler const& handle) {
    std::string buffer;
    std::vector<char> chunk(chunkSize);
    DWORD read = 0;

    while (ReadFile(pipe, chunk.data(), static_cast<DWORD>(chunk.size()), &read, nullptr) && read > 0) {
        buffer.append(chunk.data(), read);
        takeLines(buffer, handle);
    }

    CloseHandle(pipe);
}

class Win32HelperProcess : public HelperProcess {
public:
    Win32HelperProcess(HANDLE process, HANDLE input, HANDLE output, HANDLE errors)
        : process(process), input(input), output(output), errors(errors) {}

    ~Win32HelperProcess() override {
        if (input) CloseHandle(input);
        CloseHandle(process);
    }

    void listen(LineHandler onOutput, LineHandler onError, std::function<void()> onClosed) override {
        std::thread([pipe = output, onOutput = std::move(onOutput), onClosed = std::move(onClosed)] {
            readLines(pipe, 65536, onOutput);
            onClosed();
        }).detach();
        std::thread([pipe = errors, onError = std::move(onError)] { readLines(pipe, 4096, onError); }).detach();
    }

    void write(std::string const& text) override {
        std::lock_guard lock(mutex);
        if (!input) return;

        DWORD written = 0;
        WriteFile(input, text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
    }

    void closeInput() override {
        std::lock_guard lock(mutex);
        if (input) CloseHandle(input);
        input = nullptr;
    }

    std::optional<uint32_t> wait(int timeoutMs) override {
        if (WaitForSingleObject(process, timeoutMs) == WAIT_TIMEOUT) return std::nullopt;

        DWORD code = 0;
        if (GetExitCodeProcess(process, &code) && code != STILL_ACTIVE) return code;
        return std::nullopt;
    }

    void terminate() override {
        TerminateProcess(process, 1);
    }

private:
    std::mutex mutex;
    HANDLE process;
    HANDLE input;
    HANDLE output;
    HANDLE errors;
};

}

std::unique_ptr<HelperProcess> HelperProcess::start(std::filesystem::path const& path) {
    SECURITY_ATTRIBUTES inherit{sizeof(inherit), nullptr, TRUE};
    HANDLE inputRead = nullptr, inputWrite = nullptr;
    HANDLE outputRead = nullptr, outputWrite = nullptr;
    HANDLE errorRead = nullptr, errorWrite = nullptr;

    if (!CreatePipe(&inputRead, &inputWrite, &inherit, 0) ||
        !CreatePipe(&outputRead, &outputWrite, &inherit, 0) ||
        !CreatePipe(&errorRead, &errorWrite, &inherit, 0)) {
        throwLastError("CreatePipe");
    }

    SetHandleInformation(inputWrite, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(outputRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(errorRead, HANDLE_FLAG_INHERIT, 0);

    HANDLE inherited[] = {inputRead, outputWrite, errorWrite};
    SIZE_T size = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
    std::vector<char> attributes(size);
    auto list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
    InitializeProcThreadAttributeList(list, 1, 0, &size);
    UpdateProcThreadAttribute(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherited, sizeof(inherited),
                              nullptr, nullptr);

    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup.StartupInfo.wShowWindow = SW_HIDE;
    startup.StartupInfo.hStdInput = inputRead;
    startup.StartupInfo.hStdOutput = outputWrite;
    startup.StartupInfo.hStdError = errorWrite;
    startup.lpAttributeList = list;

    auto executable = path.wstring();
    std::wstring command = L"\"" + executable + L"\"";
    PROCESS_INFORMATION info{};

    BOOL created = CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
                                  CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr,
                                  &startup.StartupInfo, &info);
    DWORD error = GetLastError();

    DeleteProcThreadAttributeList(list);
    CloseHandle(inputRead);
    CloseHandle(outputWrite);
    CloseHandle(errorWrite);

    if (!created) {
        CloseHandle(inputWrite);
        CloseHandle(outputRead);
        CloseHandle(errorRead);
        SetLastError(error);
        throwLastError("CreateProcess");
    }

    CloseHandle(info.hThread);
    return std::make_unique<Win32HelperProcess>(info.hProcess, inputWrite, outputRead, errorRead);
}

}

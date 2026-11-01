#include <host/process.h>

#include <shared/utf8.h>

#include <windows.h>

#include <array>

namespace fileship::host {

struct ChildProcess::Handles {
    HANDLE process = nullptr;
    HANDLE input = nullptr;
    HANDLE output = nullptr;
    HANDLE errors = nullptr;
};

namespace {

struct Spawned {
    HANDLE process = nullptr;
    HANDLE input = nullptr;
    HANDLE output = nullptr;
    HANDLE errors = nullptr;
};

void closeHandle(HANDLE& handle) {
    if (handle) CloseHandle(handle);
    handle = nullptr;
}

std::wstring quote(std::wstring const& arg) {
    if (!arg.empty() && arg.find_first_of(L" \t\n\v\"") == std::wstring::npos) return arg;

    std::wstring result = L"\"";

    for (auto it = arg.begin();; ++it) {
        size_t backslashes = 0;
        while (it != arg.end() && *it == L'\\') {
            ++it;
            ++backslashes;
        }

        if (it == arg.end()) {
            result.append(backslashes * 2, L'\\');
            break;
        }

        if (*it == L'"') {
            result.append(backslashes * 2 + 1, L'\\');
        } else {
            result.append(backslashes, L'\\');
        }
        result.push_back(*it);
    }

    result.push_back(L'"');
    return result;
}

std::wstring commandLine(std::wstring const& path, std::vector<std::wstring> const& args) {
    auto line = quote(path);
    for (auto const& arg : args) line += L" " + quote(arg);
    return line;
}

void createPipe(HANDLE& parent, HANDLE& child, bool childReads) {
    SECURITY_ATTRIBUTES attributes{sizeof(attributes), nullptr, TRUE};
    HANDLE read = nullptr;
    HANDLE write = nullptr;
    if (!CreatePipe(&read, &write, &attributes, 1 << 16)) throw ProcessError("CreatePipe failed");

    parent = childReads ? write : read;
    child = childReads ? read : write;
    SetHandleInformation(parent, HANDLE_FLAG_INHERIT, 0);
}

HANDLE openNull() {
    SECURITY_ATTRIBUTES attributes{sizeof(attributes), nullptr, TRUE};
    return CreateFileW(L"NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       &attributes, OPEN_EXISTING, 0, nullptr);
}

Spawned spawn(std::wstring const& path, std::vector<std::wstring> const& args, bool withInput,
              bool withOutput) {
    Spawned result;
    std::array<HANDLE, 3> child{};

    try {
        if (withInput) {
            createPipe(result.input, child[0], true);
        } else {
            child[0] = openNull();
        }

        if (withOutput) {
            createPipe(result.output, child[1], false);
        } else {
            child[1] = openNull();
        }

        createPipe(result.errors, child[2], false);
    } catch (...) {
        for (auto& handle : child) closeHandle(handle);
        closeHandle(result.input);
        closeHandle(result.output);
        closeHandle(result.errors);
        throw;
    }

    SIZE_T size = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
    std::vector<char> buffer(size);
    auto list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(buffer.data());
    InitializeProcThreadAttributeList(list, 1, 0, &size);
    UpdateProcThreadAttribute(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, child.data(),
                              sizeof(HANDLE) * child.size(), nullptr, nullptr);

    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput = child[0];
    startup.StartupInfo.hStdOutput = child[1];
    startup.StartupInfo.hStdError = child[2];
    startup.lpAttributeList = list;

    auto line = commandLine(path, args);
    PROCESS_INFORMATION info{};
    BOOL created = CreateProcessW(path.c_str(), line.data(), nullptr, nullptr, TRUE,
                                  CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT |
                                      CREATE_UNICODE_ENVIRONMENT,
                                  nullptr, nullptr, &startup.StartupInfo, &info);
    DWORD error = GetLastError();

    DeleteProcThreadAttributeList(list);
    for (auto& handle : child) closeHandle(handle);

    if (!created) {
        closeHandle(result.input);
        closeHandle(result.output);
        closeHandle(result.errors);
        throw ProcessError("Could not start " + toUtf8(path) + " (" + std::to_string(error) + ")");
    }

    CloseHandle(info.hThread);
    result.process = info.hProcess;
    return result;
}

void readLines(HANDLE handle, ChildProcess::LineHandler const& onLine) {
    std::string partial;
    char buffer[16 * 1024];
    DWORD read = 0;

    while (ReadFile(handle, buffer, sizeof(buffer), &read, nullptr) && read) {
        splitLines(partial, buffer, read, onLine);
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

std::string readAll(HANDLE handle) {
    std::string text;
    char buffer[16 * 1024];
    DWORD read = 0;

    while (ReadFile(handle, buffer, sizeof(buffer), &read, nullptr) && read) text.append(buffer, read);
    return text;
}

}

ProcessResult runProcess(std::wstring const& path, std::vector<std::wstring> const& args) {
    auto spawned = spawn(path, args, false, true);
    ProcessResult result;

    std::thread errors([&] { result.err = readAll(spawned.errors); });
    result.out = readAll(spawned.output);
    errors.join();

    DWORD code = 0;
    WaitForSingleObject(spawned.process, INFINITE);
    GetExitCodeProcess(spawned.process, &code);
    result.code = static_cast<int>(code);

    closeHandle(spawned.output);
    closeHandle(spawned.errors);
    closeHandle(spawned.process);

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
    handles->process = spawned.process;
    handles->input = spawned.input;
    handles->output = spawned.output;
    handles->errors = spawned.errors;

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
    closeHandle(handles->input);
    if (WaitForSingleObject(handles->process, 0) == WAIT_TIMEOUT) TerminateProcess(handles->process, 1);
    joinOrDetach(reader);
    joinOrDetach(outputReader);
    closeHandle(handles->output);
    closeHandle(handles->errors);
    closeHandle(handles->process);
}

void ChildProcess::write(std::string const& text) {
    DWORD written = 0;
    if (handles->input) WriteFile(handles->input, text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
}

void ChildProcess::closeInput() {
    closeHandle(handles->input);
}

void ChildProcess::kill() {
    TerminateProcess(handles->process, 1);
}

bool ChildProcess::wait(uint32_t timeoutMs) {
    return WaitForSingleObject(handles->process, timeoutMs == WAIT_UNTIL_EXIT ? INFINITE : timeoutMs) == WAIT_OBJECT_0;
}

int ChildProcess::exitCode() const {
    DWORD code = 0;
    GetExitCodeProcess(handles->process, &code);
    return static_cast<int>(code);
}

}

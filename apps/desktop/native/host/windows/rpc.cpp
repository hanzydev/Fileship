#include "host.h"

#include <host/platform.h>
#include <shared/log.h>

#include <aclapi.h>
#include <sddl.h>

#include <atomic>
#include <thread>

namespace fileship::host {

namespace {

constexpr DWORD BUFFER_SIZE = 64 * 1024;

std::atomic<bool> isStopping = false;
HANDLE stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
std::thread listenerThread;

class PipeChannel : public RpcChannel {
public:
    explicit PipeChannel(HANDLE pipe) : pipe(pipe) {}

    ~PipeChannel() override {
        CloseHandle(readEvent);
    }

    size_t read(char* buffer, size_t size) override {
        if (isStopping) return 0;

        OVERLAPPED overlapped{};
        overlapped.hEvent = readEvent;
        DWORD read = 0;
        ResetEvent(readEvent);

        if (!ReadFile(pipe, buffer, static_cast<DWORD>(size), nullptr, &overlapped) &&
            GetLastError() != ERROR_IO_PENDING && GetLastError() != ERROR_MORE_DATA)
            return 0;

        HANDLE events[] = {readEvent, stopEvent};
        if (WaitForMultipleObjects(2, events, FALSE, INFINITE) != WAIT_OBJECT_0) {
            CancelIoEx(pipe, &overlapped);
            GetOverlappedResult(pipe, &overlapped, &read, TRUE);
            return 0;
        }

        if (!GetOverlappedResult(pipe, &overlapped, &read, FALSE) && GetLastError() != ERROR_MORE_DATA) return 0;
        return read;
    }

    bool write(std::string const& data) override {
        OVERLAPPED overlapped{};
        overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        size_t offset = 0;
        bool isOk = true;

        while (offset < data.size()) {
            DWORD written = 0;
            ResetEvent(overlapped.hEvent);
            auto chunk = static_cast<DWORD>(std::min<size_t>(data.size() - offset, 1024 * 1024));

            if (!WriteFile(pipe, data.data() + offset, chunk, nullptr, &overlapped) &&
                GetLastError() != ERROR_IO_PENDING) {
                isOk = false;
                break;
            }

            if (!GetOverlappedResult(pipe, &overlapped, &written, TRUE) || !written) {
                isOk = false;
                break;
            }

            offset += written;
        }

        CloseHandle(overlapped.hEvent);
        return isOk;
    }

    void close() override {
        FlushFileBuffers(pipe);
        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }

private:
    HANDLE pipe;
    HANDLE readEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
};

std::wstring securityDescriptor() {
    HANDLE processToken = nullptr;
    std::wstring sid;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &processToken)) {
        DWORD size = 0;
        GetTokenInformation(processToken, TokenUser, nullptr, 0, &size);
        std::vector<uint8_t> buffer(size);

        if (GetTokenInformation(processToken, TokenUser, buffer.data(), size, &size)) {
            LPWSTR text = nullptr;
            if (ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid, &text)) {
                sid = text;
                LocalFree(text);
            }
        }

        CloseHandle(processToken);
    }

    return L"D:P(A;;GA;;;" + sid + L")";
}

HANDLE createPipe(bool isFirst) {
    static std::wstring const descriptor = securityDescriptor();

    PSECURITY_DESCRIPTOR security = nullptr;
    ConvertStringSecurityDescriptorToSecurityDescriptorW(descriptor.c_str(), SDDL_REVISION_1, &security, nullptr);

    SECURITY_ATTRIBUTES attributes{sizeof(attributes), security, FALSE};
    DWORD mode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | (isFirst ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0);

    HANDLE pipe = CreateNamedPipeW(pipeName().c_str(), mode, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT |
                                                                 PIPE_REJECT_REMOTE_CLIENTS,
                                   PIPE_UNLIMITED_INSTANCES, BUFFER_SIZE, BUFFER_SIZE, 0, &attributes);

    if (security) LocalFree(security);
    return pipe;
}

void listen() {
    bool isFirst = true;

    while (!isStopping) {
        HANDLE pipe = createPipe(isFirst);
        if (pipe == INVALID_HANDLE_VALUE) {
            logError(L"Could not create the host pipe: " + std::to_wstring(GetLastError()));
            if (WaitForSingleObject(stopEvent, 1000) == WAIT_OBJECT_0) return;
            continue;
        }

        isFirst = false;

        OVERLAPPED overlapped{};
        overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        bool isConnected = ConnectNamedPipe(pipe, &overlapped) != FALSE;
        DWORD error = GetLastError();

        if (!isConnected && error == ERROR_IO_PENDING) {
            HANDLE events[] = {overlapped.hEvent, stopEvent};
            if (WaitForMultipleObjects(2, events, FALSE, INFINITE) == WAIT_OBJECT_0) {
                DWORD ignored = 0;
                isConnected = GetOverlappedResult(pipe, &overlapped, &ignored, FALSE) != FALSE;
            } else {
                CancelIoEx(pipe, &overlapped);
            }
        } else if (error == ERROR_PIPE_CONNECTED) {
            isConnected = true;
        }

        CloseHandle(overlapped.hEvent);

        if (!isConnected || isStopping) {
            CloseHandle(pipe);
            continue;
        }

        std::thread(serveRpcChannel, std::make_shared<PipeChannel>(pipe)).detach();
    }
}

}

void startRpcServer() {
    listenerThread = std::thread(listen);
}

void stopRpcServer() {
    isStopping = true;
    SetEvent(stopEvent);
    if (listenerThread.joinable()) listenerThread.join();
}

bool sendToRunningHost(std::string const& line) {
    auto name = pipeName();
    HANDLE pipe = INVALID_HANDLE_VALUE;
    auto deadline = GetTickCount64() + 10000;

    while (GetTickCount64() < deadline) {
        pipe = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (pipe != INVALID_HANDLE_VALUE) break;

        if (GetLastError() == ERROR_PIPE_BUSY) WaitNamedPipeW(name.c_str(), 1000);
        else Sleep(100);
    }

    if (pipe == INVALID_HANDLE_VALUE) return false;

    DWORD written = 0;
    bool isSent = WriteFile(pipe, line.data(), static_cast<DWORD>(line.size()), &written, nullptr) != FALSE;

    char buffer[4096];
    DWORD read = 0;
    if (isSent) ReadFile(pipe, buffer, sizeof(buffer), &read, nullptr);

    CloseHandle(pipe);
    return isSent;
}

}

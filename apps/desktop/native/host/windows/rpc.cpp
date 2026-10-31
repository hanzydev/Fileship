#include <host/app.h>
#include <shared/log.h>

#include <windows.h>
#include <aclapi.h>
#include <sddl.h>

#include <atomic>
#include <mutex>
#include <thread>

namespace fileship::host {

namespace {

constexpr DWORD BUFFER_SIZE = 64 * 1024;

std::atomic<bool> isStopping = false;
HANDLE stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
std::thread listenerThread;

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

HANDLE createPipe(std::wstring const& name, bool isFirst) {
    static std::wstring const descriptor = securityDescriptor();

    PSECURITY_DESCRIPTOR security = nullptr;
    ConvertStringSecurityDescriptorToSecurityDescriptorW(descriptor.c_str(), SDDL_REVISION_1, &security, nullptr);

    SECURITY_ATTRIBUTES attributes{sizeof(attributes), security, FALSE};
    DWORD mode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | (isFirst ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0);

    HANDLE pipe = CreateNamedPipeW(name.c_str(), mode,
                                   PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
                                   PIPE_UNLIMITED_INSTANCES, BUFFER_SIZE, BUFFER_SIZE, 0, &attributes);

    if (security) LocalFree(security);
    return pipe;
}

class PipeConnection : public RpcConnection {
public:
    explicit PipeConnection(HANDLE pipe) : pipe(pipe) {
        readEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        writeEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    }

    ~PipeConnection() override {
        close();
        CloseHandle(readEvent);
        CloseHandle(writeEvent);
    }

    size_t read(char* buffer, size_t size) override {
        OVERLAPPED overlapped{};
        overlapped.hEvent = readEvent;
        ResetEvent(readEvent);
        DWORD read = 0;

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

    bool write(char const* data, size_t size) override {
        OVERLAPPED overlapped{};
        overlapped.hEvent = writeEvent;
        size_t offset = 0;

        while (offset < size) {
            DWORD written = 0;
            ResetEvent(writeEvent);
            auto chunk = static_cast<DWORD>(std::min<size_t>(size - offset, 1024 * 1024));

            if (!WriteFile(pipe, data + offset, chunk, nullptr, &overlapped) && GetLastError() != ERROR_IO_PENDING)
                return false;
            if (!GetOverlappedResult(pipe, &overlapped, &written, TRUE) || !written) return false;

            offset += written;
        }

        return true;
    }

    void close() override {
        std::lock_guard lock(mutex);
        if (pipe == INVALID_HANDLE_VALUE) return;

        FlushFileBuffers(pipe);
        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
        pipe = INVALID_HANDLE_VALUE;
    }

private:
    std::mutex mutex;
    HANDLE pipe;
    HANDLE readEvent = nullptr;
    HANDLE writeEvent = nullptr;
};

class ClientConnection : public RpcConnection {
public:
    explicit ClientConnection(HANDLE pipe) : pipe(pipe) {}

    ~ClientConnection() override {
        close();
    }

    size_t read(char* buffer, size_t size) override {
        DWORD read = 0;
        return ReadFile(pipe, buffer, static_cast<DWORD>(size), &read, nullptr) ? read : 0;
    }

    bool write(char const* data, size_t size) override {
        DWORD written = 0;
        return WriteFile(pipe, data, static_cast<DWORD>(size), &written, nullptr) != FALSE;
    }

    void close() override {
        if (pipe == INVALID_HANDLE_VALUE) return;
        CloseHandle(pipe);
        pipe = INVALID_HANDLE_VALUE;
    }

private:
    HANDLE pipe;
};

void listen(std::wstring name, std::function<void(std::shared_ptr<RpcConnection>)> onConnection) {
    bool isFirst = true;

    while (!isStopping) {
        HANDLE pipe = createPipe(name, isFirst);
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

        onConnection(std::make_shared<PipeConnection>(pipe));
    }
}

}

void startRpcListener(std::wstring const& name, std::function<void(std::shared_ptr<RpcConnection>)> onConnection) {
    listenerThread = std::thread(listen, name, std::move(onConnection));
}

void stopRpcListener() {
    isStopping = true;
    SetEvent(stopEvent);
    if (listenerThread.joinable()) listenerThread.join();
}

std::shared_ptr<RpcConnection> connectRpc(std::wstring const& name, int timeoutMs) {
    auto deadline = GetTickCount64() + timeoutMs;

    while (GetTickCount64() < deadline) {
        HANDLE pipe = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (pipe != INVALID_HANDLE_VALUE) return std::make_shared<ClientConnection>(pipe);

        if (GetLastError() == ERROR_PIPE_BUSY) WaitNamedPipeW(name.c_str(), 1000);
        else Sleep(100);
    }

    return nullptr;
}

}

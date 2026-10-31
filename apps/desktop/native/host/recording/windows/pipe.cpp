#include <host/recording/pipe.h>

#include <windows.h>

#include <stdexcept>

namespace fileship::host::recording {

struct AudioPipe::Channel {
    HANDLE pipe = INVALID_HANDLE_VALUE;
    HANDLE stopEvent = nullptr;
    HANDLE dataEvent = nullptr;
};

namespace {

constexpr DWORD PIPE_BUFFER = 1 << 20;

bool waitIo(HANDLE pipe, OVERLAPPED& overlapped, HANDLE stopEvent) {
    HANDLE events[] = {stopEvent, overlapped.hEvent};

    if (WaitForMultipleObjects(2, events, FALSE, INFINITE) == WAIT_OBJECT_0) {
        CancelIoEx(pipe, &overlapped);
        DWORD ignored = 0;
        GetOverlappedResult(pipe, &overlapped, &ignored, TRUE);
        return false;
    }

    DWORD transferred = 0;
    return GetOverlappedResult(pipe, &overlapped, &transferred, FALSE);
}

}

std::wstring audioPipeName(std::filesystem::path const&, std::wstring const& name) {
    return L"\\\\.\\pipe\\fileship-" + std::to_wstring(GetCurrentProcessId()) + L"-" + name;
}

AudioPipe::AudioPipe(std::wstring name, size_t priming)
    : pipeName(std::move(name)), primingBytes(priming), channel(std::make_unique<Channel>()) {
    channel->pipe =
        CreateNamedPipeW(pipeName.c_str(), PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
                         PIPE_TYPE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1, PIPE_BUFFER, 0, 0, nullptr);
    if (channel->pipe == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Could not create the audio pipe (" + std::to_string(GetLastError()) + ")");
    }

    channel->stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    channel->dataEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    thread = std::thread([this] { run(); });
}

AudioPipe::~AudioPipe() {
    SetEvent(channel->stopEvent);
    if (thread.joinable()) thread.join();

    if (channel->pipe != INVALID_HANDLE_VALUE) CloseHandle(channel->pipe);
    CloseHandle(channel->stopEvent);
    CloseHandle(channel->dataEvent);
}

void AudioPipe::write(char const* data, size_t size) {
    {
        std::lock_guard lock(mutex);
        if (!isConnected || isEnding) return;
        queue.emplace_back(data, data + size);
    }

    wake();
}

void AudioPipe::end() {
    {
        std::lock_guard lock(mutex);
        isEnding = true;
    }

    wake();
}

void AudioPipe::wake() {
    SetEvent(channel->dataEvent);
}

void AudioPipe::run() {
    auto pipe = channel->pipe;
    auto stopEvent = channel->stopEvent;
    OVERLAPPED overlapped{};
    overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    bool connected = ConnectNamedPipe(pipe, &overlapped) ? true : GetLastError() == ERROR_PIPE_CONNECTED;
    if (!connected && GetLastError() == ERROR_IO_PENDING) connected = waitIo(pipe, overlapped, stopEvent);

    if (connected) {
        std::lock_guard lock(mutex);
        if (!isEnding) {
            queue.emplace_front(primingBytes, '\0');
            isConnected = true;
        }
    }

    HANDLE events[] = {stopEvent, channel->dataEvent};

    while (connected) {
        std::vector<char> chunk;
        bool isDone = false;

        {
            std::lock_guard lock(mutex);
            if (!queue.empty()) {
                chunk = std::move(queue.front());
                queue.pop_front();
            } else {
                isDone = isEnding;
            }
        }

        if (isDone) break;

        if (chunk.empty()) {
            if (WaitForMultipleObjects(2, events, FALSE, INFINITE) == WAIT_OBJECT_0) break;
            continue;
        }

        ResetEvent(overlapped.hEvent);
        if (!WriteFile(pipe, chunk.data(), static_cast<DWORD>(chunk.size()), nullptr, &overlapped) &&
            (GetLastError() != ERROR_IO_PENDING || !waitIo(pipe, overlapped, stopEvent))) {
            break;
        }
    }

    {
        std::lock_guard lock(mutex);
        isConnected = false;
        queue.clear();
    }

    CloseHandle(pipe);
    channel->pipe = INVALID_HANDLE_VALUE;
    CloseHandle(overlapped.hEvent);
}

}

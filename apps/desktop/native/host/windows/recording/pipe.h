#pragma once

#include <windows.h>

#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace fileship::host::recording {

class AudioPipe {
public:
    AudioPipe(std::wstring name, size_t primingBytes);
    ~AudioPipe();

    AudioPipe(AudioPipe const&) = delete;
    AudioPipe& operator=(AudioPipe const&) = delete;

    std::wstring const& name() const { return pipeName; }
    void write(char const* data, size_t size);
    void end();

private:
    void run();

    std::wstring pipeName;
    size_t primingBytes;
    HANDLE pipe = INVALID_HANDLE_VALUE;
    HANDLE stopEvent = nullptr;
    HANDLE dataEvent = nullptr;
    std::mutex mutex;
    std::deque<std::vector<char>> queue;
    bool isConnected = false;
    bool isEnding = false;
    std::thread thread;
};

}

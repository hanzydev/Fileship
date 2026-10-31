#pragma once

#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace fileship::host::recording {

std::wstring audioPipeName(std::filesystem::path const& folder, std::wstring const& name);

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
    struct Channel;

    void run();
    void wake();

    std::wstring pipeName;
    size_t primingBytes;
    std::unique_ptr<Channel> channel;
    std::mutex mutex;
    std::deque<std::vector<char>> queue;
    bool isConnected = false;
    bool isEnding = false;
    std::thread thread;
};

}

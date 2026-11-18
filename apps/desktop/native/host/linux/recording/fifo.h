#pragma once

#include <atomic>
#include <string>

namespace fileship::host::recording {

std::string fifoPath(std::string const& name);

class Fifo {
public:
    explicit Fifo(std::string path);
    ~Fifo();

    Fifo(Fifo const&) = delete;
    Fifo& operator=(Fifo const&) = delete;

    bool open();
    bool write(char const* data, size_t size);
    bool wait(int milliseconds);
    void close();
    void cancel();

private:
    std::string path;
    int descriptor = -1;
    std::atomic<bool> isCanceled = false;
};

}

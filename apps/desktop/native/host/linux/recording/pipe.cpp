#include "fifo.h"

#include <host/recording/pipe.h>
#include <shared/utf8.h>

#include <unistd.h>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace fileship::host::recording {

namespace {

class FifoPipe : public AudioPipe {
public:
    FifoPipe(std::wstring name, size_t primingBytes) : pipeName(std::move(name)), fifo(toUtf8(pipeName)) {
        thread = std::thread([this, primingBytes] { run(primingBytes); });
    }

    ~FifoPipe() override {
        fifo.cancel();
        {
            std::lock_guard lock(mutex);
            isEnding = true;
            queue.clear();
        }
        changed.notify_all();
        thread.join();
    }

    std::wstring const& name() const override {
        return pipeName;
    }

    void write(char const* data, size_t size) override {
        {
            std::lock_guard lock(mutex);
            if (!isConnected || isEnding) return;
            queue.emplace_back(data, data + size);
        }

        changed.notify_all();
    }

    void end() override {
        {
            std::lock_guard lock(mutex);
            isEnding = true;
        }

        changed.notify_all();
    }

private:
    void run(size_t primingBytes) {
        if (!fifo.open()) return;

        {
            std::lock_guard lock(mutex);
            if (!isEnding) {
                queue.emplace_front(primingBytes, '\0');
                isConnected = true;
            }
        }

        for (;;) {
            std::vector<char> chunk;

            {
                std::unique_lock lock(mutex);
                changed.wait(lock, [this] { return !queue.empty() || isEnding; });
                if (queue.empty()) break;

                chunk = std::move(queue.front());
                queue.pop_front();
            }

            if (!fifo.write(chunk.data(), chunk.size())) break;
        }

        {
            std::lock_guard lock(mutex);
            isConnected = false;
            queue.clear();
        }

        fifo.close();
    }

    std::wstring pipeName;
    Fifo fifo;
    std::mutex mutex;
    std::condition_variable changed;
    std::deque<std::vector<char>> queue;
    bool isConnected = false;
    bool isEnding = false;
    std::thread thread;
};

}

std::wstring AudioPipe::nameOf(std::wstring const& label) {
    return toWide(fifoPath("audio-" + toUtf8(label)));
}

std::unique_ptr<AudioPipe> AudioPipe::create(std::wstring const& label, size_t primingBytes) {
    return std::make_unique<FifoPipe>(nameOf(std::to_wstring(getpid()) + L"-" + label), primingBytes);
}

}

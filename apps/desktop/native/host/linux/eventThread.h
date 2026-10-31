#pragma once

#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace fileship::host {

class EventThread {
public:
    struct Handlers {
        int fd = -1;
        std::function<void()> prepare = {};
        std::function<void(bool isReadable)> afterPoll = {};
        std::function<int()> timeout = {};
    };

    void start(Handlers handlers);
    void post(std::function<void()> work);
    void stopWatching();
    bool isCurrent() const;

    template <typename Result>
    Result run(std::function<Result()> work) {
        if (isCurrent()) return work();

        auto task = std::make_shared<std::packaged_task<Result()>>(std::move(work));
        auto future = task->get_future();
        post([task] { (*task)(); });
        return future.get();
    }

private:
    Handlers handlers;
    int wake = -1;
    std::thread::id id;
    std::mutex mutex;
    std::vector<std::function<void()>> queue;

    void loop();
};

}

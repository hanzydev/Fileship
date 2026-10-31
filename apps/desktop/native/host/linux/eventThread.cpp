#include "eventThread.h"

#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <cerrno>

namespace fileship::host {

void EventThread::start(Handlers value) {
    handlers = std::move(value);
    wake = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

    std::thread thread([this] { loop(); });
    id = thread.get_id();
    thread.detach();
}

void EventThread::post(std::function<void()> work) {
    {
        std::lock_guard lock(mutex);
        queue.push_back(std::move(work));
    }

    uint64_t one = 1;
    while (write(wake, &one, sizeof(one)) < 0 && errno == EINTR) {
    }
}

void EventThread::stopWatching() {
    handlers.fd = -1;
}

bool EventThread::isCurrent() const {
    return std::this_thread::get_id() == id;
}

void EventThread::loop() {
    while (true) {
        if (handlers.prepare) handlers.prepare();

        pollfd fds[2] = {{handlers.fd, POLLIN, 0}, {wake, POLLIN, 0}};
        int count = poll(fds, 2, handlers.timeout ? handlers.timeout() : -1);

        if (handlers.afterPoll) handlers.afterPoll(count > 0 && fds[0].fd >= 0 && fds[0].revents);
        if (count <= 0 || !(fds[1].revents & POLLIN)) continue;

        uint64_t value = 0;
        while (read(wake, &value, sizeof(value)) > 0) {
        }

        std::vector<std::function<void()>> work;

        {
            std::lock_guard lock(mutex);
            work.swap(queue);
        }

        for (auto& item : work) item();
    }
}

}

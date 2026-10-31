#include <host/recording/pipe.h>

#include <shared/utf8.h>

#include <fcntl.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstring>
#include <stdexcept>

namespace fileship::host::recording {

struct AudioPipe::Channel {
    std::string path;
    int fd = -1;
    int wakeFd = -1;
    std::atomic<bool> isStopping = false;
};

namespace {

constexpr int PIPE_BUFFER = 1 << 20;

bool waitWritable(int fd, int wakeFd, std::atomic<bool> const& isStopping) {
    pollfd items[] = {{fd, POLLOUT, 0}, {wakeFd, POLLIN, 0}};
    while (poll(items, 2, -1) < 0) {
        if (errno != EINTR) return false;
    }

    if (items[1].revents & POLLIN) {
        uint64_t value = 0;
        [[maybe_unused]] auto ignored = read(wakeFd, &value, sizeof(value));
    }
    return !isStopping && !(items[0].revents & (POLLERR | POLLHUP));
}

}

std::wstring audioPipeName(std::filesystem::path const& folder, std::wstring const& name) {
    return (folder / (name + L".pcm")).wstring();
}

AudioPipe::AudioPipe(std::wstring name, size_t priming)
    : pipeName(std::move(name)), primingBytes(priming), channel(std::make_unique<Channel>()) {
    channel->path = toUtf8(pipeName);
    unlink(channel->path.c_str());
    if (mkfifo(channel->path.c_str(), 0600) != 0) {
        throw std::runtime_error("Could not create the audio pipe (" + std::string(std::strerror(errno)) + ")");
    }

    channel->wakeFd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    thread = std::thread([this] { run(); });
}

AudioPipe::~AudioPipe() {
    channel->isStopping = true;
    wake();

    int unblock = open(channel->path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (thread.joinable()) thread.join();
    if (unblock >= 0) close(unblock);

    if (channel->fd >= 0) close(channel->fd);
    close(channel->wakeFd);
    unlink(channel->path.c_str());
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
    uint64_t one = 1;
    [[maybe_unused]] auto ignored = ::write(channel->wakeFd, &one, sizeof(one));
}

void AudioPipe::run() {
    int fd = -1;
    while ((fd = open(channel->path.c_str(), O_WRONLY | O_CLOEXEC)) < 0 && errno == EINTR) {
    }

    bool connected = fd >= 0 && !channel->isStopping;
    if (connected) {
        fcntl(fd, F_SETPIPE_SZ, PIPE_BUFFER);
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
        channel->fd = fd;

        std::lock_guard lock(mutex);
        if (!isEnding) {
            queue.emplace_front(primingBytes, '\0');
            isConnected = true;
        }
    }

    while (connected && !channel->isStopping) {
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
            pollfd item{channel->wakeFd, POLLIN, 0};
            poll(&item, 1, -1);
            uint64_t value = 0;
            [[maybe_unused]] auto ignored = read(channel->wakeFd, &value, sizeof(value));
            continue;
        }

        size_t offset = 0;
        while (offset < chunk.size()) {
            auto count = ::write(fd, chunk.data() + offset, chunk.size() - offset);
            if (count > 0) {
                offset += static_cast<size_t>(count);
            } else if (count < 0 && errno == EINTR) {
                continue;
            } else if (count < 0 && errno == EAGAIN && waitWritable(fd, channel->wakeFd, channel->isStopping)) {
                continue;
            } else {
                connected = false;
                break;
            }
        }
    }

    {
        std::lock_guard lock(mutex);
        isConnected = false;
        queue.clear();
    }

    if (fd >= 0) close(fd);
    channel->fd = -1;
}

}

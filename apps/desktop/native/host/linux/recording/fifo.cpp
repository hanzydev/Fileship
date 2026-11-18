#include "fifo.h"

#include <fcntl.h>
#include <glib.h>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <stdexcept>

namespace fileship::host::recording {

std::string fifoPath(std::string const& name) {
    return std::string(g_get_user_runtime_dir()) + "/fileship-" + name;
}

Fifo::Fifo(std::string value) : path(std::move(value)) {
    unlink(path.c_str());
    if (mkfifo(path.c_str(), 0600)) throw std::runtime_error("Could not create " + path);
}

Fifo::~Fifo() {
    close();
    unlink(path.c_str());
}

bool Fifo::open() {
    while (!isCanceled) {
        descriptor = ::open(path.c_str(), O_WRONLY | O_NONBLOCK | O_CLOEXEC);
        if (descriptor >= 0) return true;
        if (errno != ENXIO) return false;

        g_usleep(5000);
    }

    return false;
}

bool Fifo::write(char const* data, size_t size) {
    while (size && !isCanceled) {
        auto written = ::write(descriptor, data, size);

        if (written > 0) {
            data += written;
            size -= static_cast<size_t>(written);
        } else if (errno == EAGAIN) {
            pollfd waiting{descriptor, POLLOUT, 0};
            poll(&waiting, 1, 50);
        } else if (errno != EINTR) {
            return false;
        }
    }

    return !isCanceled;
}

bool Fifo::wait(int milliseconds) {
    g_usleep(milliseconds * 1000);
    return !isCanceled;
}

void Fifo::close() {
    if (descriptor >= 0) ::close(descriptor);
    descriptor = -1;
}

void Fifo::cancel() {
    isCanceled = true;
}

}

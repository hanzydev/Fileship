#include <host/app.h>
#include <host/platform.h>
#include <shared/log.h>

#include <poll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <thread>

namespace fileship::host {

namespace {

int stopSignal = eventfd(0, EFD_CLOEXEC);
int listener = -1;
std::thread listenerThread;

bool waitFor(int descriptor) {
    pollfd items[] = {{descriptor, POLLIN, 0}, {stopSignal, POLLIN, 0}};

    while (poll(items, 2, -1) < 0) {
        if (errno != EINTR) return false;
    }

    return !(items[1].revents & POLLIN);
}

sockaddr_un addressOf(std::string const& path) {
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, path.c_str(), sizeof(address.sun_path) - 1);
    return address;
}

class SocketChannel : public RpcChannel {
public:
    explicit SocketChannel(int descriptor) : descriptor(descriptor) {}

    size_t read(char* buffer, size_t size) override {
        if (!waitFor(descriptor)) return 0;

        ssize_t count = recv(descriptor, buffer, size, 0);
        return count > 0 ? static_cast<size_t>(count) : 0;
    }

    bool write(std::string const& data) override {
        size_t offset = 0;

        while (offset < data.size()) {
            ssize_t count = send(descriptor, data.data() + offset, data.size() - offset, MSG_NOSIGNAL);
            if (count <= 0) return false;
            offset += static_cast<size_t>(count);
        }

        return true;
    }

    void close() override {
        ::close(descriptor);
    }

private:
    int descriptor;
};

bool isOwnUser(int descriptor) {
    ucred peer{};
    socklen_t size = sizeof(peer);
    return getsockopt(descriptor, SOL_SOCKET, SO_PEERCRED, &peer, &size) == 0 && peer.uid == getuid();
}

void acceptClients() {
    while (waitFor(listener)) {
        int client = accept4(listener, nullptr, nullptr, SOCK_CLOEXEC);
        if (client < 0) continue;

        if (!isOwnUser(client)) {
            close(client);
            continue;
        }

        std::thread(serveRpcChannel, std::make_shared<SocketChannel>(client)).detach();
    }
}

}

void startRpcServer() {
    auto path = toUtf8(pipeName());
    auto address = addressOf(path);

    unlink(path.c_str());
    listener = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

    if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0 || listen(listener, 16) < 0) {
        logError(L"Could not open the host socket: " + toWide(strerror(errno)));
        return;
    }

    chmod(path.c_str(), 0600);
    listenerThread = std::thread(acceptClients);
}

void stopRpcServer() {
    uint64_t one = 1;
    if (write(stopSignal, &one, sizeof(one)) < 0) return;

    if (listenerThread.joinable()) listenerThread.join();
    close(listener);
    unlink(toUtf8(pipeName()).c_str());
}

bool sendToRunningHost(std::string const& line) {
    auto address = addressOf(toUtf8(pipeName()));
    int client = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    bool isConnected = false;

    for (int attempt = 0; attempt < 100 && !isConnected; attempt++) {
        isConnected = connect(client, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
        if (!isConnected) usleep(100 * 1000);
    }

    bool isSent = isConnected && send(client, line.data(), line.size(), MSG_NOSIGNAL) == static_cast<ssize_t>(line.size());

    char buffer[4096];
    if (isSent && recv(client, buffer, sizeof(buffer), 0) < 0) isSent = false;

    close(client);
    return isSent;
}

}

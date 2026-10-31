#include <host/app.h>
#include <shared/log.h>

#include <poll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>

namespace fileship::host {

namespace {

int stopEvent = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
std::atomic<bool> isStopping = false;
std::thread listenerThread;
std::string listenerPath;

std::optional<sockaddr_un> addressOf(std::string const& path) {
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    if (path.size() >= sizeof(address.sun_path)) return std::nullopt;
    std::memcpy(address.sun_path, path.c_str(), path.size() + 1);
    return address;
}

bool waitReadable(int fd) {
    pollfd targets[] = {{fd, POLLIN, 0}, {stopEvent, POLLIN, 0}};
    while (poll(targets, 2, -1) < 0) {
        if (errno != EINTR) return false;
    }
    return !(targets[1].revents & POLLIN) && targets[0].revents;
}

class SocketConnection : public RpcConnection {
public:
    explicit SocketConnection(int fd, bool isServer) : fd(fd), isServer(isServer) {}

    ~SocketConnection() override {
        close();
    }

    size_t read(char* buffer, size_t size) override {
        if (isServer && !waitReadable(fd)) return 0;

        while (true) {
            auto received = recv(fd, buffer, size, 0);
            if (received >= 0) return static_cast<size_t>(received);
            if (errno != EINTR) return 0;
        }
    }

    bool write(char const* data, size_t size) override {
        while (size) {
            auto sent = send(fd, data, size, MSG_NOSIGNAL);
            if (sent < 0) {
                if (errno == EINTR) continue;
                return false;
            }

            data += sent;
            size -= static_cast<size_t>(sent);
        }

        return true;
    }

    void close() override {
        std::lock_guard lock(mutex);
        if (fd < 0) return;

        shutdown(fd, SHUT_RDWR);
        ::close(fd);
        fd = -1;
    }

private:
    std::mutex mutex;
    int fd;
    bool isServer;
};

int connectTo(std::string const& path) {
    auto address = addressOf(path);
    if (!address) return -1;

    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) return -1;

    if (connect(fd, reinterpret_cast<sockaddr*>(&*address), sizeof(*address)) != 0) {
        ::close(fd);
        return -1;
    }

    return fd;
}

void listen(int server, std::function<void(std::shared_ptr<RpcConnection>)> onConnection) {
    while (!isStopping) {
        if (!waitReadable(server)) break;

        int client = accept4(server, nullptr, nullptr, SOCK_CLOEXEC);
        if (client < 0) continue;

        ucred credentials{};
        socklen_t length = sizeof(credentials);
        if (getsockopt(client, SOL_SOCKET, SO_PEERCRED, &credentials, &length) != 0 || credentials.uid != getuid()) {
            ::close(client);
            continue;
        }

        onConnection(std::make_shared<SocketConnection>(client, true));
    }

    ::close(server);
}

}

void startRpcListener(std::wstring const& name, std::function<void(std::shared_ptr<RpcConnection>)> onConnection) {
    listenerPath = toUtf8(name);
    auto address = addressOf(listenerPath);
    if (!address) {
        logError(L"The host socket path is too long: " + name);
        return;
    }

    std::filesystem::path folder = std::filesystem::path(listenerPath).parent_path();
    std::error_code ignored;
    std::filesystem::create_directories(folder, ignored);
    chmod(folder.c_str(), 0700);

    if (int existing = connectTo(listenerPath); existing >= 0) {
        ::close(existing);
        logError(L"Another host is already listening on " + name);
        return;
    }
    unlink(listenerPath.c_str());

    int server = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (server < 0 || bind(server, reinterpret_cast<sockaddr*>(&*address), sizeof(*address)) != 0 ||
        ::listen(server, SOMAXCONN) != 0) {
        logError(L"Could not create the host socket: " + toWide(std::strerror(errno)));
        if (server >= 0) ::close(server);
        return;
    }

    chmod(listenerPath.c_str(), 0600);
    listenerThread = std::thread(listen, server, std::move(onConnection));
}

void stopRpcListener() {
    isStopping = true;
    uint64_t signal = 1;
    [[maybe_unused]] auto written = ::write(stopEvent, &signal, sizeof(signal));
    if (listenerThread.joinable()) listenerThread.join();
    if (!listenerPath.empty()) unlink(listenerPath.c_str());
}

std::shared_ptr<RpcConnection> connectRpc(std::wstring const& name, int timeoutMs) {
    auto path = toUtf8(name);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);

    while (std::chrono::steady_clock::now() < deadline) {
        if (int fd = connectTo(path); fd >= 0) return std::make_shared<SocketConnection>(fd, false);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return nullptr;
}

}

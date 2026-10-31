#pragma once

#include "http.h"
#include "json.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace fileship {

class SocketIo {
public:
    using Handler = std::function<void(IJsonValue const& payload)>;

    explicit SocketIo(HttpEndpoint endpoint);
    ~SocketIo();

    SocketIo(SocketIo const&) = delete;
    SocketIo& operator=(SocketIo const&) = delete;

    void on(std::wstring const& event, Handler handler);
    void onReconnect(std::function<void()> handler);
    void onConnectError(std::function<void()> handler);
    void connect();
    void close();
    bool isConnected() const;
    void emit(std::wstring const& event, IJsonValue const& payload);

private:
    void run();
    bool open();
    void closeSocket();
    bool sendText(std::string const& text);
    void handlePacket(std::string const& packet);
    void dispatch(std::string const& data);
    bool waitFor(int delayMs);

    HttpEndpoint endpoint;
    std::map<std::wstring, Handler> handlers;
    std::function<void()> reconnectHandler;
    std::function<void()> connectErrorHandler;
    std::thread worker;
    std::mutex mutex;
    std::mutex sendMutex;
    std::condition_variable wake;
    std::shared_ptr<WebSocket> socket;
    std::atomic<bool> stopping = false;
    std::atomic<bool> connected = false;
    bool rejected = false;
    bool hasAttempted = false;
};

}

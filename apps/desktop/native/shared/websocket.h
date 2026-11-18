#pragma once

#include "http.h"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace fileship {

struct InvalidAddress : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class WebSocket {
public:
    WebSocket(HttpEndpoint const& endpoint, std::wstring const& path, int connectTimeoutMs, int receiveTimeoutMs);
    ~WebSocket();

    WebSocket(WebSocket const&) = delete;
    WebSocket& operator=(WebSocket const&) = delete;

    std::optional<std::string> receive();
    bool send(std::string const& text);
    void close();

private:
    struct State;
    std::unique_ptr<State> state;
};

}

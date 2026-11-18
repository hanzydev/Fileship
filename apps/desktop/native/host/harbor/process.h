#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace fileship::host::harbor {

class HelperProcess {
public:
    using LineHandler = std::function<void(std::string const& line)>;

    static std::unique_ptr<HelperProcess> start(std::filesystem::path const& executable);

    virtual ~HelperProcess() = default;

    virtual void listen(LineHandler onOutput, LineHandler onError, std::function<void()> onClosed) = 0;
    virtual void write(std::string const& text) = 0;
    virtual void closeInput() = 0;
    virtual std::optional<uint32_t> wait(int timeoutMs) = 0;
    virtual void terminate() = 0;
};

template <typename Handler>
void takeLines(std::string& buffer, Handler const& handle) {
    size_t newline = 0;

    while ((newline = buffer.find('\n')) != std::string::npos) {
        std::string line = buffer.substr(0, newline);
        buffer.erase(0, newline + 1);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) handle(line);
    }
}

}

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace fileship::host {

using ClipboardData = std::shared_ptr<std::string const>;

class ClipboardBackend {
public:
    virtual ~ClipboardBackend() = default;

    virtual bool write(std::vector<std::string> const& types, ClipboardData data) = 0;
    virtual std::vector<std::string> types() = 0;
    virtual std::optional<std::string> read(std::string const& type) = 0;
};

std::unique_ptr<ClipboardBackend> createDataControlClipboard();
std::unique_ptr<ClipboardBackend> createX11Clipboard();
std::optional<std::string> readPipe(int fd);
void writePipe(int fd, ClipboardData data);

}

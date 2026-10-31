#include "process.h"

#include <algorithm>

namespace fileship::host::recording {

std::string lastLine(std::string const& text) {
    size_t end = text.find_last_not_of(" \t\r\n");
    if (end == std::string::npos) return {};

    size_t start = text.find_last_of("\r\n", end);
    start = start == std::string::npos ? 0 : start + 1;

    return text.substr(start, std::min<size_t>(end - start + 1, 200));
}

void splitLines(std::string& partial, char const* data, size_t size, ChildProcess::LineHandler const& onLine) {
    partial.append(data, size);

    size_t start = 0;
    for (size_t end; (end = partial.find('\n', start)) != std::string::npos; start = end + 1) {
        auto line = partial.substr(start, end - start);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        onLine(line);
    }
    partial.erase(0, start);
}

}

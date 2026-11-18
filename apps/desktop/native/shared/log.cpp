#include "log.h"

#include "platform.h"
#include "utf8.h"

#include <fstream>
#include <mutex>

namespace fileship {

namespace {

constexpr uintmax_t MAX_LOG_SIZE = 1024 * 1024;

std::mutex mutex;
std::filesystem::path logFile;

}

void initLog(std::filesystem::path const& file) {
    std::lock_guard lock(mutex);
    logFile = file;

    std::error_code error;
    std::filesystem::create_directories(file.parent_path(), error);
    if (std::filesystem::file_size(file, error) > MAX_LOG_SIZE) {
        std::filesystem::rename(file, std::filesystem::path(file).replace_extension(L".old.log"), error);
    }
}

void writeLog(LogLevel level, std::wstring_view message) {
    auto line = "[" + logTimestamp() + "] [" +
                (level == LogLevel::Error ? "error" : level == LogLevel::Warn ? "warn" : "info") + "] " +
                toUtf8(message) + "\n";
    debugOutput(line);

    std::lock_guard lock(mutex);
    if (logFile.empty()) return;

    std::ofstream file(logFile, std::ios::binary | std::ios::app);
    file.write(line.data(), static_cast<std::streamsize>(line.size()));
}

}

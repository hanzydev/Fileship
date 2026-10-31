#include "log.h"

#include "system.h"
#include "utf8.h"

#include <chrono>
#include <cstdio>
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
    auto now = std::chrono::system_clock::now();
    auto time = localTime(std::chrono::system_clock::to_time_t(now));
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

    char prefix[64]{};
    snprintf(prefix, sizeof(prefix), "[%04d-%02d-%02d %02d:%02d:%02d.%03d] [%s] ", time.tm_year + 1900,
             time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec, static_cast<int>(milliseconds),
             level == LogLevel::Error ? "error" : level == LogLevel::Warn ? "warn" : "info");

    auto line = prefix + toUtf8(message) + "\n";
    debugOutput(line);

    std::lock_guard lock(mutex);
    if (logFile.empty()) return;

    std::ofstream file(logFile, std::ios::binary | std::ios::app);
    file.write(line.data(), static_cast<std::streamsize>(line.size()));
}

}

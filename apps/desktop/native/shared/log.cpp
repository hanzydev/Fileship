#include "log.h"

#include "utf8.h"

#include <cstdio>
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
    SYSTEMTIME time{};
    GetLocalTime(&time);

    char prefix[64]{};
    snprintf(prefix, sizeof(prefix), "[%04u-%02u-%02u %02u:%02u:%02u.%03u] [%s] ", time.wYear, time.wMonth,
             time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds,
             level == LogLevel::Error ? "error" : level == LogLevel::Warn ? "warn" : "info");

    auto line = prefix + toUtf8(message) + "\n";
    OutputDebugStringW(toWide(line).c_str());

    std::lock_guard lock(mutex);
    if (logFile.empty()) return;

    FILE* file = nullptr;
    if (_wfopen_s(&file, logFile.c_str(), L"ab") != 0) return;

    fwrite(line.data(), 1, line.size(), file);
    fclose(file);
}

}

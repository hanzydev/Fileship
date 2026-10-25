#pragma once

#include <filesystem>
#include <string_view>

namespace fileship {

enum class LogLevel { Info, Warn, Error };

void initLog(std::filesystem::path const& file);
void writeLog(LogLevel level, std::wstring_view message);

inline void logInfo(std::wstring_view message) {
    writeLog(LogLevel::Info, message);
}

inline void logWarn(std::wstring_view message) {
    writeLog(LogLevel::Warn, message);
}

inline void logError(std::wstring_view message) {
    writeLog(LogLevel::Error, message);
}

}

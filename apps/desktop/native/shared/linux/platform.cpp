#include <shared/platform.h>
#include <shared/utf8.h>

#include <glib.h>

#include <cstdio>
#include <ctime>

namespace fileship {

std::string logTimestamp() {
    timespec now{};
    clock_gettime(CLOCK_REALTIME, &now);

    tm local{};
    localtime_r(&now.tv_sec, &local);

    char text[96]{};
    snprintf(text, sizeof(text), "%04d-%02d-%02d %02d:%02d:%02d.%03d", local.tm_year + 1900, local.tm_mon + 1,
             local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec, static_cast<int>(now.tv_nsec / 1000000));
    return text;
}

void debugOutput(std::string const& line) {
    fputs(line.c_str(), stderr);
}

std::wstring systemLanguage() {
    return toWide(g_get_language_names()[0]);
}

std::wstring exceptionMessage(std::exception_ptr error) {
    try {
        std::rethrow_exception(error);
    } catch (std::exception const& failure) {
        return toWide(failure.what());
    } catch (...) {
        return L"Unknown error";
    }
}

}

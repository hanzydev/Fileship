#include <shared/platform.h>
#include <shared/utf8.h>

#include <windows.h>

#include <winrt/base.h>

#include <cstdio>

namespace fileship {

std::string logTimestamp() {
    SYSTEMTIME time{};
    GetLocalTime(&time);

    char text[32]{};
    snprintf(text, sizeof(text), "%04u-%02u-%02u %02u:%02u:%02u.%03u", time.wYear, time.wMonth, time.wDay,
             time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
    return text;
}

void debugOutput(std::string const& line) {
    OutputDebugStringW(toWide(line).c_str());
}

std::wstring systemLanguage() {
    ULONG count = 0;
    ULONG size = 0;
    GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, nullptr, &size);

    std::wstring buffer(size, L'\0');
    if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, buffer.data(), &size)) return {};

    return std::wstring(buffer.c_str());
}

std::wstring exceptionMessage(std::exception_ptr error) {
    try {
        std::rethrow_exception(error);
    } catch (winrt::hresult_error const& failure) {
        return std::wstring(failure.message());
    } catch (std::exception const& failure) {
        return toWide(failure.what());
    } catch (...) {
        return L"Unknown error";
    }
}

}

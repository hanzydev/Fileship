#include <shared/system.h>
#include <shared/utf8.h>

#include <windows.h>

#include <bcrypt.h>

#include <winrt/base.h>

namespace fileship {

void randomBytes(void* buffer, size_t size) {
    BCryptGenRandom(nullptr, static_cast<PUCHAR>(buffer), static_cast<ULONG>(size), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
}

std::vector<std::wstring> preferredLanguages() {
    ULONG count = 0;
    ULONG size = 0;
    GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, nullptr, &size);

    std::wstring buffer(size, L'\0');
    if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, buffer.data(), &size)) return {};

    std::vector<std::wstring> result;
    for (auto const* item = buffer.c_str(); *item; item += wcslen(item) + 1) result.emplace_back(item);
    return result;
}

std::tm localTime(std::time_t time) {
    std::tm result{};
    localtime_s(&result, &time);
    return result;
}

void debugOutput(std::string_view line) {
    OutputDebugStringW(toWide(line).c_str());
}

std::wstring describeException(std::exception_ptr error) {
    try {
        std::rethrow_exception(error);
    } catch (std::exception const& failure) {
        return toWide(failure.what());
    } catch (winrt::hresult_error const& failure) {
        return std::wstring(failure.message());
    } catch (...) {
        return L"Unknown error";
    }
}

}

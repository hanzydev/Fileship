#include <shared/system.h>
#include <shared/utf8.h>

#include <sys/random.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>

namespace fileship {

void randomBytes(void* buffer, size_t size) {
    auto bytes = static_cast<char*>(buffer);
    size_t filled = 0;

    while (filled < size) {
        auto read = getrandom(bytes + filled, size - filled, 0);
        if (read > 0) filled += static_cast<size_t>(read);
    }
}

std::vector<std::wstring> preferredLanguages() {
    std::vector<std::wstring> result;

    auto add = [&](std::string value) {
        value = value.substr(0, value.find_first_of(".@"));
        if (value.empty() || value == "C" || value == "POSIX") return;
        for (auto& character : value) {
            if (character == '_') character = '-';
        }
        result.push_back(toWide(value));
    };

    if (auto list = std::getenv("LANGUAGE"); list && *list) {
        std::string text(list);
        for (size_t start = 0; start <= text.size();) {
            auto end = text.find(':', start);
            add(text.substr(start, end == std::string::npos ? std::string::npos : end - start));
            if (end == std::string::npos) break;
            start = end + 1;
        }
    }

    for (auto name : {"LC_ALL", "LC_MESSAGES", "LANG"}) {
        if (auto value = std::getenv(name); value && *value) {
            add(value);
            break;
        }
    }

    return result;
}

std::tm localTime(std::time_t time) {
    std::tm result{};
    localtime_r(&time, &result);
    return result;
}

void debugOutput(std::string_view line) {
    if (isatty(STDERR_FILENO)) std::fwrite(line.data(), 1, line.size(), stderr);
}

std::wstring describeException(std::exception_ptr error) {
    try {
        std::rethrow_exception(error);
    } catch (std::exception const& failure) {
        return toWide(failure.what());
    } catch (...) {
        return L"Unknown error";
    }
}

uint32_t currentProcessId() {
    return static_cast<uint32_t>(getpid());
}

}

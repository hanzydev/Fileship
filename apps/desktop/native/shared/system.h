#pragma once

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <exception>
#include <string>
#include <string_view>
#include <vector>

namespace fileship {

void randomBytes(void* buffer, size_t size);
std::vector<std::wstring> preferredLanguages();
std::tm localTime(std::time_t time);
void debugOutput(std::string_view line);
std::wstring describeException(std::exception_ptr error);
uint32_t currentProcessId();

}

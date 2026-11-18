#pragma once

#include <exception>
#include <string>

namespace fileship {

std::string logTimestamp();
void debugOutput(std::string const& line);
std::wstring systemLanguage();
std::wstring exceptionMessage(std::exception_ptr error = std::current_exception());

}

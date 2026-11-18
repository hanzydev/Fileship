#pragma once

#include <string>
#include <string_view>

namespace fileship {

std::wstring toWide(std::string_view text);
std::string toUtf8(std::wstring_view text);

}

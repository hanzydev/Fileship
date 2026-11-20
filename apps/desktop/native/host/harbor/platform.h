#pragma once

#include <filesystem>
#include <string>

namespace fileship::host::harbor {

extern wchar_t const SEPARATOR;

std::filesystem::path helperPath();
std::filesystem::path homeFolder();
std::wstring stampOf(std::filesystem::path const& path);
std::wstring pathKey(std::wstring const& path);
bool isHiddenName(std::wstring const& name);
bool confirmPublic(std::wstring const& message, std::wstring const& detail);
void openFolder(std::filesystem::path const& folder);
void forgetHelperSession();

}

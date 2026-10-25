#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fileship {

using I18nParams = std::vector<std::pair<std::wstring, std::wstring>>;

void initI18n(std::filesystem::path const& directory);
std::vector<std::wstring> languages();
std::wstring resolveLanguage(std::wstring const& setting);
bool setLanguage(std::wstring const& code);
std::wstring language();
std::wstring t(std::wstring_view key, I18nParams const& params = {});

}

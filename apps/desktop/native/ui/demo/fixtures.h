#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace fileship::ui::demo {

void print(std::string const& text);
void savePng(std::filesystem::path const& path, int width, int height, std::vector<uint8_t> const& bgra);
std::vector<uint8_t> fixtureSnapshot(int width, int height);
int runFixture(std::filesystem::path const& directory);

}

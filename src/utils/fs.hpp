#pragma once

#include <vector>

#include "common.hpp"

namespace utils {

fs::path get_local_appdata();

std::vector<std::byte> read_file(const fs::path & path);

void write_file(const fs::path & path, const std::vector<std::byte> & contents);

std::vector<fs::path> find_slack_app_folders(const fs::path & slack_path);

} // namespace utils

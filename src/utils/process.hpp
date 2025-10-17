#pragma once

#include <optional>
#include <string_view>

#include "common.hpp"

namespace utils {

struct process_close_info {
    fs::path exe_path;
    bool     was_foreground = false;
};

// Find and terminate all processes with the given executable name
// Returns the exe path and whether any window was in the foreground
std::optional<process_close_info> close_process(std::wstring_view process_name);

// Start a process with the specified show command (SW_SHOWNORMAL, SW_SHOWMINIMIZED, etc.)
bool start_process(const fs::path & exe_path, int show_command);

} // namespace utils

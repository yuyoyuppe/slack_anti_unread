#include <vector>
#include <iterator>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <wil/win32_helpers.h>

#include "common.hpp"
#include "logging.hpp"

namespace utils {

void setup_logging() {
    const auto temp_path = wil::TryGetEnvironmentVariableW(L"TEMP");
    if(!temp_path)
        return;

    wil::unique_cotaskmem_string exe_path;
    if(FAILED(wil::QueryFullProcessImageNameW(GetCurrentProcess(), 0, exe_path)))
        return;

    const fs::path exe_file{exe_path.get()};
    const auto     log_file = fs::path{temp_path.get()} / (exe_file.stem().wstring() + L".log");

    constexpr size_t max_file_size = 1024 * 1024; // 1MB
    constexpr size_t max_files     = 3;

    auto file_sink =
      std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_file.string(), max_file_size, max_files);

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    std::vector<spdlog::sink_ptr> sinks{file_sink, console_sink};
    auto                          logger = std::make_shared<spdlog::logger>("main", begin(sinks), end(sinks));

    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
}

} // namespace utils

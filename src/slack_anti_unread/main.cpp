#include <vector>
#include <optional>
#include <iterator>
#include <thread>
#include <chrono>

#include <Windows.h>
#include <spdlog/spdlog.h>
#include <wil/filesystem.h>

#include <utils/common.hpp>
#include <utils/fs.hpp>
#include <utils/logging.hpp>
#include <utils/process.hpp>

using namespace std::chrono_literals;

struct patch_result {
    bool                                     icons_patched = false;
    std::optional<utils::process_close_info> slack_info;
};

patch_result patch_slack_app_icons(const fs::path & app_folder) {
    patch_result result;

    const auto icons_folder = app_folder / L"resources/app.asar.unpacked/dist/resources";

    const auto rest_icon_path   = icons_folder / L"slack-taskbar-rest.ico";
    const auto unread_icon_path = icons_folder / L"slack-taskbar-unread.ico";

    spdlog::info("Checking app folder: {}", app_folder.filename().string());

    try {
        const auto rest_icon   = utils::read_file(rest_icon_path);
        const auto unread_icon = utils::read_file(unread_icon_path);

        if(rest_icon == unread_icon) {
            spdlog::info("Icons are identical - no action needed");
            return result;
        }

        spdlog::info("Icons are different - patching unread icon");
        utils::write_file(unread_icon_path, rest_icon);
        result.icons_patched = true;
    } catch(const std::system_error & e) { spdlog::warn("{}", e.what()); }

    return result;
}

void restart_slack(const utils::process_close_info & slack_info) {
    const int  show_command = slack_info.was_foreground ? SW_SHOWNORMAL : SW_SHOWMINIMIZED;
    const auto state_str    = slack_info.was_foreground ? "normal" : "minimized";

    spdlog::info("Reopening Slack ({})...", state_str);
    if(!utils::start_process(slack_info.exe_path, show_command)) {
        spdlog::warn("Failed to reopen Slack automatically");
        return;
    }
    spdlog::info("Slack restarted successfully");
}

void patch_all_slack_installations(const fs::path & slack_base_path) {
    spdlog::info("Checking Slack installation at: {}", slack_base_path.string());

    const auto app_folders = utils::find_slack_app_folders(slack_base_path);
    if(empty(app_folders)) {
        spdlog::info("No Slack app folders found");
        return;
    }

    bool                                     any_icons_patched = false;
    std::optional<utils::process_close_info> slack_info;

    for(const auto & app_folder : app_folders) {
        const auto result = patch_slack_app_icons(app_folder);

        const bool need_to_close_slack = result.icons_patched && !any_icons_patched;
        if(need_to_close_slack) {
            spdlog::info("Closing Slack...");
            slack_info        = utils::close_process(L"slack.exe");
            any_icons_patched = true;
        }
    }

    if(!any_icons_patched)
        return;

    if(!slack_info) {
        spdlog::warn("Could not determine Slack executable path");
        return;
    }

    restart_slack(*slack_info);
}

void watch_for_slack_updates(const fs::path & slack_folder) {
    spdlog::info("Watching for Slack updates in: {}", slack_folder.string());

    const auto on_change = [slack_folder](const wil::FolderChangeEvent event, PCWSTR fileName) {
        if(event != wil::FolderChangeEvent::Added)
            return;

        const fs::path changed_path(fileName);
        const auto     folder_name = changed_path.filename().wstring();

        const bool is_slack_app = folder_name.starts_with(L"app-");
        if(!is_slack_app)
            return;

        spdlog::info("New Slack version detected: {}", changed_path.string());

        // Give Slack time to finish installation
        std::this_thread::sleep_for(2s);

        try {
            patch_all_slack_installations(slack_folder);
        } catch(const std::exception & e) { spdlog::error("Error patching new installation: {}", e.what()); }
    };

    auto reader = wil::make_folder_change_reader(slack_folder.c_str(),
                                                 true, // recursive
                                                 wil::FolderChangeEvents::All,
                                                 on_change);

    // Keep the folder change reader alive
    while(true)
        std::this_thread::sleep_for(10s);
}

int main() {
    try {
#ifdef NDEBUG
        const HWND console_window = GetConsoleWindow();
        if(console_window)
            ShowWindow(console_window, SW_HIDE);
#endif

        utils::setup_logging();

        const auto slack_folder = utils::get_local_appdata() / L"slack";

        patch_all_slack_installations(slack_folder);
        watch_for_slack_updates(slack_folder);

        return 0;
    } catch(const std::exception & e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    }
}

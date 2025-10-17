#include <stdexcept>
#include <iterator>

#include <wil/resource.h>
#include <wil/win32_helpers.h>

#include "fs.hpp"

namespace utils {

fs::path get_local_appdata() {
    if(const auto env_var = wil::TryGetEnvironmentVariableW(L"LOCALAPPDATA"))
        return fs::path{env_var.get()};
    else
        throw std::runtime_error{"Failed to get LOCALAPPDATA environment variable"};
}

std::vector<std::byte> read_file(const fs::path & path) {
    wil::unique_hfile file{
      CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};

    THROW_LAST_ERROR_IF_MSG(!file, "Failed to open file for reading: %ls", path.c_str());

    LARGE_INTEGER file_size{};
    THROW_LAST_ERROR_IF_MSG(!GetFileSizeEx(file.get(), &file_size), "Failed to get file size: %ls", path.c_str());

    std::vector<std::byte> buffer{static_cast<size_t>(file_size.QuadPart)};
    DWORD                  _{};
    THROW_LAST_ERROR_IF_MSG(!ReadFile(file.get(), data(buffer), static_cast<DWORD>(size(buffer)), &_, nullptr),
                            "Failed to read file: %ls",
                            path.c_str());

    return buffer;
}

void write_file(const fs::path & path, const std::vector<std::byte> & contents) {
    wil::unique_hfile file{
      CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)};

    THROW_LAST_ERROR_IF_MSG(!file, "Failed to open file for writing: %ls", path.c_str());

    DWORD bytes_written{};
    THROW_LAST_ERROR_IF_MSG(
      !WriteFile(file.get(), data(contents), static_cast<DWORD>(size(contents)), &bytes_written, nullptr),
      "Failed to write to file: %ls",
      path.c_str());

    if(bytes_written != size(contents))
        throw std::runtime_error{std::format(
          "Incomplete write to file: {} (wrote {}/{} bytes)", path.string(), bytes_written, size(contents))};
}

std::vector<fs::path> find_slack_app_folders(const fs::path & slack_path) {
    std::vector<fs::path> folders;
    const auto            search_path = slack_path / L"app-*";

    WIN32_FIND_DATAW  find_data = {};
    wil::unique_hfind find_handle{FindFirstFileW(search_path.c_str(), &find_data)};
    if(!find_handle)
        return folders;

    do {
        const bool is_directory = find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        if(!is_directory) {
            find_data = {};
            continue;
        }

        const std::wstring_view folder_name{find_data.cFileName};
        const bool              is_slack_app = folder_name.starts_with(L"app-");
        if(is_slack_app)
            folders.push_back(slack_path / folder_name);

        find_data = {};
    } while(FindNextFileW(find_handle.get(), &find_data));

    return folders;
}

} // namespace utils

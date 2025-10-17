#include <vector>
#include <algorithm>
#include <iterator>

#include <Windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <wil/resource.h>

#include "process.hpp"

namespace {

bool iequals(const std::wstring_view a, const std::wstring_view b) {
    if(size(a) != size(b))
        return false;

    return equal(begin(a), end(a), begin(b), end(b), [](const wchar_t ca, const wchar_t cb) {
        return towlower(ca) == towlower(cb);
    });
}

bool is_process_window_foreground(const DWORD process_id) {
    const HWND foreground_window = GetForegroundWindow();
    if(!foreground_window)
        return false;

    DWORD foreground_pid = 0;
    GetWindowThreadProcessId(foreground_window, &foreground_pid);

    return foreground_pid == process_id;
}

} // namespace

namespace utils {

std::optional<process_close_info> close_process(const std::wstring_view process_name) {
    const wil::unique_handle snapshot{CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)};
    if(!snapshot)
        return std::nullopt;

    PROCESSENTRY32W pe32 = {.dwSize = sizeof(PROCESSENTRY32W)};

    std::optional<process_close_info> info;
    std::vector<wil::unique_handle>   process_handles;

    if(!Process32FirstW(snapshot.get(), &pe32))
        return std::nullopt;

    do {
        const bool is_target_process = iequals(pe32.szExeFile, process_name);
        if(!is_target_process)
            continue;

        wil::unique_handle process{
          OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pe32.th32ProcessID)};
        if(!process)
            continue;

        if(!info) {
            wchar_t    path_buffer[MAX_PATH];
            DWORD      path_size = MAX_PATH;
            const bool got_path  = QueryFullProcessImageNameW(process.get(), 0, path_buffer, &path_size);
            if(got_path) {
                info = process_close_info{.exe_path       = fs::path(std::wstring_view(path_buffer, path_size)),
                                          .was_foreground = is_process_window_foreground(pe32.th32ProcessID)};
            }
        } else if(!info->was_foreground) {
            // Check if any other instance has a foreground window
            if(is_process_window_foreground(pe32.th32ProcessID))
                info->was_foreground = true;
        }

        if(!TerminateProcess(process.get(), 0))
            continue;

        process_handles.push_back(std::move(process));
    } while(Process32NextW(snapshot.get(), &pe32));

    for(const auto & process : process_handles)
        WaitForSingleObject(process.get(), INFINITE);

    return info;
}

bool start_process(const fs::path & exe_path, const int show_command) {
    SHELLEXECUTEINFOW sei = {.cbSize       = sizeof(SHELLEXECUTEINFOW),
                             .fMask        = SEE_MASK_NOASYNC,
                             .hwnd         = nullptr,
                             .lpVerb       = L"open",
                             .lpFile       = exe_path.c_str(),
                             .lpParameters = nullptr,
                             .lpDirectory  = nullptr,
                             .nShow        = show_command,
                             .hInstApp     = nullptr};

    return ShellExecuteExW(&sei);
}

} // namespace utils

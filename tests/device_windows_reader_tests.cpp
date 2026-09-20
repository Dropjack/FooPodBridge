// SPDX-License-Identifier: LGPL-3.0-or-later
#include "database_test_support.h"
#include "core/device/windows_reader.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>

int main() {
    using namespace foopodbridge::core::device;
    using foopodbridge::tests::require;
    namespace fs = std::filesystem;
    // Never use attached volumes; only a newly created process-specific temp directory.
    const auto dir = fs::temp_directory_path() / (L"foopodbridge-reader-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
    try {
        require(fs::create_directory(dir), "temporary directory collision");
        fs::create_directories(dir / "iPod_Control" / "iTunes");
        const auto root = dir.wstring() + L"\\";
        const auto path = dir / "iPod_Control" / "iTunes" / "iTunesDB";
        const auto bytes = foopodbridge::tests::make_empty().original_bytes;
        { std::ofstream output(path, std::ios::binary); output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())); }
        const auto before = fs::last_write_time(path);
        const auto result = detail::read_relative(root, L"iPod_Control\\iTunes\\iTunesDB", 1024 * 1024, {});
        require(result.problem == reason::none && result.bytes == bytes, "native read did not match input");
        require(fs::last_write_time(path) == before, "read changed last-write time");
        require(detail::read_relative(root, L"iPod_Control\\iTunes\\iTunesDB", 1, {}).problem == reason::resource_limit, "size limit ignored");
        require(detail::read_relative(root, L"iPod_Control\\iTunes\\missing", 1024, {}).problem == reason::database_missing, "missing result wrong");
        require(detail::read_relative(root, L"..\\outside", 1024, {}).problem == reason::unsafe_path, "escape accepted");
        require(detail::read_relative(root, L"C:\\outside", 1024, {}).problem == reason::unsafe_path, "absolute accepted");
        std::stop_source cancelled; cancelled.request_stop();
        require(detail::read_relative(root, L"iPod_Control\\iTunes\\iTunesDB", 1024 * 1024, cancelled.get_token()).problem == reason::cancelled, "cancel ignored");
        HANDLE occupied = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        require(occupied != INVALID_HANDLE_VALUE, "test lock failed");
        auto locked = detail::read_relative(root, L"iPod_Control\\iTunes\\iTunesDB", 1024 * 1024, {});
        CloseHandle(occupied);
        require(locked.problem == reason::sharing_violation, "sharing conflict called corruption");
        fs::rename(path, path.string() + ".moved"); // proves the production reader released handles
        fs::remove_all(dir);
        std::cout << "Native read-only file access, cancellation, limits, sharing and handle release passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        // Leave failed evidence inside the unique computer temp directory.
        return 1;
    }
}

// SPDX-License-Identifier: LGPL-3.0-or-later
#include "database_test_support.h"
#include "core/device/windows_reader.h"
#include "core/device/windows_identity.h"
#include "foopodbridge/core/transaction/transaction.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <array>
#include <set>

int main() {
    using namespace foopodbridge::core::device;
    using foopodbridge::tests::require;
    namespace fs = std::filesystem;
    // Never use attached volumes; only a newly created process-specific temp directory.
    const auto dir = fs::temp_directory_path() / (L"foopodbridge-reader-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
    try {
        // Synthetic PnP tree: disk and USB nodes are one physical iPod, even
        // when Windows returns differently cased IDs. A second iPod stays separate.
        const std::array<std::wstring, 5> ids{L"ROOT", L"USB\\vid_05ac&pid_1209\\fake-a",
            L"USBSTOR\\DISK&VEN_APPLE&PROD_IPOD\\fake-a", L"USB\\VID_05AC&PID_1209\\FAKE-B",
            L"1394\\apple_computer__inc.&ipod\\fake-c"};
        const std::array<int, 5> parents{-1, 0, 1, 0, 0};
        auto key = [&](int node) { return detail::physical_ancestor(node,
            [&](int n) { return ids.at(n); }, [&](int n, int& parent) { parent = parents.at(n); return parent >= 0; }); };
        require(key(2) == key(1) && key(1) == L"USB\\VID_05AC&PID_1209\\FAKE-A", "disk/USB identity split");
        std::set<std::wstring> mounted{key(2)};
        require(mounted.contains(key(1)), "mounted USB node would appear as NotMounted");
        require(!mounted.contains(key(3)), "different physical devices merged");
        require(key(0).empty(), "unrelated root identified as iPod");
        require(key(4) == L"1394\\APPLE_COMPUTER__INC.&IPOD\\FAKE-C", "FireWire identity lost");
        require(detail::physical_ancestor(0, [](int) { return std::wstring(L"ROOT"); },
            [](int n, int& p) { p = n; return true; }).empty(), "cyclic ancestry not bounded");
        const std::array<std::wstring, 5> nested_ids{L"ROOT", L"USB\\VID_05AC&PID_1261\\PHYSICAL-A",
            L"USB\\VID_05AC&PID_1261&MI_00\\INTERFACE-A", L"USBSTOR\\DISK&VEN_APPLE&PROD_IPOD\\A",
            L"USB\\VID_05AC&PID_1261\\PHYSICAL-B"};
        const std::array<int, 5> nested_parents{-1, 0, 1, 2, 0};
        const auto chain = detail::physical_ancestors(3,
            [&](int n) { return nested_ids.at(n); },
            [&](int n, int& parent) { parent = nested_parents.at(n); return parent >= 0; });
        require(chain.size() == 2U && chain[0] == L"USB\\VID_05AC&PID_1261&MI_00\\INTERFACE-A" &&
            chain[1] == L"USB\\VID_05AC&PID_1261\\PHYSICAL-A",
            "mounted disk did not retain both Apple ancestor aliases");
        const std::set<std::wstring> mounted_chain(chain.begin(), chain.end());
        require(mounted_chain.contains(detail::physical_ancestor(1,
            [&](int n) { return nested_ids.at(n); },
            [&](int n, int& parent) { parent = nested_parents.at(n); return parent >= 0; })),
            "physical parent of a mounted iPod was emitted as a second NotMounted device");
        require(!mounted_chain.contains(detail::physical_ancestor(4,
            [&](int n) { return nested_ids.at(n); },
            [&](int n, int& parent) { parent = nested_parents.at(n); return parent >= 0; })),
            "a different iPod with the same model was hidden");
        require(fs::create_directory(dir), "temporary directory collision");
        fs::create_directories(dir / "iPod_Control" / "iTunes");
        const auto root = dir.wstring() + L"\\";
        require(detail::probe_recovery(root, {}).pending == 0, "absent journal directory");
        fs::create_directory(dir / ".foopodbridge");
        { std::ofstream bad(dir / ".foopodbridge" / "broken.journal"); bad << "invalid"; }
        require(detail::probe_recovery(root, {}).invalid == 1, "invalid journal hidden");
        require(fs::exists(dir / ".foopodbridge" / "broken.journal"), "probe deleted evidence");
        const auto seal = [](const std::string& body) {
            const auto* data = reinterpret_cast<const std::byte*>(body.data());
            return body + foopodbridge::core::transaction::sha256(std::span(data, body.size())) + "\n";
        };
        const std::string hash(64, 'a');
        const auto journal = seal("FPBTXN1\nfixture-operation\nfixture-device\n1 1 0\n\"iPod_Control/iTunes/iTunesDB\"\n" + hash + "\n" + hash + "\n0\n0\n");
        { std::ofstream output(dir / ".foopodbridge" / "fixture-operation.journal", std::ios::binary); output << journal; }
        require(detail::probe_recovery(root, {}).pending == 1, "native pending journal");
        { std::ofstream output(dir / ".foopodbridge" / "fixture-operation.journal.done", std::ios::binary); output << seal(hash + "\n"); }
        require(detail::probe_recovery(root, {}).pending == 0, "native completed journal");
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

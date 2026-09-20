// SPDX-License-Identifier: LGPL-3.0-or-later
#include "foopodbridge/core/device/device.h"
#include "windows_reader.h"
#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <winioctl.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace foopodbridge::core::device {
namespace {
struct handle {
    HANDLE value{INVALID_HANDLE_VALUE};
    handle() = default;
    explicit handle(HANDLE h) : value(h) {}
    ~handle() { if (value != INVALID_HANDLE_VALUE && value != nullptr) CloseHandle(value); }
    handle(const handle&) = delete;
    handle& operator=(const handle&) = delete;
    handle(handle&& h) noexcept : value(std::exchange(h.value, INVALID_HANDLE_VALUE)) {}
    bool valid() const { return value != INVALID_HANDLE_VALUE && value != nullptr; }
};
struct device_set {
    HDEVINFO value;
    ~device_set() { if (value != INVALID_HANDLE_VALUE) SetupDiDestroyDeviceInfoList(value); }
};
std::string utf8(std::wstring_view text) {
    if (text.empty()) return {};
    const auto count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (count <= 0) throw std::runtime_error("invalid device text");
    std::string out(static_cast<std::size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), out.data(), count, nullptr, nullptr);
    return out;
}
std::wstring wide(std::string_view text) {
    if (text.empty()) return {};
    const auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (count <= 0) throw std::runtime_error("invalid device text");
    std::wstring out(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), out.data(), count);
    return out;
}
std::wstring node_id(DEVINST node) {
    std::array<wchar_t, MAX_DEVICE_ID_LEN> id{};
    if (CM_Get_Device_IDW(node, id.data(), static_cast<ULONG>(id.size()), 0) != CR_SUCCESS) return {};
    return id.data();
}
std::string apple_ancestor(DEVINST node) {
    for (unsigned depth = 0; depth < 16; ++depth) {
        auto id = node_id(node);
        for (auto& ch : id) if (ch >= L'a' && ch <= L'z') ch -= L'a' - L'A';
        if (id.starts_with(L"USB\\VID_05AC&PID_") || id.starts_with(L"1394\\APPLE_COMPUTER__INC.&IPOD")) return utf8(id);
        DEVINST parent{};
        if (CM_Get_Parent(&parent, node, 0) != CR_SUCCESS) break;
        node = parent;
    }
    return {};
}
reason last_reason(DWORD e) {
    switch (e) {
    case ERROR_FILE_NOT_FOUND: case ERROR_PATH_NOT_FOUND: return reason::database_missing;
    case ERROR_ACCESS_DENIED: return reason::access_denied;
    case ERROR_SHARING_VIOLATION: case ERROR_LOCK_VIOLATION: return reason::sharing_violation;
    case ERROR_DEVICE_NOT_CONNECTED: case ERROR_NOT_READY: case ERROR_NO_MEDIA_IN_DRIVE: return reason::removed;
    case ERROR_OPERATION_ABORTED: return reason::cancelled;
    default: return reason::io_failure;
    }
}
std::optional<DWORD> disk_number(HANDLE file) {
    STORAGE_DEVICE_NUMBER number{}; DWORD returned{};
    if (DeviceIoControl(file, IOCTL_STORAGE_GET_DEVICE_NUMBER, nullptr, 0, &number, sizeof(number), &returned, nullptr)
        && returned >= sizeof(number) && number.DeviceType == FILE_DEVICE_DISK) return number.DeviceNumber;
    return {};
}
struct disk { DWORD number; std::string hardware; std::string node; };
std::vector<disk> disks() {
    device_set set{SetupDiGetClassDevsW(&GUID_DEVINTERFACE_DISK, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE)};
    if (set.value == INVALID_HANDLE_VALUE) throw std::runtime_error("disk enumeration unavailable");
    std::vector<disk> found;
    for (DWORD index = 0;; ++index) {
        SP_DEVICE_INTERFACE_DATA iface{}; iface.cbSize = sizeof(iface);
        if (!SetupDiEnumDeviceInterfaces(set.value, nullptr, &GUID_DEVINTERFACE_DISK, index, &iface)) {
            if (GetLastError() != ERROR_NO_MORE_ITEMS) throw std::runtime_error("disk enumeration failed");
            break;
        }
        DWORD bytes{};
        SetupDiGetDeviceInterfaceDetailW(set.value, &iface, nullptr, 0, &bytes, nullptr);
        if (bytes < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) || bytes > 65536) continue;
        std::vector<std::byte> storage(bytes);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(storage.data());
        detail->cbSize = sizeof(*detail);
        SP_DEVINFO_DATA info{}; info.cbSize = sizeof(info);
        if (!SetupDiGetDeviceInterfaceDetailW(set.value, &iface, detail, bytes, nullptr, &info)) continue;
        const auto ancestor = apple_ancestor(info.DevInst);
        if (ancestor.empty() || identify(ancestor).excluded) continue;
        handle file(CreateFileW(detail->DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
        if (!file.valid()) continue;
        if (const auto number = disk_number(file.value)) found.push_back({*number, ancestor, utf8(node_id(info.DevInst))});
    }
    return found;
}
bool volume_on_disk(std::wstring root, DWORD expected) {
    if (!root.empty() && root.back() == L'\\') root.pop_back();
    handle file(CreateFileW(root.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
    if (!file.valid()) return false;
    if (const auto actual = disk_number(file.value)) return *actual == expected;
    std::array<std::byte, sizeof(VOLUME_DISK_EXTENTS) + sizeof(DISK_EXTENT) * 16> buf{};
    DWORD returned{};
    if (!DeviceIoControl(file.value, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, buf.data(), static_cast<DWORD>(buf.size()), &returned, nullptr)) return false;
    const auto* extents = reinterpret_cast<const VOLUME_DISK_EXTENTS*>(buf.data());
    return returned >= sizeof(VOLUME_DISK_EXTENTS) && extents->NumberOfDiskExtents == 1 && extents->Extents[0].DiskNumber == expected;
}

// Hold each directory without DELETE sharing while opening the next component.
// Never follow directory junctions or file reparse points into another volume.
file_result read_relative_impl(const std::wstring& root, const std::wstring& relative, std::size_t limit, std::stop_token cancel) {
    if (relative.empty() || relative.front() == L'\\' || relative.find(L':') != std::wstring::npos ||
        relative.find(L'/') != std::wstring::npos || relative.find(L"..") != std::wstring::npos)
        return {{}, reason::unsafe_path};
    std::vector<handle> directories;
    std::wstring current = root;
    std::size_t start = 0;
    while (true) {
        if (cancel.stop_requested()) return {{}, reason::cancelled};
        const auto end = relative.find(L'\\', start);
        if (end == std::wstring::npos) break;
        current += relative.substr(start, end - start);
        handle directory(CreateFileW(current.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        if (!directory.valid()) return {{}, last_reason(GetLastError())};
        BY_HANDLE_FILE_INFORMATION info{};
        if (!GetFileInformationByHandle(directory.value, &info)) return {{}, last_reason(GetLastError())};
        if ((info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) || !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) return {{}, reason::unsafe_path};
        directories.push_back(std::move(directory));
        current += L'\\'; start = end + 1;
    }
    current += relative.substr(start);
    handle file(CreateFileW(current.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
    if (!file.valid()) return {{}, last_reason(GetLastError())};
    BY_HANDLE_FILE_INFORMATION before{}, after{};
    if (!GetFileInformationByHandle(file.value, &before)) return {{}, last_reason(GetLastError())};
    if (before.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) return {{}, reason::unsafe_path};
    const auto size = (static_cast<std::uint64_t>(before.nFileSizeHigh) << 32) | before.nFileSizeLow;
    if (size > limit) return {{}, reason::resource_limit};
    file_result result; result.bytes.resize(static_cast<std::size_t>(size));
    handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!event.valid()) return {{}, reason::io_failure};
    std::stop_callback cancellation(cancel, [&] { CancelIoEx(file.value, nullptr); });
    for (std::size_t pos = 0; pos < result.bytes.size();) {
        if (cancel.stop_requested()) return {{}, reason::cancelled};
        OVERLAPPED operation{}; operation.hEvent = event.value;
        operation.Offset = static_cast<DWORD>(pos); operation.OffsetHigh = static_cast<DWORD>(static_cast<std::uint64_t>(pos) >> 32);
        ResetEvent(event.value);
        const auto chunk = static_cast<DWORD>(std::min<std::size_t>(1024 * 1024, result.bytes.size() - pos));
        DWORD count{};
        if (!ReadFile(file.value, result.bytes.data() + pos, chunk, &count, &operation)) {
            if (GetLastError() != ERROR_IO_PENDING) return {{}, last_reason(GetLastError())};
            while (WaitForSingleObject(event.value, 50) == WAIT_TIMEOUT) {
                if (cancel.stop_requested()) CancelIoEx(file.value, &operation);
            }
            if (!GetOverlappedResult(file.value, &operation, &count, TRUE)) return {{}, last_reason(GetLastError())};
        }
        if (count != chunk) return {{}, reason::content_changed};
        pos += count;
    }
    if (!GetFileInformationByHandle(file.value, &after)) return {{}, last_reason(GetLastError())};
    if (before.nFileSizeHigh != after.nFileSizeHigh || before.nFileSizeLow != after.nFileSizeLow ||
        CompareFileTime(&before.ftLastWriteTime, &after.ftLastWriteTime) != 0 || before.nFileIndexLow != after.nFileIndexLow || before.nFileIndexHigh != after.nFileIndexHigh)
        return {{}, reason::content_changed};
    return result;
}
std::string property(std::string_view text, std::string_view key) {
    const auto xml_key = "<key>" + std::string(key) + "</key>";
    const auto xml = text.find(xml_key);
    if (xml != std::string_view::npos) {
        auto start = text.find("<string>", xml + xml_key.size());
        const auto next_key = text.find("<key>", xml + xml_key.size());
        if (start == std::string_view::npos || (next_key != std::string_view::npos && start > next_key)) return {};
        start += 8;
        const auto end = text.find("</string>", start);
        if (end == std::string_view::npos || end - start > 256) return {};
        const auto value = text.substr(start, end - start);
        if (value.find_first_of("<&\r\n") != std::string_view::npos) return {};
        return std::string(value);
    }
    for (std::size_t start = 0; start < text.size();) {
        const auto end = text.find('\n', start);
        auto line = text.substr(start, end == std::string_view::npos ? text.size() - start : end - start);
        const auto colon = line.find(':');
        if (colon != std::string_view::npos && line.substr(0, colon) == key) {
            line.remove_prefix(colon + 1);
            while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.remove_prefix(1);
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.remove_suffix(1);
            if (line.size() <= 256) return std::string(line);
        }
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    return {};
}
bool hex_identity(std::string_view value) {
    return value.size() == 16 && std::all_of(value.begin(), value.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    });
}
void enrich(candidate& c, std::stop_token stop) {
    const auto root = wide(c.volume_key);
    std::array<wchar_t, 64> fs{};
    if (!GetVolumeInformationW(root.c_str(), nullptr, 0, nullptr, nullptr, nullptr, fs.data(), static_cast<DWORD>(fs.size()))) {
        c.problem = last_reason(GetLastError()); return;
    }
    c.filesystem_supported = _wcsicmp(fs.data(), L"FAT32") == 0 || _wcsicmp(fs.data(), L"FAT") == 0;
    ULARGE_INTEGER free{}, total{};
    c.capacity_known = GetDiskFreeSpaceExW(root.c_str(), &free, &total, nullptr) != FALSE;
    if (c.capacity_known) { c.capacity = total.QuadPart; c.available = free.QuadPart; }
    if (!c.filesystem_supported || !identify(c.hardware_id).positive) return;
    const auto tail = c.hardware_id.substr(c.hardware_id.find_last_of('\\') + 1);
    if (hex_identity(tail)) { c.signing_identity = tail; c.identity_complete = true; }
    for (const auto* relative : {L"iPod_Control\\Device\\SysInfo", L"iPod_Control\\Device\\SysInfoExtended"}) {
        const auto data = read_relative_impl(root, relative, 1024 * 1024, stop);
        if (data.problem == reason::database_missing) continue;
        if (data.problem != reason::none) { c.problem = data.problem; return; }
        const std::string text(data.bytes.empty() ? "" : reinterpret_cast<const char*>(data.bytes.data()), data.bytes.size());
        auto serial = property(text, "SerialNumber");
        if (serial.empty()) serial = property(text, "pszSerialNumber");
        if (!serial.empty()) c.serial_suffix = serial.substr(serial.size() > 11 ? serial.size() - 4 : serial.size() > 3 ? serial.size() - 3 : 0);
        auto guid = property(text, "FirewireGuid");
        if (guid.empty()) guid = property(text, "FireWireGUID");
        if (guid.starts_with("0x")) guid.erase(0, 2);
        if (hex_identity(guid)) {
            if (!c.signing_identity.empty() && _stricmp(guid.c_str(), c.signing_identity.c_str()) != 0) { c.mapping_valid = false; return; }
            c.signing_identity = guid; c.identity_complete = true;
        }
    }
    // Existence is an observation only; these formats are never parsed as iTunesDB.
    for (const auto* name : {L"iPod_Control\\iTunes\\iTunesCDB", L"iPod_Control\\iTunes\\iTunes Library.itlp"}) {
        const auto probe = read_relative_impl(root, name, 0, stop);
        if (probe.problem == reason::none || probe.problem == reason::resource_limit || probe.problem == reason::unsafe_path)
            c.alternative_database = true;
        else if (probe.problem != reason::database_missing) { c.problem = probe.problem; return; }
    }
    const auto artwork = read_relative_impl(root, L"iPod_Control\\Artwork\\ArtworkDB", 0, stop);
    c.artwork_present = artwork.problem == reason::none || artwork.problem == reason::resource_limit;
}

class windows_backend final : public read_backend {
public:
    ~windows_backend() override { unwatch(); }
    std::vector<candidate> enumerate(std::stop_token cancel) override {
        const auto known_disks = disks();
        std::vector<candidate> result;
        std::set<std::string> mounted;
        std::array<wchar_t, 1024> root{};
        const auto volume_search = FindFirstVolumeW(root.data(), static_cast<DWORD>(root.size()));
        if (volume_search == INVALID_HANDLE_VALUE) throw std::runtime_error("volume enumeration unavailable");
        struct closer { HANDLE h; ~closer() { FindVolumeClose(h); } } guard{volume_search};
        do {
            if (cancel.stop_requested()) return {};
            for (const auto& disk : known_disks) {
                if (!volume_on_disk(root.data(), disk.number)) continue;
                candidate c; c.physical_key = disk.hardware; c.hardware_id = disk.hardware;
                c.volume_key = utf8(root.data());
                enrich(c, cancel);
                mounted.insert(disk.hardware);
                result.push_back(std::move(c));
            }
        } while (FindNextVolumeW(volume_search, root.data(), static_cast<DWORD>(root.size())));
        if (GetLastError() != ERROR_NO_MORE_FILES) throw std::runtime_error("volume enumeration failed");
        // Devices without a disk/volume must still be visible as NotMounted.
        device_set all{SetupDiGetClassDevsW(nullptr, nullptr, nullptr, DIGCF_PRESENT | DIGCF_ALLCLASSES)};
        if (all.value == INVALID_HANDLE_VALUE) throw std::runtime_error("device enumeration unavailable");
        for (DWORD index = 0;; ++index) {
            SP_DEVINFO_DATA info{}; info.cbSize = sizeof(info);
            if (!SetupDiEnumDeviceInfo(all.value, index, &info)) {
                if (GetLastError() != ERROR_NO_MORE_ITEMS) throw std::runtime_error("device enumeration failed");
                break;
            }
            if (cancel.stop_requested()) return {};
            const auto id = utf8(node_id(info.DevInst));
            if (identify(id).positive && !mounted.contains(id)) {
                candidate c; c.hardware_id = id; c.physical_key = id; c.mounted = false;
                result.push_back(std::move(c)); mounted.insert(id);
            }
        }
        return result;
    }
    file_result read_database(const candidate& target, std::stop_token cancel) override {
        if (!still_present(target)) return {{}, reason::removed};
        auto result = read_relative_impl(wide(target.volume_key), L"iPod_Control\\iTunes\\iTunesDB", 512ULL * 1024 * 1024, cancel);
        if (!cancel.stop_requested() && !still_present(target)) return {{}, reason::removed};
        return result;
    }
    bool still_present(const candidate& target) override {
        if (!target.mounted) return true;
        for (const auto& disk : disks()) {
            if (disk.hardware == target.physical_key && volume_on_disk(wide(target.volume_key), disk.number)) return true;
        }
        return false;
    }
    void watch(std::function<void()> callback) override {
        changed_ = std::move(callback);
        CM_NOTIFY_FILTER filter{}; filter.cbSize = sizeof(filter);
        filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
        filter.Flags = CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES;
        if (CM_Register_Notification(&filter, this, &notify, &notification_) != CR_SUCCESS) {
            changed_ = {}; throw std::runtime_error("device notifications unavailable");
        }
    }
    void unwatch() noexcept override {
        if (notification_) { CM_Unregister_Notification(notification_); notification_ = nullptr; }
        changed_ = {};
    }
private:
    static DWORD CALLBACK notify(HCMNOTIFICATION, PVOID context, CM_NOTIFY_ACTION action, PCM_NOTIFY_EVENT_DATA, DWORD) {
        if (action == CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL || action == CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL) {
            try { auto* self = static_cast<windows_backend*>(context); if (self->changed_) self->changed_(); } catch (...) {}
        }
        return ERROR_SUCCESS;
    }
    HCMNOTIFICATION notification_{};
    std::function<void()> changed_;
};
}
std::unique_ptr<read_backend> make_windows_backend() { return std::make_unique<windows_backend>(); }
file_result detail::read_relative(const std::wstring& root, const std::wstring& relative, std::size_t limit, std::stop_token cancel) {
    return read_relative_impl(root, relative, limit, cancel);
}
} // namespace foopodbridge::core::device

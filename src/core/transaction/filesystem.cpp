#include "internal.h"
#include <algorithm>
#include <cwctype>
#include <set>

namespace foopodbridge::core::transaction {
namespace {
class handle final {
public:
    explicit handle(HANDLE value) : value_(value) { if (value == INVALID_HANDLE_VALUE) throw failure("filesystem_io_" + std::to_string(GetLastError())); }
    ~handle() { if (value_ != INVALID_HANDLE_VALUE) CloseHandle(value_); }
    handle(handle&& other) noexcept : value_(other.value_) { other.value_ = INVALID_HANDLE_VALUE; }
    handle(const handle&) = delete;
    operator HANDLE() const { return value_; }
private:
    HANDLE value_;
};
void ordinary(HANDLE file, bool directory) {
    BY_HANDLE_FILE_INFORMATION info{};
    if (!GetFileInformationByHandle(file, &info) || (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
        (((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) != directory) ||
        (!directory && info.nNumberOfLinks != 1)) throw failure("unsafe_file");
}
handle open_directory(const std::filesystem::path& path) {
    handle h(CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    ordinary(h, true);
    return h;
}
class local final : public filesystem {
public:
    explicit local(const std::filesystem::path& path) : root_(std::filesystem::absolute(path).lexically_normal()) {
        if (root_ == root_.root_path() || root_.wstring().rfind(L"\\\\", 0) == 0 ||
            GetDriveTypeW(root_.root_path().c_str()) != DRIVE_FIXED) throw failure("invalid_test_root");
        std::array<wchar_t, MAX_PATH> volume{}, format{};
        if (!GetVolumePathNameW(root_.c_str(), volume.data(), static_cast<DWORD>(volume.size())) ||
            !GetVolumeInformationW(volume.data(), nullptr, 0, nullptr, nullptr, nullptr, format.data(), static_cast<DWORD>(format.size())) ||
            _wcsicmp(format.data(), L"NTFS") != 0) throw failure("offline_ntfs_directory_required");
        auto current = root_.root_path();
        // Pin every ancestor without FILE_SHARE_DELETE; check each for junctions.
        for (const auto& segment : root_.relative_path()) { current /= segment; roots_.push_back(open_directory(current)); }
        if (roots_.empty()) throw failure("invalid_test_root");
    }
    std::optional<std::uint64_t> size(const std::string& path) override {
        auto pins = parents(path, false);
        const auto full = resolve(path);
        const auto attrs = GetFileAttributesW(full.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) {
            const auto error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) return std::nullopt;
            throw failure("stat_failed");
        }
        handle h(CreateFileW(full.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        ordinary(h, false);
        LARGE_INTEGER length{};
        if (!GetFileSizeEx(h, &length) || length.QuadPart < 0) throw failure("stat_failed");
        return static_cast<std::uint64_t>(length.QuadPart);
    }
    std::size_t read(const std::string& path, std::uint64_t offset, std::span<std::byte> out) override {
        auto pins = parents(path, false);
        handle h(CreateFileW(resolve(path).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        ordinary(h, false);
        if (offset > static_cast<std::uint64_t>(std::numeric_limits<LONGLONG>::max()) || out.size() > MAXDWORD) throw failure("read_bounds");
        LARGE_INTEGER pos{}; pos.QuadPart = static_cast<LONGLONG>(offset);
        DWORD count{};
        if (!SetFilePointerEx(h, pos, nullptr, FILE_BEGIN) || !ReadFile(h, out.data(), static_cast<DWORD>(out.size()), &count, nullptr)) throw failure("read_failed");
        return count;
    }
    void create(const std::string& path) override {
        auto pins = parents(path, true);
        handle h(CreateFileW(resolve(path).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        ordinary(h, false);
    }
    void append(const std::string& path, std::span<const std::byte> data) override {
        auto pins = parents(path, false);
        handle h(CreateFileW(resolve(path).c_str(), FILE_APPEND_DATA, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        ordinary(h, false);
        DWORD written{};
        if (data.size() > MAXDWORD || !WriteFile(h, data.data(), static_cast<DWORD>(data.size()), &written, nullptr) || written != data.size()) throw failure("short_write");
    }
    void flush(const std::string& path) override {
        auto pins = parents(path, false);
        handle h(CreateFileW(resolve(path).c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        ordinary(h, false);
        if (!FlushFileBuffers(h)) throw failure("flush_failed");
    }
    void rename(const std::string& from, const std::string& to) override {
        auto a = parents(from, false); auto b = parents(to, true);
        if (!size(from) || size(to)) throw failure("rename_conflict");
        if (!MoveFileExW(resolve(from).c_str(), resolve(to).c_str(), MOVEFILE_WRITE_THROUGH)) throw failure("rename_failed");
    }
    void remove(const std::string& path) override {
        auto pins = parents(path, false);
        if (!size(path)) return;
        if (!DeleteFileW(resolve(path).c_str())) throw failure("remove_failed");
    }
    std::uint64_t available() override {
        ULARGE_INTEGER free{};
        if (!GetDiskFreeSpaceExW(root_.c_str(), &free, nullptr, nullptr)) throw failure("space_failed");
        return free.QuadPart;
    }
    std::vector<std::string> list(const std::string& directory) override {
        auto pins = parents(directory + "/entry", false);
        std::vector<std::string> entries;
        std::error_code ec;
        std::filesystem::directory_iterator it(resolve(directory), ec);
        if (ec == std::errc::no_such_file_or_directory) return entries;
        if (ec) throw failure("list_failed");
        for (const auto& item : it) {
            const auto u8 = item.path().filename().u8string();
            const std::string name(reinterpret_cast<const char*>(u8.data()), u8.size());
            const auto rel = directory + "/" + name;
            const auto attributes = GetFileAttributesW(resolve(rel).c_str());
            if (attributes == INVALID_FILE_ATTRIBUTES) throw failure("list_invalid");
            if ((attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) throw failure("unsafe_file");
            entries.push_back(name);
        }
        return entries;
    }
    bool is_directory(const std::string& path) override {
        auto pins = parents(path, false);
        const auto attributes = GetFileAttributesW(resolve(path).c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES) throw failure("directory_stat_failed");
        if ((attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) throw failure("unsafe_file");
        if (attributes & FILE_ATTRIBUTE_DIRECTORY) { auto pin = open_directory(resolve(path)); }
        return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }
private:
    std::filesystem::path resolve(const std::string& path) const {
        if (!safe_path(path)) throw failure("unsafe_path");
        const auto* first = reinterpret_cast<const char8_t*>(path.data());
        return root_ / std::filesystem::path(std::u8string(first, first + path.size()));
    }
    std::vector<handle> parents(const std::string& path, bool create_missing) {
        const auto relative = resolve(path).lexically_relative(root_).parent_path();
        auto current = root_;
        std::vector<handle> pins;
        for (const auto& segment : relative) {
            current /= segment;
            auto attr = GetFileAttributesW(current.c_str());
            if (attr == INVALID_FILE_ATTRIBUTES) {
                const auto error = GetLastError();
                if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND) throw failure("parent_access");
                if (!create_missing) break;
                if (!CreateDirectoryW(current.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) throw failure("mkdir_failed");
            }
            pins.push_back(open_directory(current));
        }
        return pins;
    }
    std::filesystem::path root_;
    std::vector<handle> roots_;
};
}

bool safe_path(const std::string& path) {
    if (path.empty() || path.size() > 512 || path.front() == '/' || path.back() == '/') return false;
    std::size_t start{};
    while (start < path.size()) {
        const auto end = path.find('/', start);
        auto part = path.substr(start, end == std::string::npos ? end : end - start);
        if (part.empty() || part == "." || part == ".." || part.back() == '.' || part.back() == ' ') return false;
        for (const unsigned char c : part) if (c < 32 || c == 127 || std::string("\\:*?\"<>|").find(static_cast<char>(c)) != std::string::npos) return false;
        auto base = part.substr(0, part.find('.'));
        std::transform(base.begin(), base.end(), base.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        if (base == "CON" || base == "PRN" || base == "AUX" || base == "NUL" ||
            (base.size() == 4 && (base.substr(0, 3) == "COM" || base.substr(0, 3) == "LPT") && base[3] >= '0' && base[3] <= '9')) return false;
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return true;
}
std::unique_ptr<filesystem> local_directory(const std::filesystem::path& root) { return std::make_unique<local>(root); }
std::string sha256(std::span<const std::byte> data) { detail::hash hash; hash.add(data); return hash.finish(); }
bytes read_all(filesystem& fs, const std::string& path, std::uint64_t limit) {
    const auto size = fs.size(path);
    if (!size || *size > limit || *size > std::numeric_limits<std::size_t>::max()) throw failure("read_limit");
    bytes out(static_cast<std::size_t>(*size));
    std::size_t offset{};
    while (offset < out.size()) {
        const auto wanted = std::min<std::size_t>(out.size() - offset, 1024 * 1024);
        const auto count = fs.read(path, offset, std::span(out).subspan(offset, wanted));
        if (!count || count > wanted) throw failure("short_read");
        offset += count;
    }
    if (fs.size(path) != size) throw failure("file_changed");
    return out;
}
std::string fingerprint(filesystem& fs, const std::string& path) {
    const auto size = fs.size(path);
    if (!size) throw failure("missing_file");
    detail::hash hash;
    bytes buffer(1024 * 1024);
    std::uint64_t offset{};
    while (offset < *size) {
        const auto wanted = static_cast<std::size_t>(std::min<std::uint64_t>(buffer.size(), *size - offset));
        const auto count = fs.read(path, offset, std::span(buffer).first(wanted));
        if (!count || count > wanted) throw failure("short_read");
        hash.add(std::span(buffer).first(count)); offset += count;
    }
    if (fs.size(path) != size) throw failure("file_changed");
    return hash.finish();
}
std::vector<std::string> enumerate_files(filesystem& fs, const std::string& root) {
    if (!safe_path(root) || !fs.is_directory(root)) throw failure("baseline_root");
    std::vector<std::string> pending{root}, files;
    std::set<std::wstring> seen;
    std::size_t count{};
    while (!pending.empty()) {
        const auto directory = pending.back(); pending.pop_back();
        for (const auto& name : fs.list(directory)) {
            if (name.find('/') != std::string::npos || !safe_path(name)) throw failure("enumeration_name");
            const auto path = directory + "/" + name;
            if (!safe_path(path) || ++count > 1000000) throw failure("enumeration_limit");
            const auto* first = reinterpret_cast<const char8_t*>(path.data());
            auto normalized = std::filesystem::path(std::u8string(first, first + path.size())).wstring();
            std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](wchar_t c) { return static_cast<wchar_t>(towupper(c)); });
            if (!seen.insert(normalized).second) throw failure("enumeration_duplicate");
            if (fs.is_directory(path)) pending.push_back(path);
            else { if (!fs.size(path)) throw failure("enumeration_changed"); files.push_back(path); }
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}
namespace {
std::filesystem::path normalized_root(const std::filesystem::path& p) {
    auto text = std::filesystem::absolute(p).lexically_normal().wstring();
    std::transform(text.begin(), text.end(), text.begin(), [](wchar_t c) { return static_cast<wchar_t>(towupper(c)); });
    return text;
}
bool nested(const std::filesystem::path& a, const std::filesystem::path& b) {
    auto x = a.begin(), y = b.begin();
    for (; x != a.end() && y != b.end(); ++x, ++y) if (*x != *y) return false;
    return x == a.end();
}
class baseline_reader final : public filesystem {
public:
    baseline_reader(filesystem& fs, identity expected, std::function<identity()> current)
        : fs_(fs), expected_(std::move(expected)), current_(std::move(current)) {}
    void check() { if (current_() != expected_) throw failure("baseline_identity_changed"); }
    std::optional<std::uint64_t> size(const std::string& p) override { check(); return fs_.size(p); }
    std::size_t read(const std::string& p, std::uint64_t o, std::span<std::byte> b) override { check(); return fs_.read(p, o, b); }
    std::vector<std::string> list(const std::string& p) override { check(); return fs_.list(p); }
    bool is_directory(const std::string& p) override { check(); return fs_.is_directory(p); }
    std::uint64_t available() override { check(); return fs_.available(); }
    void create(const std::string&) override { throw failure("baseline_read_only"); }
    void append(const std::string&, std::span<const std::byte>) override { throw failure("baseline_read_only"); }
    void flush(const std::string&) override { throw failure("baseline_read_only"); }
    void rename(const std::string&, const std::string&) override { throw failure("baseline_read_only"); }
    void remove(const std::string&) override { throw failure("baseline_read_only"); }
private:
    filesystem& fs_; identity expected_; std::function<identity()> current_;
};
}
bool baseline_proof::matches(const identity& current, const std::string& task,
    const std::filesystem::path& source, const std::filesystem::path& backup) const {
    return current == identity_ && task == task_ && normalized_root(source) == source_ && normalized_root(backup) == backup_;
}
baseline_proof verify_directory_baseline(const std::filesystem::path& source, const std::filesystem::path& backup,
    const std::string& task, const identity& expected, const std::function<identity()>& current) {
    if (!detail::key(task) || !detail::key(expected.device) || !expected.generation || !expected.capability_version || !current)
        throw failure("baseline_binding");
    const auto a = normalized_root(source), b = normalized_root(backup);
    if (nested(a, b) || nested(b, a)) throw failure("baseline_overlap");
    auto input = local_directory(source), external = local_directory(backup);
    baseline_reader guarded_source(*input, expected, current), guarded_backup(*external, expected, current);
    baseline_proof proof;
    proof.entries_ = verify_baseline_tree(guarded_source, guarded_backup, "iPod_Control");
    guarded_source.check();
    proof.identity_ = expected; proof.task_ = task; proof.source_ = a; proof.backup_ = b;
    return proof;
}
} // namespace foopodbridge::core::transaction

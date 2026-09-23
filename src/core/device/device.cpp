// SPDX-License-Identifier: LGPL-3.0-or-later
#include "foopodbridge/core/device/device.h"
#include "foopodbridge/core/database/reader.h"
#include "foopodbridge/core/database/validator.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <map>
#include <set>

namespace foopodbridge::core::device {
namespace {
constexpr std::string_view source = "FD-DEV-01: device_info.cpp; ipod_manager.cpp";
constexpr family_entry entries[] = {
    {"ipod1", "iPod 1G", family::traditional, 0, source},
    {"ipod2", "iPod 2G", family::traditional, 0, source},
    {"ipod3", "iPod 3G", family::traditional, 0x1201, source},
    {"ipod4", "iPod 4G", family::traditional, 0x1203, source},
    {"photo", "iPod Photo/Color", family::traditional, 0x1204, source},
    {"video5", "iPod Video 5G", family::traditional, 0x1209, source},
    {"video55", "iPod Video 5.5G", family::traditional, 0x1209, source},
    {"classic2007", "iPod Classic (2007)", family::signed_traditional, 0x1261, source},
    {"classic2008", "iPod Classic (2008)", family::signed_traditional, 0x1261, source},
    {"classic2009", "iPod Classic (2009)", family::signed_traditional, 0x1261, source},
    {"mini1", "iPod Mini 1G", family::traditional, 0x1205, source},
    {"mini2", "iPod Mini 2G", family::traditional, 0x1205, source},
    {"nano1", "iPod Nano 1G", family::traditional, 0x120a, source},
    {"nano2", "iPod Nano 2G", family::traditional, 0x1260, source},
    {"nano3", "iPod Nano 3G", family::signed_traditional, 0x1262, source},
    {"nano4", "iPod Nano 4G", family::signed_traditional, 0x1263, source},
    {"nano5", "iPod Nano 5G", family::nano_later, 0x1265, source},
    {"nano6", "iPod Nano 6G", family::nano_later, 0x1266, source},
    {"nano7", "iPod Nano 7G", family::nano_later, 0x1267, source},
    {"shuffle1", "iPod Shuffle 1G", family::shuffle, 0x1300, source},
    {"shuffle2", "iPod Shuffle 2G", family::shuffle, 0x1301, source},
    {"shuffle3", "iPod Shuffle 3G", family::shuffle, 0x1302, source},
    {"shuffle4", "iPod Shuffle 4G", family::shuffle, 0x1303, source},
};
std::string upper(std::string_view s) {
    std::string out(s);
    for (auto& c : out) if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    return out;
}
std::uint32_t u32(std::span<const std::byte> b, std::size_t o) {
    return std::to_integer<std::uint32_t>(b[o]) | (std::to_integer<std::uint32_t>(b[o + 1]) << 8U)
        | (std::to_integer<std::uint32_t>(b[o + 2]) << 16U) | (std::to_integer<std::uint32_t>(b[o + 3]) << 24U);
}
void media_types(const database::record_node& n, std::span<const std::byte> bytes, std::map<std::uint32_t, media_kind>& types) {
    if (n.marker == std::array<char, 4>{'m', 'h', 'i', 't'} && n.header_size >= 212) {
        const auto offset = static_cast<std::size_t>(n.offset);
        const auto value = u32(bytes, offset + 208);
        types[u32(bytes, offset + 16)] = value == 1 ? media_kind::music : value == 8 ? media_kind::audiobook
            : value == 0 ? media_kind::unknown : media_kind::other;
    }
    for (const auto& child : n.children) media_types(child, bytes, types);
}
}
std::span<const family_entry> registry() noexcept { return entries; }
identification identify(std::string_view hardware_id, std::string_view serial_suffix) {
    identification out;
    const auto id = upper(hardware_id);
    if (id.starts_with("1394\\APPLE_COMPUTER__INC.&IPOD")) {
        out.positive = true; out.group = family::traditional; out.name = "iPod (FireWire; generation unknown)";
        return out;
    }
    constexpr std::string_view prefix = "USB\\VID_05AC&PID_";
    if (!id.starts_with(prefix) || id.size() < prefix.size() + 4) return out;
    const auto end = prefix.size() + 4;
    if (id.size() > end && id[end] != '&' && id[end] != '\\') return out;
    unsigned product{};
    auto parsed = std::from_chars(id.data() + prefix.size(), id.data() + end, product, 16);
    if (parsed.ec != std::errc{} || parsed.ptr != id.data() + end) return out;
    if (product >= 0x1290 && product <= 0x12ff) { out.excluded = true; return out; }
    std::vector<const family_entry*> matches;
    for (const auto& e : entries) if (e.usb_product == product && product != 0) matches.push_back(&e);
    if (matches.empty()) return out;
    out.positive = true; out.group = matches.front()->group;
    if (matches.size() == 1) { out.name = matches.front()->name; out.model_id = matches.front()->id; return out; }
    const auto suffix = upper(serial_suffix);
    std::string_view refined;
    if (product == 0x1261) {
        out.name = "iPod Classic (generation unknown)";
        if (suffix == "YMV" || suffix == "YMX" || suffix == "Y5N" || suffix == "YMU") refined = "classic2007";
        if (suffix == "2C7" || suffix == "2C5") refined = "classic2008";
        if (suffix == "9ZU" || suffix == "9ZS") refined = "classic2009";
    } else if (product == 0x1205) out.name = "iPod Mini (1G/2G)";
    else out.name = "iPod Video (5G/5.5G)";
    for (const auto* e : matches) if (e->id == refined) { out.name = e->name; out.model_id = e->id; }
    return out;
}
std::optional<std::string> safe_relative_path(std::string_view input) {
    if (input.empty() || input.size() > 2048) return {};
    std::string path(input);
    if (path.front() == ':') path.erase(0, 1);
    else if (path.find(':') != std::string::npos) return {};
    std::replace(path.begin(), path.end(), ':', '/');
    std::replace(path.begin(), path.end(), '\\', '/');
    if (!upper(path).starts_with("IPOD_CONTROL/MUSIC/")) return {};
    // Reject alternate streams (a colon suffix cannot be an additional path component).
    const auto original_colon = input.rfind(':');
    if (original_colon != std::string_view::npos && input.substr(0, original_colon).find('.') != std::string_view::npos) return {};
    for (std::size_t begin = 0; begin < path.size();) {
        const auto end = path.find('/', begin);
        const auto part = path.substr(begin, end == std::string::npos ? path.size() - begin : end - begin);
        if (part.empty() || part == "." || part == ".." || part.back() == ' ' || part.back() == '.') return {};
        for (const auto ch : part) if (static_cast<unsigned char>(ch) < 32 || std::string_view("<>\"|?*").find(ch) != std::string_view::npos) return {};
        const auto base = upper(part.substr(0, part.find('.')));
        if (base == "CON" || base == "PRN" || base == "AUX" || base == "NUL" ||
            (base.size() == 4 && (base.starts_with("COM") || base.starts_with("LPT")) && base[3] >= '1' && base[3] <= '9')) return {};
        if (end == std::string::npos) break;
        begin = end + 1;
        if (begin == path.size()) return {};
    }
    return path;
}
const char* state_name(state s) noexcept {
    switch (s) {
    case state::not_mounted: return "Not mounted";
    case state::unidentified: return "Unidentified volume";
    case state::unsupported_filesystem: return "Unsupported file system";
    case state::format_pending: return "Format pending";
    case state::initializable: return "Initializable (read-only)";
    case state::ready_read_only: return "Ready - read-only";
    case state::database_corrupt: return "Database corrupt";
    case state::recovery_required: return "Recovery required";
    default: return "Read error";
    }
}
const char* reason_text(reason r) noexcept {
    switch (r) {
    case reason::none: return "No error";
    case reason::no_volume: return "Windows has not exposed an accessible storage volume.";
    case reason::identity_unproven: return "The volume cannot be positively identified as a supported iPod.";
    case reason::identity_conflict: return "Device identity or volume mapping is ambiguous.";
    case reason::filesystem: return "The file system is not supported by this reader.";
    case reason::format_unimplemented: return "This database format/profile is not implemented yet.";
    case reason::database_missing: return "The main database is missing. Initialization is not enabled.";
    case reason::database_invalid: return "Database structure or references are invalid. No repair was attempted.";
    case reason::signature_invalid: return "The database signature does not match the device identity.";
    case reason::access_denied: return "Windows denied read access.";
    case reason::sharing_violation: return "Another application is using the database. Refresh after it closes.";
    case reason::removed: return "The device was removed.";
    case reason::content_changed: return "The device or database changed during reading. Refresh to retry.";
    case reason::resource_limit: return "The input exceeds the reader's resource limit.";
    case reason::unsafe_path: return "A device path leaves the allowed directory or uses a reparse point.";
    case reason::recovery_record: return "Pending or invalid transaction records require recovery review.";
    case reason::cancelled: return "Reading was cancelled.";
    case reason::enumeration_failed: return "Windows device enumeration failed. Refresh to retry.";
    default: return "A device read failed. No files were changed.";
    }
}
snapshot inspect(const candidate& c, const file_result& f) {
    snapshot s;
    s.identity = identify(c.hardware_id, c.serial_suffix);
    s.identity_complete = c.identity_complete;
    s.capacity_known = c.capacity_known; s.capacity = c.capacity; s.available = c.available;
    s.artwork_present = c.artwork_present;
    const auto fail = [&](state status, reason why) { s.status = status; s.problem = why; return s; };
    if (!s.identity.positive) return fail(state::unidentified, reason::identity_unproven);
    s.level = evidence::structure_known;
    if (!c.mapping_valid) return fail(state::unidentified, reason::identity_conflict);
    if (!c.mounted) return fail(state::not_mounted, reason::no_volume);
    if (c.problem != reason::none) return fail(state::read_error, c.problem);
    if (!c.filesystem_supported) return fail(state::unsupported_filesystem, reason::filesystem);
    s.recovery_pending = c.recovery_pending;
    s.recovery_invalid = c.recovery_invalid;
    if (c.recovery_verified || c.recovery_pending || c.recovery_invalid) return fail(state::recovery_required, reason::recovery_record);
    if (s.identity.group == family::shuffle || s.identity.group == family::nano_later || c.alternative_database)
        return fail(state::format_pending, reason::format_unimplemented);
    if (f.problem == reason::database_missing)
        return fail(c.initialization_authorized && c.identity_complete && s.identity.group == family::signed_traditional
            && database::parse_hash58_device_key(c.signing_identity) ? state::initializable : state::format_pending, f.problem);
    if (f.problem != reason::none) return fail(state::read_error, f.problem);
    // Unknown container/version is not a corrupt instance of an implemented format.
    if (f.bytes.size() >= 20) {
        if (f.bytes[0] != std::byte{'m'} || f.bytes[1] != std::byte{'h'} || f.bytes[2] != std::byte{'b'} || f.bytes[3] != std::byte{'d'})
            return fail(state::format_pending, reason::format_unimplemented);
        const auto version = u32(f.bytes, 16);
        if (version < 9 || version > 115) return fail(state::format_pending, reason::format_unimplemented);
    }
    const bool signed_family = s.identity.group == family::signed_traditional;
    const auto profile = signed_family ? database::traditional_hash58_profile() : database::traditional_preserve_only_profile();
    s.profile = signed_family ? "TraditionalHash58" : "TraditionalPreserveOnly";
    auto parsed = database::reader{}.read(f.bytes, profile);
    if (!parsed) {
        if (parsed.error().code == database::error_code::resource_limit_exceeded) return fail(state::read_error, reason::resource_limit);
        if (parsed.error().code == database::error_code::unsupported_signed_profile || parsed.error().code == database::error_code::profile_mismatch || parsed.error().code == database::error_code::unsupported_encoding)
            return fail(state::format_pending, reason::format_unimplemented);
        return fail(state::database_corrupt, reason::database_invalid);
    }
    s.signature = parsed.value().hash58_status;
    if (signed_family && !c.signing_identity.empty()) {
        const auto key = database::parse_hash58_device_key(c.signing_identity);
        if (key) {
            auto verified = database::reader{}.read(f.bytes, profile, key.value());
            if (!verified || verified.value().hash58_status != database::hash58_signature_status::valid || !database::validator{}.validate(verified.value())) {
                s.signature = database::hash58_signature_status::invalid;
                return fail(state::database_corrupt, reason::signature_invalid);
            }
            s.signature = database::hash58_signature_status::valid;
        }
    }
    if (!database::validator{}.validate(parsed.value())) return fail(state::database_corrupt, reason::database_invalid);
    std::map<std::uint32_t, media_kind> types;
    media_types(parsed.value().root, f.bytes, types);
    for (const auto& t : parsed.value().model.tracks) {
        const auto path = safe_relative_path(t.location);
        // Unsafe references remain visible as metadata, but cannot resolve to playable paths.
        s.tracks.push_back({t, types[t.id], path.value_or("")});
    }
    s.master = std::move(parsed.value().model.master_playlist);
    s.playlists = std::move(parsed.value().model.playlists);
    s.status = state::ready_read_only; s.problem = reason::none;
    return s;
}

discovery::discovery(std::unique_ptr<read_backend> backend, std::filesystem::path repository)
    : backend_(std::move(backend)), repository_(std::move(repository)) {}
discovery::~discovery() { stop(); }
void discovery::start(std::function<void()> changed) {
    { std::lock_guard lock(mutex_); changed_ = std::move(changed); catalog_.stopped = false; }
    backend_->watch([this] { request(true); });
    worker_ = std::jthread([this](std::stop_token token) { run(token); });
    request(true);
}
void discovery::request(bool topology) {
    std::function<void()> callback;
    {
        std::lock_guard lock(mutex_);
        if (catalog_.stopped) return;
        scan_stop_.request_stop();
        ++epoch_; if (topology) ++mount_epoch_;
        catalog_.revision = epoch_; catalog_.scanning = true; catalog_.problem = reason::none;
        catalog_.devices.clear(); pending_ = true; callback = changed_;
    }
    wake_.notify_all();
    if (callback) callback();
}
void discovery::refresh() { request(false); }
catalog discovery::current() const { std::lock_guard lock(mutex_); return catalog_; }
bool discovery::is_current(std::string_view token, std::uint64_t generation, std::uint64_t revision) const {
    std::lock_guard lock(mutex_);
    if (catalog_.scanning || catalog_.stopped || catalog_.revision != revision) return false;
    return std::any_of(catalog_.devices.begin(), catalog_.devices.end(), [&](const auto& p) {
        return p->token == token && p->generation == generation && p->revision == revision;
    });
}
void discovery::stop() noexcept {
    {
        std::lock_guard lock(mutex_);
        if (catalog_.stopped) return;
        catalog_.stopped = true; catalog_.scanning = false; catalog_.devices.clear();
        scan_stop_.request_stop(); changed_ = {};
    }
    backend_->unwatch();
    worker_.request_stop(); wake_.notify_all();
    if (worker_.joinable()) worker_.join();
}
void discovery::run(std::stop_token stop) {
    std::map<std::string, std::string> tokens;
    std::uint64_t next_token{};
    std::uint64_t token_mount{};
    while (!stop.stop_requested()) {
        std::uint64_t epoch{}, mount{};
        std::stop_token scan;
        {
            std::unique_lock lock(mutex_);
            if (!wake_.wait(lock, stop, [this] { return pending_ || catalog_.stopped; }) || catalog_.stopped) break;
            epoch = epoch_; mount = mount_epoch_; pending_ = false;
            scan_stop_ = std::stop_source{}; scan = scan_stop_.get_token();
        }
        catalog result; result.revision = epoch;
        if (token_mount != mount) { tokens.clear(); token_mount = mount; }
        try {
            auto candidates = backend_->enumerate(scan);
            if (candidates.size() > 64) throw std::runtime_error("device candidate limit exceeded");
            std::set<std::pair<std::string, std::string>> seen;
            std::map<std::string, std::set<std::string>> physical_volumes, volume_owners, hardware_evidence, recovery_owners;
            std::set<std::string> invalid_mappings;
            for (const auto& c : candidates) {
                const auto recovery_key = recovery_repository_key(c);
                if (!recovery_key.empty()) recovery_owners[recovery_key].insert(c.physical_key);
                hardware_evidence[c.physical_key].insert(upper(c.hardware_id));
                if (!c.mapping_valid) invalid_mappings.insert(c.physical_key);
                if (!c.mounted) continue;
                physical_volumes[c.physical_key].insert(c.volume_key);
                volume_owners[c.volume_key].insert(c.physical_key);
            }
            for (auto& c : candidates) {
                if (scan.stop_requested()) break;
                if (!seen.insert({c.physical_key, c.volume_key}).second) continue;
                if (identify(c.hardware_id).excluded) continue;
                if (c.physical_key.empty() || invalid_mappings.contains(c.physical_key)
                    || hardware_evidence[c.physical_key].size() > 1
                    || recovery_owners[recovery_repository_key(c)].size() > 1
                    || (c.mounted && (c.volume_key.empty() || physical_volumes[c.physical_key].size() > 1
                        || volume_owners[c.volume_key].size() > 1))) {
                    c.mapping_valid = false; c.identity_complete = false;
                }
                auto s = inspect(c, {{}, reason::database_missing});
                // A single device's I/O failure must not erase healthy devices.
                // Untrusted mappings and unsupported inputs never reach database I/O.
                if (s.problem == reason::database_missing) {
                    try {
                        auto bytes = backend_->read_database(c, scan);
                        s = inspect(c, bytes);
                        if (!backend_->still_present(c)) s = inspect(c, {{}, reason::removed});
                    } catch (...) {
                        s = inspect(c, {{}, reason::io_failure});
                    }
                }
                s.recovery = inspect_repository(c, repository_, scan);
                if (s.recovery.pending || s.recovery.invalid) {
                    s.status = state::recovery_required; s.problem = reason::recovery_record;
                }
                if (!backend_->still_present(c)) { s = inspect(c, {{}, reason::removed}); }
                if (scan.stop_requested()) break;
                const auto key = c.physical_key + '\n' + c.volume_key;
                auto& token = tokens[key];
                if (token.empty()) token = "device-" + std::to_string(++next_token);
                s.token = token; s.generation = mount; s.revision = epoch;
                result.devices.push_back(std::make_shared<const snapshot>(std::move(s)));
            }
        } catch (...) { result.devices.clear(); result.problem = reason::enumeration_failed; }
        std::function<void()> callback;
        {
            std::lock_guard lock(mutex_);
            if (catalog_.stopped || epoch != epoch_ || scan.stop_requested()) continue;
            catalog_ = std::move(result); callback = changed_;
        }
        if (callback) callback();
    }
}
} // namespace foopodbridge::core::device

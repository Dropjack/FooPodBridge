// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "foopodbridge/core/database/model.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

namespace foopodbridge::core::device {

enum class family : std::uint32_t { unknown, traditional, signed_traditional, shuffle, nano_later };
enum class state : std::uint32_t {
    not_mounted, unidentified, unsupported_filesystem, format_pending, initializable,
    ready_read_only, database_corrupt, recovery_required, read_error
};
enum class reason : std::uint32_t {
    none, no_volume, identity_unproven, identity_conflict, filesystem,
    format_unimplemented, database_missing, database_invalid, signature_invalid,
    access_denied, sharing_violation, io_failure, removed, content_changed,
    resource_limit, unsafe_path, recovery_record, cancelled, enumeration_failed
};
enum class evidence : std::uint32_t { unknown, structure_known, fixture_round_trip, device_read_verified, device_write_verified };
enum class media_kind : std::uint32_t { unknown, music, audiobook, other };

struct family_entry {
    std::string_view id;
    std::string_view name;
    family group;
    std::uint16_t usb_product;
    std::string_view source;
};
std::span<const family_entry> registry() noexcept;
struct identification {
    bool positive{};
    bool excluded{};
    bool conflict{};
    family group{};
    std::string name{"Unidentified volume"};
    std::string model_id;
};
identification identify(std::string_view hardware_id, std::string_view serial_suffix = {});
std::optional<std::string> safe_relative_path(std::string_view database_path);
const char* state_name(state value) noexcept;
const char* reason_text(reason value) noexcept;

// All identifiers and paths here are internal. The service exports opaque tokens.
struct candidate {
    std::string physical_key;
    std::string volume_key;
    std::string hardware_id;
    std::string serial_suffix;
    std::string signing_identity;
    bool identity_complete{};
    bool mapping_valid{true};
    bool mounted{true};
    bool filesystem_supported{true};
    bool capacity_known{};
    bool alternative_database{};
    bool artwork_present{};
    bool recovery_verified{};
    std::uint32_t recovery_pending{};
    std::uint32_t recovery_invalid{};
    bool initialization_authorized{};
    std::uint64_t capacity{};
    std::uint64_t available{};
    reason problem{reason::none};
};
struct file_result {
    std::vector<std::byte> bytes;
    reason problem{reason::none};
};
struct library_track {
    database::track metadata;
    media_kind kind{};
    std::string relative_path;
};
enum class recovery_link { identity_unavailable, repository_missing, available, unavailable };
struct recovery_summary {
    recovery_link link{recovery_link::identity_unavailable};
    std::uint32_t pending{}, invalid{}, snapshots{}, last_known_good{}, backups{};
};
// Private stable key; never exported through the service.
std::string recovery_repository_key(const candidate& input);
recovery_summary inspect_repository(const candidate& input, const std::filesystem::path& repository, std::stop_token stop);

struct snapshot {
    recovery_summary recovery;
    std::string token;
    std::uint64_t generation{};
    std::uint64_t revision{};
    identification identity;
    state status{state::unidentified};
    reason problem{reason::identity_unproven};
    evidence level{evidence::unknown};
    bool identity_complete{};
    bool capacity_known{};
    bool artwork_present{};
    std::uint32_t recovery_pending{};
    std::uint32_t recovery_invalid{};
    std::uint64_t capacity{};
    std::uint64_t available{};
    database::hash58_signature_status signature{database::hash58_signature_status::not_applicable};
    std::string profile;
    std::vector<library_track> tracks;
    std::vector<database::playlist> playlists;
    std::optional<database::playlist> master;
};
snapshot inspect(const candidate& input, const file_result& file);

// The production implementation exposes queries only; tests supply explicit simulations.
class read_backend {
public:
    virtual ~read_backend() = default;
    virtual std::vector<candidate> enumerate(std::stop_token cancel) = 0;
    virtual file_result read_database(const candidate& target, std::stop_token cancel) = 0;
    virtual bool still_present(const candidate& target) = 0;
    virtual void watch(std::function<void()> changed) = 0;
    virtual void unwatch() noexcept = 0;
};
struct catalog {
    std::uint64_t revision{};
    bool scanning{};
    bool stopped{};
    reason problem{reason::none};
    std::vector<std::shared_ptr<const snapshot>> devices;
};
class discovery final {
public:
    explicit discovery(std::unique_ptr<read_backend> backend, std::filesystem::path repository = {});
    ~discovery();
    void start(std::function<void()> changed);
    void refresh();
    void stop() noexcept;
    catalog current() const;
    bool is_current(std::string_view token, std::uint64_t generation, std::uint64_t revision) const;
private:
    void request(bool topology_changed);
    void run(std::stop_token stop);
    std::unique_ptr<read_backend> backend_;
    std::filesystem::path repository_;
    mutable std::mutex mutex_;
    std::condition_variable_any wake_;
    std::jthread worker_;
    std::stop_source scan_stop_;
    std::function<void()> changed_;
    catalog catalog_;
    bool pending_{};
    std::uint64_t epoch_{};
    std::uint64_t mount_epoch_{};
};
std::unique_ptr<read_backend> make_windows_backend();

} // namespace foopodbridge::core::device

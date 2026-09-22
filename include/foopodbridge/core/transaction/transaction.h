#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace foopodbridge::core::transaction {

using bytes = std::vector<std::byte>;
class failure : public std::runtime_error {
public:
    explicit failure(const char* code) : std::runtime_error(code) {}
    explicit failure(const std::string& code) : std::runtime_error(code) {}
};

// All paths are relative to an already acquired root. Implementations must reject
// aliases, reparse points, traversal, replacement and cross-root operations.
class filesystem {
public:
    virtual ~filesystem() = default;
    virtual std::optional<std::uint64_t> size(const std::string& path) = 0;
    virtual std::size_t read(const std::string& path, std::uint64_t offset, std::span<std::byte> out) = 0;
    virtual void create(const std::string& path) = 0;
    virtual void append(const std::string& path, std::span<const std::byte> data) = 0;
    virtual void flush(const std::string& path) = 0;
    virtual void rename(const std::string& from, const std::string& to) = 0;
    virtual void remove(const std::string& path) = 0;
    virtual std::uint64_t available() = 0;
    virtual std::vector<std::string> list(const std::string& directory) = 0;
};

// Only an existing, non-root local directory is accepted. No discovery or volume
// writer is exposed by this adapter; tests create their own directory first.
std::unique_ptr<filesystem> local_directory(const std::filesystem::path& root);
bool safe_path(const std::string& path);
std::string sha256(std::span<const std::byte> data);
std::string fingerprint(filesystem& fs, const std::string& path);
bytes read_all(filesystem& fs, const std::string& path, std::uint64_t limit = 512ULL * 1024 * 1024);

struct identity {
    std::string device;
    std::uint64_t generation{};
    std::uint64_t capability_version{};
    bool writable{};
    bool operator==(const identity&) const = default;
};
enum class operation { update, initialize_library };
enum class phase { preparing, copying, staging_database, committing, cleaning };
enum class outcome { completed, cancelled, failed, busy, recovery_required, cleaning_required };
struct result {
    outcome status{outcome::failed};
    bool committed{};
    bool cancel_deferred{};
    std::string code;
};
struct media_file {
    std::string source;
    std::string target;
    std::uint64_t size{};
    std::string digest;
};
struct deletion {
    std::string target;
    std::string digest;
};
// Validator returns every referenced device-relative media path and throws on
// structural/profile/signature errors. It must use the independent DB Reader.
using database_validator = std::function<std::vector<std::string>(std::span<const std::byte>)>;
struct request {
    std::string id;
    identity device;
    operation kind{operation::update};
    std::string database_path;
    bytes new_database;
    std::vector<media_file> additions;
    std::vector<deletion> deletions;
    std::uint64_t reserve_bytes{256ULL * 1024 * 1024};
};
class plan final {
public:
    const request& details() const noexcept { return request_; }
private:
    request request_;
    bytes old_database_;
    std::string old_digest_;
    std::vector<std::string> new_references_;
    friend class engine;
};
struct callbacks {
    std::function<identity()> current_identity;
    std::function<bool()> cancelled = [] { return false; };
    std::function<void(phase)> progress = [](phase) {};
};
class engine final {
public:
    engine(filesystem& device, filesystem& host, filesystem& sources,
           database_validator validator, callbacks hooks);
    plan prepare(request input);
    result execute(const plan& input);
    result recover(const std::string& device_key, const std::string& operation_id);
private:
    filesystem& device_;
    filesystem& host_;
    filesystem& sources_;
    database_validator validate_;
    callbacks hooks_;
};

struct baseline_entry { std::string path; std::uint64_t size{}; std::string digest; };
// Caller enumerates the complete iPod_Control tree under a stable read session.
// The list is explicit to avoid silently excluding inaccessible entries.
std::vector<baseline_entry> verify_baseline(filesystem& source, filesystem& backup,
                                          const std::vector<std::string>& complete_paths);
struct snapshot {
    std::string id;
    std::uint64_t sequence{};
    bool validated{};
    bool last_known_good{};
    bool active_recovery{};
};
std::vector<std::string> retention_candidates(const std::vector<snapshot>& snapshots);

class snapshot_store final {
public:
    snapshot_store(filesystem& host, std::string device_key, database_validator validate);
    void save(const std::string& id, std::uint64_t sequence, std::span<const std::byte> database,
              bool active_recovery = false);
    void mark_last_known_good(const std::string& id);
    void release_recovery(const std::string& id);
    std::vector<snapshot> inspect();
    void prune();
private:
    filesystem& host_;
    std::string prefix_;
    database_validator validate_;
};

} // namespace foopodbridge::core::transaction

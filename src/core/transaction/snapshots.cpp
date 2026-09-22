#include "internal.h"
#include <algorithm>
#include <sstream>
#include <set>

namespace foopodbridge::core::transaction {
snapshot_store::snapshot_store(filesystem& host, std::string device_key, database_validator validate)
    : host_(host), prefix_("snapshots/" + device_key + "/"), validate_(std::move(validate)) {
    if (!detail::key(device_key) || !validate_) throw failure("snapshot_arguments");
}
void snapshot_store::save(const std::string& id, std::uint64_t sequence, std::span<const std::byte> database, bool active) {
    if (!detail::key(id)) throw failure("snapshot_id");
    validate_(database);
    const auto entries = inspect();
    if (std::any_of(entries.begin(), entries.end(), [&](const auto& entry) { return entry.sequence == sequence; })) throw failure("snapshot_sequence");
    const auto base = prefix_ + id;
    if (host_.size(base + ".db") || host_.size(base + ".meta")) throw failure("snapshot_exists");
    detail::write(host_, base + ".db", database);
    // Metadata is last; an interrupted save is not a validated snapshot.
    const auto body = "FPBSNAPSHOT1\n" + std::to_string(sequence) + "\n" + sha256(database) + "\n" + (active ? "1\n" : "0\n");
    detail::write(host_, base + ".meta", detail::encode(detail::seal(body)));
}
std::vector<snapshot> snapshot_store::inspect() {
    std::vector<snapshot> entries;
    for (const auto& name : host_.list(prefix_.substr(0, prefix_.size() - 1))) {
        if (!name.ends_with(".meta")) continue;
        snapshot entry; entry.id = name.substr(0, name.size() - 5);
        if (!detail::key(entry.id)) throw failure("snapshot_catalog_invalid");
        try {
            const auto base = prefix_ + entry.id;
            std::istringstream in(detail::unseal(read_all(host_, base + ".meta", 1024)));
            std::string magic, digest; int active{};
            in >> magic >> entry.sequence >> digest >> active;
            if (!in || magic != "FPBSNAPSHOT1" || !detail::digest(digest) || active < 0 || active > 1) throw failure("snapshot_metadata");
            in >> std::ws;
            if (!in.eof()) throw failure("snapshot_metadata");
            const auto data = read_all(host_, base + ".db");
            if (sha256(data) != digest) throw failure("snapshot_hash");
            validate_(data);
            entry.validated = true;
            const auto marker = detail::encode(detail::seal(digest + "\n"));
            entry.last_known_good = host_.size(base + ".lkg") && read_all(host_, base + ".lkg", 1024) == marker;
            entry.active_recovery = active != 0 && !(host_.size(base + ".released") && read_all(host_, base + ".released", 1024) == marker);
        } catch (const failure&) {
            // Retain corrupted evidence. It neither counts towards ten valid
            // snapshots nor becomes a deletion candidate.
            entry.validated = false;
        }
        entries.push_back(std::move(entry));
    }
    std::set<std::uint64_t> sequences;
    for (const auto& entry : entries) if (entry.validated && !sequences.insert(entry.sequence).second) throw failure("snapshot_sequence");
    return entries;
}
void snapshot_store::mark_last_known_good(const std::string& id) {
    const auto entries = inspect();
    const auto found = std::find_if(entries.begin(), entries.end(), [&](const auto& s) { return s.id == id && s.validated; });
    if (found == entries.end()) throw failure("invalid_lkg");
    const auto base = prefix_ + id;
    const auto data = detail::encode(detail::seal(fingerprint(host_, base + ".db") + "\n"));
    if (!host_.size(base + ".lkg")) detail::write(host_, base + ".lkg", data);
    else if (read_all(host_, base + ".lkg") != data) throw failure("lkg_invalid");
}
void snapshot_store::release_recovery(const std::string& id) {
    const auto entries = inspect();
    const auto found = std::find_if(entries.begin(), entries.end(), [&](const auto& s) { return s.id == id && s.validated; });
    if (found == entries.end()) throw failure("snapshot_invalid");
    const auto base = prefix_ + id;
    const auto data = detail::encode(detail::seal(fingerprint(host_, base + ".db") + "\n"));
    if (!host_.size(base + ".released")) detail::write(host_, base + ".released", data);
    else if (read_all(host_, base + ".released") != data) throw failure("release_marker_invalid");
}
void snapshot_store::prune() {
    for (const auto& id : retention_candidates(inspect())) {
        const auto base = prefix_ + id;
        // Remove publication first. A crash leaves data, never a valid catalog
        // entry pointing to an absent DB or a deleted protected restore point.
        host_.remove(base + ".meta");
        host_.remove(base + ".db");
        if (host_.size(base + ".released")) host_.remove(base + ".released");
    }
}
}

#include "internal.h"
#include <algorithm>
#include <iomanip>
#include <mutex>
#include <set>
#include <sstream>

namespace foopodbridge::core::transaction {
namespace {
std::mutex write_mutex;
constexpr std::uint64_t max_file = 0xffffffffULL;
std::string folded(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}
bool contains(const std::vector<std::string>& paths, const std::string& path) {
    return std::any_of(paths.begin(), paths.end(), [&](const auto& item) { return folded(item) == folded(path); });
}
// A new identity check is made before EVERY filesystem call, including rollback.
class guarded final : public filesystem {
public:
    guarded(filesystem& fs, callbacks& hooks, identity expected) : fs_(fs), hooks_(hooks), expected_(std::move(expected)) {}
    void check() { if (!(hooks_.current_identity() == expected_) || !expected_.writable) throw failure("identity_changed"); }
    std::optional<std::uint64_t> size(const std::string& p) override { check(); return fs_.size(p); }
    std::size_t read(const std::string& p, std::uint64_t offset, std::span<std::byte> b) override { check(); return fs_.read(p, offset, b); }
    void create(const std::string& p) override { check(); fs_.create(p); }
    void append(const std::string& p, std::span<const std::byte> b) override { check(); fs_.append(p, b); }
    void flush(const std::string& p) override { check(); fs_.flush(p); }
    void rename(const std::string& a, const std::string& b) override { check(); fs_.rename(a, b); }
    void remove(const std::string& p) override { check(); fs_.remove(p); }
    std::uint64_t available() override { check(); return fs_.available(); }
    std::vector<std::string> list(const std::string& p) override { check(); return fs_.list(p); }
private:
    filesystem& fs_; callbacks& hooks_; identity expected_;
};
struct record {
    request input;
    std::string old_hash;
    std::string new_hash;
};
std::string prefix(const record& r) { return "transactions/" + r.input.device.device + "/" + r.input.id + "/"; }
std::string marker(const record& r) { return ".foopodbridge/" + r.input.id + ".journal"; }
std::string temp(const record& r, const std::string& path) { return path + ".fpb-" + r.input.id + ".tmp"; }
std::string old_path(const record& r) { return r.input.database_path + ".fpb-" + r.input.id + ".old"; }
std::string serialize(const record& r) {
    std::ostringstream out;
    out << "FPBTXN1\n" << r.input.id << '\n' << r.input.device.device << '\n'
        << r.input.device.generation << ' ' << r.input.device.capability_version << ' '
        << static_cast<int>(r.input.kind) << '\n' << std::quoted(r.input.database_path) << '\n'
        << (r.old_hash.empty() ? "-" : r.old_hash) << '\n' << r.new_hash << '\n'
        << r.input.additions.size() << '\n';
    for (const auto& file : r.input.additions) out << std::quoted(file.target) << ' ' << file.size << ' ' << file.digest << '\n';
    out << r.input.deletions.size() << '\n';
    for (const auto& file : r.input.deletions) out << std::quoted(file.target) << ' ' << file.digest << '\n';
    return detail::seal(out.str());
}
void paths_valid(const request& input) {
    if (!detail::key(input.id) || input.id.size() > 64 || !detail::key(input.device.device) || !safe_path(input.database_path) ||
        input.database_path.size() + input.id.size() + 10 > 512 || !input.reserve_bytes ||
        input.database_path.rfind("iPod_Control/iTunes/", 0) != 0 ||
        (input.kind != operation::update && input.kind != operation::initialize_library)) throw failure("invalid_plan");
    std::set<std::string> paths;
    const auto add = [&](const std::string& path) {
        if (!safe_path(path) || !paths.insert(folded(path)).second) throw failure("path_collision");
    };
    add(input.database_path); add(input.database_path + ".fpb-" + input.id + ".tmp");
    add(input.database_path + ".fpb-" + input.id + ".old");
    for (const auto& file : input.additions) {
        if (file.target.rfind("iPod_Control/Music/", 0) != 0 || !file.size || file.size > max_file || !detail::digest(file.digest)) throw failure("invalid_media");
        add(file.target); add(file.target + ".fpb-" + input.id + ".tmp");
    }
    for (const auto& file : input.deletions) {
        if (file.target.rfind("iPod_Control/Music/", 0) != 0 || !detail::digest(file.digest)) throw failure("invalid_deletion");
        add(file.target);
    }
    if (input.kind == operation::initialize_library && !input.deletions.empty()) throw failure("initialization_deletion");
}
record parse(const bytes& data) {
    std::istringstream in(detail::unseal(data));
    record r; std::string magic; int kind{}; std::size_t count{};
    in >> magic >> r.input.id >> r.input.device.device >> r.input.device.generation >> r.input.device.capability_version >> kind
       >> std::quoted(r.input.database_path) >> r.old_hash >> r.new_hash >> count;
    if (!in || magic != "FPBTXN1" || kind < 0 || kind > 1 || count > 100000) throw failure("record_invalid");
    r.input.kind = static_cast<operation>(kind);
    if (r.old_hash == "-") r.old_hash.clear();
    if ((!r.old_hash.empty() && !detail::digest(r.old_hash)) || !detail::digest(r.new_hash) ||
        ((r.input.kind == operation::initialize_library) != r.old_hash.empty())) throw failure("record_invalid");
    for (std::size_t i = 0; i < count; ++i) { media_file f; in >> std::quoted(f.target) >> f.size >> f.digest; r.input.additions.push_back(std::move(f)); }
    in >> count;
    if (!in || count > 100000) throw failure("record_invalid");
    for (std::size_t i = 0; i < count; ++i) { deletion f; in >> std::quoted(f.target) >> f.digest; r.input.deletions.push_back(std::move(f)); }
    in >> std::ws;
    if (!in.eof()) throw failure("record_invalid");
    paths_valid(r.input);
    return r;
}
bool matches(filesystem& fs, const std::string& path, const std::string& expected) {
    return !expected.empty() && fs.size(path).has_value() && fingerprint(fs, path) == expected;
}
std::vector<std::string> checked_db(filesystem& fs, const std::string& path, const std::string& digest,
                                    const database_validator& validate) {
    const auto db = read_all(fs, path);
    if (sha256(db) != digest) throw failure("database_changed");
    auto refs = validate(db);
    for (const auto& ref : refs) if (!safe_path(ref)) throw failure("unsafe_reference");
    return refs;
}
void references_exist(filesystem& device, const std::vector<std::string>& refs) {
    for (const auto& ref : refs) { const auto n = device.size(ref); if (!n || !*n) throw failure("missing_reference"); }
}
void recovered_media_valid(filesystem& device, const record& r, const std::vector<std::string>& refs) {
    references_exist(device, refs);
    for (const auto& file : r.input.additions) {
        if (contains(refs, file.target) && (device.size(file.target) != file.size || !matches(device, file.target, file.digest)))
            throw failure("recovery_media_invalid");
    }
}
void mark_done(filesystem& device, const record& r, bool committed) {
    const auto path = marker(r) + ".done";
    const auto text = detail::encode(detail::seal((committed ? r.new_hash : r.old_hash) + "\n"));
    if (device.size(path)) {
        if (read_all(device, path) != text) throw failure("completion_record_invalid");
    } else detail::write(device, path, text);
}
bool cleanup(filesystem& device, const record& r, const std::vector<std::string>& refs, bool committed) {
    bool complete = true;
    const auto remove_owned = [&](const std::string& path, const std::string& digest) {
        if (!device.size(path)) return;
        if (contains(refs, path) || !matches(device, path, digest)) { complete = false; return; }
        device.remove(path);
    };
    for (const auto& file : r.input.additions) {
        remove_owned(temp(r, file.target), file.digest);
        if (!committed) remove_owned(file.target, file.digest);
    }
    if (committed) for (const auto& file : r.input.deletions) remove_owned(file.target, file.digest);
    remove_owned(temp(r, r.input.database_path), r.new_hash);
    // Device .old remains as recovery evidence. Host snapshot retention is separate.
    if (complete) mark_done(device, r, committed);
    return complete;
}
void save_snapshot(filesystem& host, const record& r, const database_validator& validate,
                   const std::string& suffix, std::span<const std::byte> data, bool active) {
    snapshot_store store(host, r.input.device.device, validate);
    const auto entries = store.inspect();
    const auto id = r.input.id + suffix;
    std::uint64_t sequence{};
    for (const auto& entry : entries) {
        if (entry.id == id) {
            if (!entry.validated || fingerprint(host, "snapshots/" + r.input.device.device + "/" + id + ".db") != sha256(data))
                throw failure("snapshot_conflict");
            return;
        }
        if (entry.sequence == UINT64_MAX) throw failure("snapshot_sequence_overflow");
        sequence = std::max(sequence, entry.sequence + 1);
    }
    store.save(id, sequence, data, active);
}
void finish_snapshots(filesystem& host, const record& r, const database_validator& validate, bool committed) {
    const auto finalized = prefix(r) + "finalized";
    const auto marker_data = detail::encode(detail::seal((committed ? r.new_hash : r.old_hash) + "\n"));
    if (host.size(finalized)) {
        if (read_all(host, finalized) != marker_data) throw failure("snapshot_finalization_invalid");
        return;
    }
    if (committed) {
        save_snapshot(host, r, validate, "-after", read_all(host, prefix(r) + "new.db"), false);
    }
    snapshot_store store(host, r.input.device.device, validate);
    if (!r.old_hash.empty()) store.release_recovery(r.input.id + "-before");
    store.prune();
    detail::write(host, finalized, marker_data);
}
void room(filesystem& device, const request& input, std::uint64_t old_size) {
    std::uint64_t required = input.reserve_bytes;
    const auto add = [&](std::uint64_t value) { if (required > UINT64_MAX - value) throw failure("space_overflow"); required += value; };
    add(input.new_database.size()); add(old_size); add(1024 * 1024);
    for (const auto& media : input.additions) add(media.size);
    if (device.available() < required) throw failure("space_insufficient");
}
result recover_record(filesystem& raw, filesystem& host, callbacks& hooks, const database_validator& validate,
                      const std::string& device_key, const std::string& operation_id) {
    if (!detail::key(device_key) || !detail::key(operation_id)) throw failure("invalid_recovery_key");
    const auto stored = read_all(host, "transactions/" + device_key + "/" + operation_id + "/manifest", 16 * 1024 * 1024);
    auto r = parse(stored);
    const auto identity = hooks.current_identity();
    if (r.input.device.device != device_key || r.input.id != operation_id || identity.device != device_key ||
        !identity.writable || identity.capability_version != r.input.device.capability_version) throw failure("recovery_identity");
    guarded device(raw, hooks, identity);
    if (device.size(marker(r)) && read_all(device, marker(r), 16 * 1024 * 1024) != stored) throw failure("device_record_invalid");
    const auto& db = r.input.database_path;
    if (device.size(marker(r) + ".done")) {
        const auto selected = detail::unseal(read_all(device, marker(r) + ".done", 1024));
        if (selected != r.new_hash + "\n" && selected != r.old_hash + "\n") throw failure("completion_record_invalid");
        const auto digest = selected.substr(0, selected.size() - 1);
        if (digest.empty()) {
            if (device.size(db)) throw failure("historical_transaction");
        } else {
            if (!matches(device, db, digest)) throw failure("historical_transaction");
            recovered_media_valid(device, r, checked_db(device, db, digest, validate));
        }
        finish_snapshots(host, r, validate, digest == r.new_hash);
        return {outcome::completed, digest == r.new_hash, false, "already_recovered"};
    }
    bool committed = false;
    std::vector<std::string> refs;
    if (device.size(db)) {
        const auto value = fingerprint(device, db);
        if (value == r.new_hash) { refs = checked_db(device, db, r.new_hash, validate); committed = true; }
        else if (!r.old_hash.empty() && value == r.old_hash) refs = checked_db(device, db, r.old_hash, validate);
        else throw failure("unknown_formal_database");
        recovered_media_valid(device, r, refs);
    } else if (!r.old_hash.empty()) {
        // Prefer the exact pre-transaction version. A newer decodable DB is not
        // sufficient evidence of a commit or of surviving media references.
        refs = checked_db(host, prefix(r) + "old.db", r.old_hash, validate);
        references_exist(device, refs);
        if (matches(device, old_path(r), r.old_hash)) device.rename(old_path(r), db);
        else {
            const auto restore = db + ".fpb-" + r.input.id + ".restore";
            if (device.size(restore)) {
                if (!matches(device, restore, r.old_hash)) throw failure("restore_conflict");
            } else detail::write(device, restore, read_all(host, prefix(r) + "old.db"));
            device.rename(restore, db);
        }
        checked_db(device, db, r.old_hash, validate);
    } else {
        const auto staged = temp(r, db);
        const auto commit_record = marker(r) + ".commit";
        if (device.size(staged) && device.size(commit_record)) {
            if (detail::unseal(read_all(device, commit_record, 1024)) != r.new_hash + "\n") throw failure("commit_record_invalid");
            refs = checked_db(device, staged, r.new_hash, validate);
            recovered_media_valid(device, r, refs);
            device.rename(staged, db);
            checked_db(device, db, r.new_hash, validate);
            committed = true;
        }
        // Without durable commit intent, keep the uninitialized state even if
        // a complete staged database exists (e.g. a pre-commit cancellation).
    }
    try {
        const bool clean = cleanup(device, r, refs, committed);
        if (clean) finish_snapshots(host, r, validate, committed);
        return {clean ? outcome::completed : outcome::cleaning_required, committed, false, clean ? "recovered" : "cleanup_pending"};
    } catch (const failure& e) {
        return {outcome::cleaning_required, committed, false, e.what()};
    }
}
}

engine::engine(filesystem& device, filesystem& host, filesystem& sources, database_validator validate, callbacks hooks)
    : device_(device), host_(host), sources_(sources), validate_(std::move(validate)), hooks_(std::move(hooks)) {
    if (!validate_ || !hooks_.current_identity) throw failure("missing_adapter");
}
journal_info inspect_journal(std::span<const std::byte> manifest, std::optional<std::span<const std::byte>> completion) {
    journal_info info;
    try {
        if (manifest.size() > 16 * 1024 * 1024 || (completion && completion->size() > 1024)) throw failure("record_limit");
        const auto r = parse(bytes(manifest.begin(), manifest.end()));
        info.operation_id = r.input.id; info.device_key = r.input.device.device;
        info.capability_version = r.input.device.capability_version;
        if (completion) {
            const auto selected = detail::unseal(bytes(completion->begin(), completion->end()));
            if (selected != r.old_hash + "\n" && selected != r.new_hash + "\n") throw failure("completion_record_invalid");
            info.status = journal_state::completed;
        } else info.status = journal_state::pending;
    } catch (const failure&) { info.status = journal_state::invalid; }
    return info;
}
std::vector<journal_info> discover_recovery(filesystem& device) {
    std::vector<journal_info> out;
    for (const auto& name : device.list(".foopodbridge")) {
        if (!name.ends_with(".journal")) continue;
        if (out.size() >= 10000) throw failure("recovery_catalog_limit");
        journal_info info;
        try {
            if (!safe_path(name) || name.find('/') != std::string::npos) throw failure("record_path");
            const auto path = ".foopodbridge/" + name;
            const auto manifest = read_all(device, path, 16 * 1024 * 1024);
            if (device.size(path + ".done")) {
                const auto done = read_all(device, path + ".done", 1024);
                info = inspect_journal(manifest, std::span<const std::byte>(done));
            } else info = inspect_journal(manifest);
            if (name != info.operation_id + ".journal") info.status = journal_state::invalid;
        } catch (const failure&) { info.status = journal_state::invalid; }
        out.push_back(std::move(info));
    }
    return out;
}
plan engine::prepare(request input) {
    paths_valid(input);
    guarded device(device_, hooks_, input.device);
    device.check();
    if (input.new_database.empty() || input.new_database.size() > 512 * 1024 * 1024) throw failure("database_limit");
    for (const auto& entry : device.list(".foopodbridge")) {
        if (!entry.ends_with(".journal")) continue;
        const auto path = ".foopodbridge/" + entry;
        if (!device.size(path + ".done")) throw failure("recovery_pending");
        const auto previous = parse(read_all(device, path, 16 * 1024 * 1024));
        const auto selected = detail::unseal(read_all(device, path + ".done", 1024));
        if (previous.input.device.device != input.device.device ||
            (selected != previous.old_hash + "\n" && selected != previous.new_hash + "\n")) throw failure("completion_record_invalid");
    }
    plan out; out.request_ = std::move(input);
    auto& r = out.request_;
    record rec{r, {}, sha256(r.new_database)};
    if (host_.size(prefix(rec) + "manifest") || device.size(marker(rec)) || device.size(old_path(rec)) ||
        device.size(temp(rec, r.database_path))) throw failure("operation_collision");
    if (r.kind == operation::initialize_library) {
        if (device.size(r.database_path)) throw failure("already_initialized");
        // A family is not empty just because its expected primary file is absent.
        for (const auto& name : {"iTunesDB", "iTunesCDB", "iTunesSD"})
            if (device.size(std::string("iPod_Control/iTunes/") + name)) throw failure("alternative_database");
    } else {
        out.old_database_ = read_all(device, r.database_path);
        out.old_digest_ = sha256(out.old_database_);
        references_exist(device, validate_(out.old_database_));
    }
    out.new_references_ = validate_(r.new_database);
    for (const auto& ref : out.new_references_) if (!safe_path(ref)) throw failure("unsafe_reference");
    for (const auto& file : r.additions) {
        if (!safe_path(file.source) || sources_.size(file.source) != file.size || fingerprint(sources_, file.source) != file.digest ||
            device.size(file.target) || device.size(temp(rec, file.target)) || !contains(out.new_references_, file.target)) throw failure("media_preflight");
    }
    for (const auto& ref : out.new_references_) {
        const bool added = std::any_of(r.additions.begin(), r.additions.end(), [&](const auto& file) { return folded(file.target) == folded(ref); });
        if (!added && !device.size(ref)) throw failure("missing_reference");
    }
    const auto old_refs = out.old_database_.empty() ? std::vector<std::string>{} : validate_(out.old_database_);
    for (const auto& file : r.deletions)
        if (!contains(old_refs, file.target) || contains(out.new_references_, file.target) || !matches(device, file.target, file.digest)) throw failure("deletion_preflight");
    room(device, r, out.old_database_.size());
    return out;
}

result engine::execute(const plan& input) {
    std::unique_lock lock(write_mutex, std::try_to_lock);
    if (!lock.owns_lock()) return {outcome::busy, false, false, "busy"};
    bool journal = false, committing = false, committed = false;
    const auto& r = input.request_;
    record rec{r, input.old_digest_, sha256(r.new_database)};
    try {
        hooks_.progress(phase::preparing);
        if (hooks_.cancelled()) return {outcome::cancelled, false, false, "cancelled"};
        const auto refreshed = prepare(r);
        if (refreshed.old_digest_ != input.old_digest_) throw failure("plan_stale");
        if (r.kind == operation::update && r.new_database == input.old_database_ && r.additions.empty() && r.deletions.empty())
            return {outcome::completed, false, false, "no_changes"};
        guarded device(device_, hooks_, r.device);
        const auto pre = prefix(rec);
        if (host_.available() < r.new_database.size() + input.old_database_.size() + 1024 * 1024) throw failure("backup_space");
        if (!input.old_database_.empty()) detail::write(host_, pre + "old.db", input.old_database_);
        detail::write(host_, pre + "new.db", r.new_database);
        if (!input.old_database_.empty()) save_snapshot(host_, rec, validate_, "-before", input.old_database_, true);
        const auto manifest = detail::encode(serialize(rec));
        detail::write(host_, pre + "manifest", manifest);
        journal = true;
        detail::write(device, marker(rec), manifest);
        const auto cancel = [&] { if (hooks_.cancelled()) throw failure("cancelled"); };
        hooks_.progress(phase::copying);
        bytes buffer(1024 * 1024);
        for (const auto& file : r.additions) {
            cancel();
            const auto path = temp(rec, file.target);
            device.create(path);
            detail::hash hash;
            std::uint64_t offset{};
            while (offset < file.size) {
                cancel();
                const auto wanted = static_cast<std::size_t>(std::min<std::uint64_t>(buffer.size(), file.size - offset));
                const auto count = sources_.read(file.source, offset, std::span(buffer).first(wanted));
                if (count != wanted) throw failure("source_short_read");
                hash.add(std::span(buffer).first(count));
                device.append(path, std::span(buffer).first(count));
                offset += count;
            }
            if (hash.finish() != file.digest || sources_.size(file.source) != file.size) throw failure("source_changed");
            device.flush(path);
            if (device.size(path) != file.size) throw failure("media_short_write");
            cancel();
            device.rename(path, file.target);
        }
        cancel();
        hooks_.progress(phase::staging_database);
        const auto staged = temp(rec, r.database_path);
        detail::write(device, staged, r.new_database);
        references_exist(device, checked_db(device, staged, rec.new_hash, validate_));
        cancel();
        if (r.kind == operation::update) {
            if (!matches(device, r.database_path, rec.old_hash)) throw failure("database_changed");
        } else if (device.size(r.database_path)) throw failure("database_changed");
        hooks_.progress(phase::committing);
        cancel(); // Last cancellable boundary, before either rename.
        committing = true;
        detail::write(device, marker(rec) + ".commit", detail::encode(detail::seal(rec.new_hash + "\n")));
        if (r.kind == operation::update) device.rename(r.database_path, old_path(rec));
        device.rename(staged, r.database_path);
        const auto refs = checked_db(device, r.database_path, rec.new_hash, validate_);
        references_exist(device, refs);
        committed = true;
        hooks_.progress(phase::cleaning);
        const bool clean = cleanup(device, rec, refs, true);
        if (clean) finish_snapshots(host_, rec, validate_, true);
        return {clean ? outcome::completed : outcome::cleaning_required, true, hooks_.cancelled(), clean ? "completed" : "cleanup_pending"};
    } catch (const failure& e) {
        if (committed) return {outcome::cleaning_required, true, hooks_.cancelled(), e.what()};
        if (committing) return {outcome::recovery_required, false, hooks_.cancelled(), e.what()};
        if (journal && std::string_view(e.what()) == "cancelled") {
            try {
                guarded device(device_, hooks_, r.device);
                const auto refs = input.old_database_.empty() ? std::vector<std::string>{} : validate_(input.old_database_);
                if (!input.old_digest_.empty() && !matches(device, r.database_path, input.old_digest_)) throw failure("database_changed");
                references_exist(device, refs);
                const bool clean = cleanup(device, rec, refs, false);
                if (clean) finish_snapshots(host_, rec, validate_, false);
                return {clean ? outcome::cancelled : outcome::cleaning_required, false, false, clean ? "cancelled" : "cancelled_cleanup_pending"};
            } catch (const failure& cleanup_error) {
                return {outcome::recovery_required, false, false, cleanup_error.what()};
            }
        }
        if (journal) {
            // Do not retry I/O here. Recovery is a separate request after the
            // caller has established a usable, positively identified session.
            return {outcome::recovery_required, false, false, e.what()};
        }
        return {outcome::failed, false, false, e.what()};
    }
}
result engine::recover(const std::string& device_key, const std::string& operation_id) {
    std::unique_lock lock(write_mutex, std::try_to_lock);
    if (!lock.owns_lock()) return {outcome::busy, false, false, "busy"};
    try { return recover_record(device_, host_, hooks_, validate_, device_key, operation_id); }
    catch (const failure& e) { return {outcome::recovery_required, false, false, e.what()}; }
}

std::vector<baseline_entry> verify_baseline(filesystem& source, filesystem& backup, const std::vector<std::string>& paths) {
    if (&source == &backup || paths.empty()) throw failure("invalid_baseline");
    std::vector<baseline_entry> out;
    std::set<std::string> seen;
    for (const auto& path : paths) {
        if (!safe_path(path) || !seen.insert(folded(path)).second) throw failure("invalid_baseline_path");
        const auto size = source.size(path);
        if (!size || backup.size(path) != size) throw failure("baseline_size");
        const auto digest = fingerprint(source, path);
        if (fingerprint(backup, path) != digest || fingerprint(source, path) != digest) throw failure("baseline_changed");
        out.push_back({path, *size, digest});
    }
    return out;
}
std::vector<std::string> retention_candidates(const std::vector<snapshot>& snapshots) {
    // Refuse automatic pruning until a validated LKG has been explicitly named.
    if (std::none_of(snapshots.begin(), snapshots.end(), [](const auto& s) { return s.validated && s.last_known_good; })) return {};
    auto sorted = snapshots;
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.sequence > b.sequence; });
    std::size_t valid{};
    std::vector<std::string> out;
    for (const auto& item : sorted) {
        if (!item.validated) continue;
        ++valid;
        if (valid > 10 && !item.last_known_good && !item.active_recovery) out.push_back(item.id);
    }
    return out;
}
std::vector<baseline_entry> verify_baseline_tree(filesystem& source, filesystem& backup, const std::string& directory) {
    const auto paths = enumerate_files(source, directory);
    if (paths != enumerate_files(backup, directory)) throw failure("baseline_tree_mismatch");
    const auto entries = verify_baseline(source, backup, paths);
    if (paths != enumerate_files(source, directory) || paths != enumerate_files(backup, directory)) throw failure("baseline_tree_changed");
    // Recheck the whole source after the final backup read, not only each file
    // immediately after its individual copy comparison.
    for (const auto& entry : entries)
        if (source.size(entry.path) != entry.size || fingerprint(source, entry.path) != entry.digest ||
            backup.size(entry.path) != entry.size || fingerprint(backup, entry.path) != entry.digest) throw failure("baseline_changed");
    if (paths != enumerate_files(source, directory) || paths != enumerate_files(backup, directory)) throw failure("baseline_tree_changed");
    return entries;
}
} // namespace foopodbridge::core::transaction

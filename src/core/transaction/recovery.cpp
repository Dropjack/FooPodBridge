// SPDX-License-Identifier: LGPL-3.0-or-later
#include "foopodbridge/core/transaction/recovery.h"
#include <algorithm>
#include <cwctype>
#include "internal.h"
#include <iomanip>
#include <sstream>

namespace foopodbridge::core::transaction {
namespace {
std::wstring normalize(const std::filesystem::path& path) {
    auto text = std::filesystem::absolute(path).lexically_normal().wstring();
    std::transform(text.begin(), text.end(), text.begin(), [](wchar_t c) { return static_cast<wchar_t>(towupper(c)); });
    while (!text.empty() && (text.back() == L'\\' || text.back() == L'/')) text.pop_back();
    return text + L'\\';
}
}
recovery_session::recovery_session(std::filesystem::path target, std::filesystem::path repository,
    identity expected, std::string task, database_validator validator, callbacks hooks)
    : target_(std::move(target)), repository_(std::move(repository)), expected_(std::move(expected)),
      task_(std::move(task)), validator_(std::move(validator)), hooks_(std::move(hooks)) {
    if (!validator_ || !hooks_.current_identity) throw failure("missing_adapter");
    const auto a = normalize(target_), b = normalize(repository_);
    if (a.starts_with(b) || b.starts_with(a)) throw failure("recovery_repository_overlap");
    check(false);
}
void remember_backup(filesystem& repository, const baseline_proof& proof, const identity& expected,
    const std::string& task, const std::filesystem::path& source, const std::filesystem::path& backup) {
    if (!detail::key(expected.device) || !proof.matches(expected, task, source, backup)) throw failure("baseline_required");
    std::ostringstream manifest;
    for (const auto& entry : proof.entries()) manifest << std::quoted(entry.path) << ' ' << entry.size << ' ' << entry.digest << '\n';
    const auto native = std::filesystem::absolute(backup).lexically_normal().u8string();
    std::ostringstream body;
    body << "FPBBACKUP1\n" << expected.device << '\n'
         << std::quoted(std::string(native.begin(), native.end())) << '\n'
         << proof.entries().size() << '\n' << sha256(detail::encode(manifest.str())) << '\n';
    const auto data = detail::encode(detail::seal(body.str()));
    const auto name = "baselines/" + expected.device + "/" + sha256(data) + ".ref";
    if (repository.size(name)) {
        if (read_all(repository, name, 65536) != data) throw failure("baseline_record_conflict");
    } else detail::write(repository, name, data);
}
std::vector<backup_reference> find_backups(filesystem& repository, const std::string& device_key) {
    if (!detail::key(device_key)) throw failure("baseline_identity");
    std::vector<backup_reference> result;
    const auto base = "baselines/" + device_key;
    for (const auto& name : repository.list(base)) {
        if (!name.ends_with(".ref")) continue;
        if (result.size() >= 1000 || name.size() != 68 || !detail::digest(name.substr(0, 64))) throw failure("baseline_catalog");
        const auto bytes = read_all(repository, base + "/" + name, 65536);
        if (sha256(bytes) != name.substr(0, 64)) throw failure("baseline_record_hash");
        std::istringstream input(detail::unseal(bytes));
        std::string magic, device, directory;
        backup_reference reference;
        input >> magic >> device >> std::quoted(directory) >> reference.files >> reference.manifest_digest;
        if (!input || magic != "FPBBACKUP1" || device != device_key || directory.empty() ||
            directory.find('\0') != std::string::npos || !detail::digest(reference.manifest_digest)) throw failure("baseline_record");
        input >> std::ws;
        if (!input.eof()) throw failure("baseline_record");
        reference.directory = std::filesystem::path(std::u8string(directory.begin(), directory.end()));
        if (!reference.directory.is_absolute()) throw failure("baseline_path");
        result.push_back(std::move(reference));
    }
    return result;
}
void recovery_session::check(bool require_baseline) {
    if (hooks_.current_identity() != expected_) throw failure("session_expired");
    if (hooks_.cancelled()) throw failure("cancelled");
    if (require_baseline && (!baseline_ || !baseline_->matches(expected_, task_, target_, backup_)))
        throw failure("baseline_required");
}
recovery_catalog recovery_session::inspect() {
    check(false);
    auto device = local_directory(target_), host = local_directory(repository_);
    recovery_catalog out;
    out.journals = discover_recovery(*device);
    const auto directory = "transactions/" + expected_.device;
    for (const auto& name : host->list(directory)) {
        if (!safe_path(name) || name.find('/') != std::string::npos || out.journals.size() > 10000) throw failure("recovery_catalog_limit");
        if (!host->is_directory(directory + "/" + name)) continue;
        const auto base = directory + "/" + name + "/";
        if (!host->size(base + "manifest")) continue;
        journal_info record;
        try {
            const auto manifest = read_all(*host, base + "manifest", 16 * 1024 * 1024);
            if (host->size(base + "finalized")) {
                const auto done = read_all(*host, base + "finalized", 1024);
                record = inspect_journal(manifest, std::span<const std::byte>(done));
            } else record = inspect_journal(manifest);
            if (record.operation_id != name || record.device_key != expected_.device) record.status = journal_state::invalid;
        } catch (const failure&) { record.status = journal_state::invalid; }
        const auto existing = std::find_if(out.journals.begin(), out.journals.end(), [&](const auto& j) { return j.operation_id == name; });
        if (existing == out.journals.end()) out.journals.push_back(record);
        else if (record.status == journal_state::invalid || existing->status == journal_state::invalid) existing->status = journal_state::invalid;
        else if (record.status == journal_state::pending) existing->status = journal_state::pending;
    }
    snapshot_store store(*host, expected_.device, validator_);
    out.snapshots = store.inspect();
    check(false);
    return out;
}
void recovery_session::verify_backup(const std::filesystem::path& backup) {
    baseline_.reset(); check(false);
    const auto a = normalize(backup), b = normalize(repository_);
    if (a.starts_with(b) || b.starts_with(a)) throw failure("recovery_backup_overlap");
    baseline_ = verify_directory_baseline(target_, backup, task_, expected_, [&] { check(false); return hooks_.current_identity(); });
    auto host = local_directory(repository_);
    remember_backup(*host, *baseline_, expected_, task_, target_, backup);
    backup_ = backup;
}
result recovery_session::resume(const std::string& operation_id) {
    try {
        check(false);
        const auto catalog = inspect();
        if (std::any_of(catalog.journals.begin(), catalog.journals.end(), [&](const auto& j) {
            return j.status == journal_state::invalid || j.device_key != expected_.device;
        })) throw failure("recovery_catalog_invalid");
        const auto found = std::find_if(catalog.journals.begin(), catalog.journals.end(), [&](const auto& j) {
            return j.operation_id == operation_id && j.status == journal_state::pending;
        });
        if (found == catalog.journals.end()) throw failure("pending_operation_required");
        auto device = local_directory(target_), host = local_directory(repository_);
        engine service(*device, *host, *host, validator_, hooks_);
        check(false);
        return service.recover(expected_.device, operation_id);
    } catch (const failure& e) { return {outcome::recovery_required, false, false, e.what()}; }
}
result recovery_session::restore_last_known_good(const std::string& snapshot_id, const std::string& operation_id) {
    try {
        check(true);
        const auto catalog = inspect();
        if (std::any_of(catalog.journals.begin(), catalog.journals.end(), [&](const auto& j) {
            return j.status != journal_state::completed || j.device_key != expected_.device;
        })) throw failure("finish_pending_recovery_first");
        baseline_ = verify_directory_baseline(target_, backup_, task_, expected_, [&] { check(false); return hooks_.current_identity(); });
        auto device = local_directory(target_), host = local_directory(repository_);
        snapshot_store store(*host, expected_.device, validator_);
        request input;
        input.id = operation_id; input.device = expected_;
        input.database_path = "iPod_Control/iTunes/iTunesDB";
        input.new_database = store.load_last_known_good(snapshot_id);
        engine service(*device, *host, *host, validator_, hooks_);
        const auto plan = service.prepare(std::move(input));
        check(true);
        auto result = service.execute(plan);
        baseline_.reset();
        return result;
    } catch (const failure& e) { return {outcome::failed, false, false, e.what()}; }
}
}

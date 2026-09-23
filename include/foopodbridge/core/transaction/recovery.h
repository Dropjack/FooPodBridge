// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "foopodbridge/core/transaction/transaction.h"

namespace foopodbridge::core::transaction {
// A saved location is evidence of an earlier check, never a current proof.
struct backup_reference {
    std::filesystem::path directory;
    std::uint64_t files{};
    std::string manifest_digest;
};
void remember_backup(filesystem& repository, const baseline_proof& proof, const identity& expected,
    const std::string& task, const std::filesystem::path& source, const std::filesystem::path& backup);
std::vector<backup_reference> find_backups(filesystem& repository, const std::string& device_key);

// A directory recovery workspace is a permanent offline recovery facility. It
// never enumerates hardware or acquires a physical volume writer.
struct recovery_catalog {
    std::vector<journal_info> journals;
    std::vector<snapshot> snapshots;
};
class recovery_session final {
public:
    recovery_session(std::filesystem::path target, std::filesystem::path repository,
        identity expected, std::string task, database_validator validator, callbacks hooks);
    recovery_catalog inspect();
    void verify_backup(const std::filesystem::path& backup);
    result resume(const std::string& operation_id);
    result restore_last_known_good(const std::string& snapshot_id, const std::string& operation_id);
private:
    void check(bool require_baseline);
    std::filesystem::path target_, repository_, backup_;
    identity expected_;
    std::string task_;
    database_validator validator_;
    callbacks hooks_;
    std::optional<baseline_proof> baseline_;
};
}

// SPDX-License-Identifier: LGPL-3.0-or-later
#include "foopodbridge/core/device/device.h"
#include "foopodbridge/core/transaction/recovery.h"
#include "foopodbridge/core/database/hash58.h"
#include "foopodbridge/core/database/reader.h"
#include "foopodbridge/core/database/validator.h"
#include <algorithm>

namespace foopodbridge::core::device {
namespace tx = transaction;
std::string recovery_repository_key(const candidate& c) {
    if (!c.mounted || !c.mapping_valid || !c.identity_complete || !identify(c.hardware_id, c.serial_suffix).positive)
        return {};
    const auto key = database::parse_hash58_device_key(c.signing_identity);
    if (!key) return {};
    auto text = c.signing_identity;
    for (auto& ch : text) if (ch >= 'a' && ch <= 'f') ch = static_cast<char>(ch - 'a' + 'A');
    // Namespace and hash keep raw hardware identifiers out of filenames/UI.
    text = "FooPodBridge-recovery-v1:" + text;
    return tx::sha256(std::as_bytes(std::span(text.data(), text.size())));
}
recovery_summary inspect_repository(const candidate& c, const std::filesystem::path& repository, std::stop_token stop) {
    recovery_summary out;
    try {
        const auto key = recovery_repository_key(c);
        if (key.empty()) return out;
        out.link = recovery_link::repository_missing;
        if (repository.empty()) return out;
        std::error_code error;
        const bool exists = std::filesystem::exists(repository, error);
        if (error) throw tx::failure("repository_unavailable");
        if (!exists) return out;
        auto host = tx::local_directory(repository);
        const auto check = [&] { if (stop.stop_requested()) throw tx::failure("cancelled"); };
        std::size_t records{};
        const auto directory = "transactions/" + key;
        for (const auto& name : host->list(directory)) {
            check();
            if (++records > 10000) throw tx::failure("repository_limit");
            if (!tx::safe_path(name) || name.find('/') != std::string::npos) throw tx::failure("repository_invalid");
            if (!host->is_directory(directory + "/" + name)) continue;
            const auto base = directory + "/" + name + "/";
            if (!host->size(base + "manifest")) continue;
            tx::journal_info record;
            try {
                const auto manifest = tx::read_all(*host, base + "manifest", 16 * 1024 * 1024);
                if (host->size(base + "finalized")) {
                    const auto done = tx::read_all(*host, base + "finalized", 1024);
                    record = tx::inspect_journal(manifest, std::span<const std::byte>(done));
                } else record = tx::inspect_journal(manifest);
                if (record.device_key != key || record.operation_id != name) record.status = tx::journal_state::invalid;
            } catch (const tx::failure&) { record.status = tx::journal_state::invalid; }
            if (record.status == tx::journal_state::pending) ++out.pending;
            if (record.status == tx::journal_state::invalid) ++out.invalid;
        }
        const auto id = identify(c.hardware_id, c.serial_suffix);
        // Read validation is not write authorization. Preserve-only profiles
        // may inspect historical databases without enabling a writer.
        tx::snapshot_store snapshots(*host, key, [&](std::span<const std::byte> bytes) {
            check();
            if (id.group == family::shuffle || id.group == family::nano_later || c.alternative_database)
                throw tx::failure("profile_unavailable");
            const auto profile = database::traditional_preserve_only_profile();
            const auto parsed = id.group == family::signed_traditional
                ? database::reader{}.read(bytes, database::traditional_hash58_profile(), database::parse_hash58_device_key(c.signing_identity).value())
                : database::reader{}.read(bytes, profile);
            if (!parsed || !database::validator{}.validate(parsed.value())) throw tx::failure("snapshot_invalid");
            return std::vector<std::string>{};
        });
        for (const auto& entry : snapshots.inspect()) {
            check();
            if (!entry.validated) ++out.invalid;
            else { ++out.snapshots; if (entry.last_known_good) ++out.last_known_good; }
        }
        check();
        out.backups = static_cast<std::uint32_t>(tx::find_backups(*host, key).size());
        check();
        out.link = recovery_link::available;
    } catch (...) { out = {}; out.link = recovery_link::unavailable; }
    return out;
}
} // namespace foopodbridge::core::device

#include "database_test_support.h"
#include "foopodbridge/core/transaction/database_adapter.h"
#include "foopodbridge/core/transaction/recovery.h"
#include <Windows.h>
#include <filesystem>
#include <iostream>

using namespace foopodbridge::core;
using namespace foopodbridge::core::transaction;
using foopodbridge::tests::require;
namespace {
void put(filesystem& fs, const std::string& name, const bytes& data) {
    fs.create(name); fs.append(name, data); fs.flush(name);
}
bytes make_db(bool music) {
    auto doc = foopodbridge::tests::make_empty();
    if (music) {
        database::edit_plan edit;
        edit.operations.push_back(database::add_track{":iPod_Control:Music:F00:song.mp3", "Synthetic", "", ""});
        auto modified = database::editor{}.apply(doc, edit, foopodbridge::tests::fixed_generation());
        require(modified.has_value(), "fixture edit");
        doc = modified.value();
    }
    const auto data = database::writer{}.write(doc, database::traditional_unsigned_profile(), foopodbridge::tests::fixed_generation());
    require(data.has_value(), "fixture write");
    return data.value();
}
}
int main() {
    try {
        const auto root = std::filesystem::current_path() / ("transaction-fixture-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()));
        for (const auto* dir : {"device", "host", "source", "backup"}) std::filesystem::create_directories(root / dir);
        std::cout << "Opening test directories\n";
        auto device = local_directory(root / "device");
        auto host = local_directory(root / "host");
        auto source = local_directory(root / "source");
        std::cout << "Creating fixtures\n";
        const auto old_db = make_db(false), new_db = make_db(true);
        const bytes audio(2048, std::byte{0x25});
        put(*source, "song.mp3", audio);
        put(*device, "iPod_Control/iTunes/iTunesDB", old_db);
        identity current{"device-test", 7, 3, true};
        bool cancel = false;
        callbacks hooks;
        hooks.current_identity = [&] { return current; };
        hooks.cancelled = [&] { return cancel; };
        engine tx(*device, *host, *source, traditional_database_validator(database::traditional_unsigned_profile()), hooks);
        request input;
        input.id = "import-1"; input.device = current;
        input.database_path = "iPod_Control/iTunes/iTunesDB"; input.new_database = new_db;
        input.additions.push_back({"song.mp3", "iPod_Control/Music/F00/song.mp3", audio.size(), sha256(audio)});
        std::cout << "Preparing real database transaction\n";
        const auto prepared = tx.prepare(input);
        cancel = true;
        require(tx.execute(prepared).status == outcome::cancelled, "pre-start cancel");
        cancel = false;
        const auto imported = tx.execute(prepared);
        if (imported.status != outcome::completed) throw std::runtime_error(imported.code);
        require(imported.committed && read_all(*device, input.database_path) == new_db, "import commit");
        require(read_all(*device, input.additions.front().target) == audio, "media bytes");
        const auto recovered = tx.recover(current.device, input.id);
        require(recovered.status == outcome::completed && recovered.committed, "completed recovery idempotence");
        const auto journals = discover_recovery(*device);
        require(journals.size() == 1 && journals.front().status == journal_state::completed, "completed journal discovery");
        const auto manifest = read_all(*host, "transactions/device-test/import-1/manifest");
        require(inspect_journal(manifest).status == journal_state::pending, "pending journal discovery");
        auto corrupt = manifest; corrupt.front() ^= std::byte{1};
        require(inspect_journal(corrupt).status == journal_state::invalid, "corrupt journal discovery");
        input.id = "delete-2"; input.new_database = old_db;
        input.additions.clear(); input.deletions.push_back({"iPod_Control/Music/F00/song.mp3", sha256(audio)});
        require(tx.execute(tx.prepare(input)).status == outcome::completed, "delete commit");
        require(!device->size("iPod_Control/Music/F00/song.mp3"), "delete cleanup");
        const auto historical = tx.recover(current.device, "import-1");
        require(historical.status == outcome::recovery_required && read_all(*device, input.database_path) == old_db, "historical transaction must not run cleanup again");
        snapshot_store actual(*host, current.device, traditional_database_validator(database::traditional_unsigned_profile()));
        require(actual.inspect().size() == 4, "transaction snapshots not saved");
        for (const auto& point : actual.inspect()) require(!point.last_known_good, "unaccepted commit promoted to LKG");
        snapshot_store snapshots(*host, "retention-fixture", traditional_database_validator(database::traditional_unsigned_profile()));
        for (unsigned i = 0; i < 14; ++i) snapshots.save("snapshot-" + std::to_string(i), i, old_db, i == 1);
        snapshots.mark_last_known_good("snapshot-0"); snapshots.prune();
        require(snapshots.load_last_known_good("snapshot-0") == old_db, "load validated LKG");
        require(snapshots.inspect().size() == 12, "ten plus LKG and active snapshot");
        snapshots.release_recovery("snapshot-1"); snapshots.prune();
        require(snapshots.inspect().size() == 11, "ten plus LKG");
        auto backup = local_directory(root / "backup");
        put(*backup, "song.mp3", audio);
        require(verify_baseline(*source, *backup, {"song.mp3"}).size() == 1, "baseline validation");
        put(*source, "iPod_Control/Music/F00/a.mp3", audio);
        put(*backup, "iPod_Control/Music/F00/a.mp3", audio);
        require(verify_baseline_tree(*source, *backup, "iPod_Control").size() == 1, "complete baseline enumeration");
        const auto proof = verify_directory_baseline(root / "source", root / "backup", "task-007", current, [&] { return current; });
        require(proof.matches(current, "task-007", root / "source", root / "backup"), "baseline binding");
        auto changed_identity = current; ++changed_identity.generation;
        require(!proof.matches(changed_identity, "task-007", root / "source", root / "backup"), "stale baseline generation");
        require(!proof.matches(current, "other-task", root / "source", root / "backup"), "baseline wrong task");
        bool overlap_rejected = false;
        try { (void)verify_directory_baseline(root / "source", root / "source", "task-007", current, [&] { return current; }); }
        catch (const failure&) { overlap_rejected = true; }
        require(overlap_rejected, "overlapping backup accepted");
        auto alternative = database::writer{}.create_empty("Recovered Library", database::traditional_unsigned_profile(), foopodbridge::tests::fixed_generation());
        require(alternative.has_value(), "alternate LKG database");
        actual.save("accepted-baseline", 50, alternative.value().original_bytes);
        actual.mark_last_known_good("accepted-baseline");
        auto device_backup_path = root / "device-backup";
        std::filesystem::create_directory(device_backup_path);
        std::filesystem::copy(root / "device" / "iPod_Control", device_backup_path / "iPod_Control", std::filesystem::copy_options::recursive);
        recovery_session recovery(root / "device", root / "host", current, "task-007",
            traditional_database_validator(database::traditional_unsigned_profile()), hooks);
        require(recovery.restore_last_known_good("accepted-baseline", "restore-3").code == "baseline_required", "restore skipped baseline");
        recovery.verify_backup(device_backup_path);
        const auto restored = recovery.restore_last_known_good("accepted-baseline", "restore-3");
        require(restored.status == outcome::completed && restored.committed, "LKG restore failed");
        require(read_all(*device, input.database_path) == alternative.value().original_bytes, "LKG restore bytes");
        ++current.generation;
        require(recovery.resume("restore-3").code == "session_expired", "stale recovery session");
        --current.generation;
        input.id = "interrupted-4"; input.device = current; input.new_database = old_db; input.deletions.clear();
        callbacks crashing = hooks;
        struct interruption {};
        crashing.progress = [](phase value) { if (value == phase::committing) throw interruption{}; };
        engine interrupted(*device, *host, *source, traditional_database_validator(database::traditional_unsigned_profile()), crashing);
        try { (void)interrupted.execute(interrupted.prepare(input)); } catch (const interruption&) {}
        const auto recovery_points = recovery.inspect();
        require(std::any_of(recovery_points.journals.begin(), recovery_points.journals.end(), [](const auto& j) {
            return j.operation_id == "interrupted-4" && j.status == journal_state::pending;
        }), "service pending discovery");
        const auto resumed = recovery.resume("interrupted-4");
        require(resumed.status == outcome::completed && !resumed.committed, "service recovery resume");
        require(read_all(*device, input.database_path) == alternative.value().original_bytes, "precommit recovery changed database");
        put(*source, "iPod_Control/Music/F01/omitted.mp3", audio);
        bool incomplete_rejected = false;
        try { (void)verify_baseline_tree(*source, *backup, "iPod_Control"); } catch (const failure&) { incomplete_rejected = true; }
        require(incomplete_rejected, "incomplete backup accepted");
        for (const auto* bad : {"../escape", "C:/file", "a//b", "a/../b", "a/CON.txt", "a/b:ads", "a/b.", "a\\b"}) require(!safe_path(bad), "unsafe path accepted");
        std::cout << "Real directory / Reader-Writer / import-delete / snapshots checks passed. Fixture retained: " << root.string() << '\n';
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

#include "database_test_support.h"
#include "foopodbridge/core/transaction/database_adapter.h"
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
        require(snapshots.inspect().size() == 12, "ten plus LKG and active snapshot");
        snapshots.release_recovery("snapshot-1"); snapshots.prune();
        require(snapshots.inspect().size() == 11, "ten plus LKG");
        auto backup = local_directory(root / "backup");
        put(*backup, "song.mp3", audio);
        require(verify_baseline(*source, *backup, {"song.mp3"}).size() == 1, "baseline validation");
        for (const auto* bad : {"../escape", "C:/file", "a//b", "a/../b", "a/CON.txt", "a/b:ads", "a/b.", "a\\b"}) require(!safe_path(bad), "unsafe path accepted");
        std::cout << "Real directory / Reader-Writer / import-delete / snapshots checks passed. Fixture retained: " << root.string() << '\n';
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

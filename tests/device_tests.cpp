// SPDX-License-Identifier: LGPL-3.0-or-later
#include "database_test_support.h"
#include "foopodbridge/core/device/device.h"
#include "foopodbridge/core/database/hash58.h"
#include "foopodbridge/core/database/reader.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <set>
#include <Windows.h>
#include "foopodbridge/core/transaction/recovery.h"
#include "foopodbridge/core/transaction/database_adapter.h"

using namespace foopodbridge::core;
using namespace foopodbridge::core::device;
using foopodbridge::tests::require;

namespace {
candidate normal() {
    candidate c;
    c.physical_key = "synthetic-device";
    c.volume_key = "synthetic-volume";
    c.hardware_id = "USB\\VID_05AC&PID_1209\\TEST";
    c.identity_complete = true;
    return c;
}
struct simulated_backend : read_backend {
    std::mutex mutex;
    std::condition_variable condition;
    std::function<void()> callback;
    bool block{}, entered{}, present{true};
    bool enumeration_failure{};
    std::string throwing_device;
    std::vector<std::string> reads;
    std::vector<candidate> devices{normal()};
    std::vector<candidate> enumerate(std::stop_token) override {
        std::lock_guard lock(mutex);
        if (enumeration_failure) throw std::runtime_error("private enumeration diagnostic");
        return devices;
    }
    file_result read_database(const candidate& target, std::stop_token cancel) override {
        std::unique_lock lock(mutex);
        reads.push_back(target.physical_key);
        if (target.physical_key == throwing_device) throw std::runtime_error("private device diagnostic");
        entered = true;
        condition.notify_all();
        std::stop_callback wake(cancel, [&] { condition.notify_all(); });
        condition.wait(lock, [&] { return !block || cancel.stop_requested(); });
        if (cancel.stop_requested()) return {{}, reason::cancelled};
        return {foopodbridge::tests::make_empty().original_bytes};
    }
    bool still_present(const candidate&) override { std::lock_guard lock(mutex); return present; }
    void watch(std::function<void()> cb) override { callback = std::move(cb); }
    void unwatch() noexcept override { callback = {}; }
};
void wait_ready(discovery& d) {
    for (int i = 0; i < 200; ++i) {
        if (!d.current().scanning) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    throw std::runtime_error("discovery timeout");
}
void recovery_association_cases() {
    namespace tx = transaction;
    auto c = normal();
    require(!recovery_repository_key(c).empty(), "stable physical identity was not associated");
    c.signing_identity = "0123456789aBcDeF";
    const auto key = recovery_repository_key(c);
    require(key.size() == 64 && key.find(c.signing_identity) == std::string::npos, "private recovery identity");
    auto remount = c; remount.volume_key = "new-volume"; remount.signing_identity = "0123456789ABCDEF";
    require(recovery_repository_key(remount) == key, "remount or casing changed association");
    auto other = c; other.physical_key = "other-physical-device";
    require(recovery_repository_key(other) != key, "different devices associated");
    auto bad = c; bad.mapping_valid = false;
    require(recovery_repository_key(bad).empty(), "conflicting mapping associated");
    bad = c; bad.physical_key.clear();
    require(recovery_repository_key(bad).empty(), "missing stable identity associated");
    const auto root = std::filesystem::current_path() / ("association-fixture-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()));
    const auto repository = root / "host";
    require(inspect_repository(c, repository, {}).link == recovery_link::repository_missing, "missing repository misreported");
    require(!std::filesystem::exists(root), "discovery created repository");
    for (const auto* dir : {"host", "source", "backup"}) std::filesystem::create_directories(root / dir);
    auto host = tx::local_directory(repository);
    auto source = tx::local_directory(root / "source"), backup = tx::local_directory(root / "backup");
    const auto bytes = foopodbridge::tests::make_empty().original_bytes;
    for (auto* fs : {source.get(), backup.get()}) {
        fs->create("iPod_Control/iTunes/iTunesDB"); fs->append("iPod_Control/iTunes/iTunesDB", bytes); fs->flush("iPod_Control/iTunes/iTunesDB");
    }
    const tx::identity identity{key, 1, 1, false};
    const auto proof = tx::verify_directory_baseline(root / "source", root / "backup", "fixture", identity, [&] { return identity; });
    tx::remember_backup(*host, proof, identity, "fixture", root / "source", root / "backup");
    tx::remember_backup(*host, proof, identity, "fixture", root / "source", root / "backup");
    require(tx::find_backups(*host, key).size() == 1, "backup registration not idempotent");
    bool rejected{};
    auto stale = identity; ++stale.generation;
    try { tx::remember_backup(*host, proof, stale, "fixture", root / "source", root / "backup"); } catch (const tx::failure&) { rejected = true; }
    require(rejected, "expired baseline was registered");
    tx::snapshot_store snapshots(*host, key, tx::traditional_database_validator(database::traditional_unsigned_profile()));
    snapshots.save("baseline", 1, bytes); snapshots.mark_last_known_good("baseline");
    auto found = inspect_repository(c, repository, {});
    require(found.link == recovery_link::available && found.snapshots == 1 && found.last_known_good == 1 && found.backups == 1, "recovery records not associated");
    require(inspect_repository(other, repository, {}).snapshots == 0 && inspect_repository(other, repository, {}).backups == 0, "another device saw backup");
    std::stop_source cancelled; cancelled.request_stop();
    require(inspect_repository(c, repository, cancelled.get_token()).link == recovery_link::unavailable, "cancelled scan published");
    const auto before = tx::read_all(*host, "snapshots/" + key + "/baseline.db");
    auto backend = std::make_unique<simulated_backend>(); auto* simulation = backend.get();
    simulation->devices = {c};
    discovery d(std::move(backend), repository); d.start([] {}); wait_ready(d);
    const auto old = d.current().devices.at(0);
    require(old->recovery.snapshots == 1, "service snapshot omitted recovery");
    { std::lock_guard lock(simulation->mutex); simulation->devices = {remount}; }
    simulation->callback(); wait_ready(d);
    require(d.current().devices.at(0)->recovery.backups == 1 && !d.is_current(old->token, old->generation, old->revision), "reconnect association or invalidation failed");
    auto duplicate = c; duplicate.volume_key = "second-volume";
    { std::lock_guard lock(simulation->mutex); simulation->devices = {c, duplicate}; }
    simulation->callback(); wait_ready(d);
    for (const auto& item : d.current().devices) require(item->recovery.link == recovery_link::identity_unavailable, "duplicate identity selected recovery repository");
    d.stop();
    require(tx::read_all(*host, "snapshots/" + key + "/baseline.db") == before, "discovery modified snapshot");
    const std::string path = "transactions/" + key + "/broken/manifest";
    host->create(path); host->append(path, std::vector<std::byte>{std::byte{'x'}}); host->flush(path);
    found = inspect_repository(c, repository, {});
    require(found.invalid == 1 && found.snapshots == 1, "corrupt journal hidden");
    host->remove("snapshots/" + key + "/baseline.db");
    found = inspect_repository(c, repository, {});
    require(found.invalid == 2 && found.last_known_good == 0, "missing snapshot accepted as LKG");

    // A physical Classic identity can locate its private records even when
    // the independent hash58 signing input is not available on this mount.
    auto classic = normal();
    classic.physical_key = "synthetic-classic";
    classic.hardware_id = "USB\\VID_05AC&PID_1261\\TEST";
    classic.signing_identity = "0011223344556677";
    const auto classic_key = recovery_repository_key(classic);
    const auto signing_key = database::parse_hash58_device_key(classic.signing_identity);
    require(signing_key.has_value(), "synthetic Classic signing key invalid");
    const auto signed_doc = database::writer{}.create_empty("Library", database::traditional_hash58_profile(),
        signing_key.value(), foopodbridge::tests::fixed_generation());
    require(signed_doc.has_value(), "synthetic Classic database generation failed");
    tx::snapshot_store classic_snapshots(*host, classic_key,
        tx::traditional_database_validator(database::traditional_hash58_profile(), signing_key.value()));
    classic_snapshots.save("classic-baseline", 1, signed_doc.value().original_bytes);
    classic_snapshots.mark_last_known_good("classic-baseline");
    const tx::identity classic_identity{classic_key, 1, 1, false};
    const auto classic_proof = tx::verify_directory_baseline(root / "source", root / "backup", "fixture",
        classic_identity, [&] { return classic_identity; });
    tx::remember_backup(*host, classic_proof, classic_identity, "fixture", root / "source", root / "backup");
    classic.signing_identity.clear();
    classic.identity_complete = false;
    const auto unsigned_view = inspect_repository(classic, repository, {});
    require(unsigned_view.link == recovery_link::available && unsigned_view.backups == 1,
        "missing signing input hid physical recovery records");
    require(unsigned_view.snapshot_verification_deferred,
        "missing signing input did not mark Classic snapshot validation as deferred");
    require(unsigned_view.snapshots == 0 && unsigned_view.last_known_good == 0,
        "unverified Classic snapshot was offered as a recovery point");
    classic.signing_identity = "0011223344556677";
    const auto signed_view = inspect_repository(classic, repository, {});
    require(signed_view.link == recovery_link::available && signed_view.snapshots == 1
        && signed_view.last_known_good == 1 && !signed_view.snapshot_verification_deferred,
        "Classic snapshot not verified when signing input returned");
    std::cout << "Recovery association fixtures retained: " << root.string() << '\n';
}
void isolated_discovery_cases() {
    auto backend = std::make_unique<simulated_backend>();
    auto* simulation = backend.get();
    auto a = normal(), b = normal();
    b.physical_key = "second-device"; b.volume_key = "second-volume";
    simulation->devices = {a, b}; simulation->throwing_device = a.physical_key;
    discovery d(std::move(backend)); d.start([] {}); wait_ready(d);
    auto result = d.current();
    require(result.problem == reason::none && result.devices.size() == 2, "one device failure erased catalog");
    require(result.devices[0]->status == state::read_error && result.devices[0]->problem == reason::io_failure,
        "device exception not converted to private-safe read error");
    require(result.devices[1]->status == state::ready_read_only, "healthy second device lost");
    const auto old = result.devices[1];
    {
        std::lock_guard lock(simulation->mutex);
        simulation->enumeration_failure = true;
    }
    d.refresh(); wait_ready(d);
    require(d.current().problem == reason::enumeration_failed && d.current().devices.empty(), "enumeration failure retained online snapshots");
    require(!d.is_current(old->token, old->generation, old->revision), "failed enumeration leaves old snapshot current");
    {
        std::lock_guard lock(simulation->mutex);
        simulation->enumeration_failure = false; simulation->throwing_device.clear();
        b.volume_key = a.volume_key; simulation->devices = {a, b}; simulation->reads.clear();
    }
    d.refresh(); wait_ready(d);
    result = d.current();
    require(result.devices.size() == 2 && std::all_of(result.devices.begin(), result.devices.end(), [](const auto& s) {
        return s->status == state::unidentified && s->problem == reason::identity_conflict;
    }), "one volume assigned to two devices was trusted");
    {
        std::lock_guard lock(simulation->mutex);
        require(simulation->reads.empty(), "ambiguous volume database was opened");
        b = a; b.volume_key = "second-volume"; simulation->devices = {a, b};
    }
    d.refresh(); wait_ready(d);
    result = d.current();
    require(result.devices.size() == 2 && result.devices[0]->problem == reason::identity_conflict
        && result.devices[1]->problem == reason::identity_conflict, "ambiguous multi-volume device was trusted");
    {
        std::lock_guard lock(simulation->mutex);
        require(simulation->reads.empty(), "ambiguous multi-volume database was opened");
        simulation->devices = {a, a};
    }
    d.refresh(); wait_ready(d);
    result = d.current();
    require(result.devices.size() == 1 && result.devices[0]->status == state::ready_read_only
        && result.devices[0]->identity_complete, "exact duplicate evidence degraded identity");
    {
        std::lock_guard lock(simulation->mutex);
        require(simulation->reads.size() == 1, "exact duplicate caused multiple reads");
        simulation->present = false;
    }
    d.refresh(); wait_ready(d);
    require(d.current().devices[0]->problem == reason::removed && d.current().devices[0]->tracks.empty(), "late absence retained library");
    d.stop();

    auto preflight_backend = std::make_unique<simulated_backend>();
    auto* preflight = preflight_backend.get(); preflight->devices.clear();
    for (int i = 0; i < 7; ++i) {
        auto c = normal(); c.physical_key += std::to_string(i); c.volume_key += std::to_string(i);
        switch (i) {
        case 0: c.mounted = false; break;
        case 1: c.filesystem_supported = false; break;
        case 2: c.recovery_verified = true; break;
        case 3: c.hardware_id = "USB\\VID_05AC&PID_1300\\PUBLIC"; break;
        case 4: c.hardware_id = "USB\\VID_1111&PID_1263\\PUBLIC"; break;
        case 5: c.problem = reason::access_denied; break;
        case 6: c.mapping_valid = false; break;
        }
        preflight->devices.push_back(c);
    }
    discovery gated(std::move(preflight_backend)); gated.start([] {}); wait_ready(gated);
    require(gated.current().devices.size() == 7, "diagnostic candidates disappeared");
    {
        std::lock_guard lock(preflight->mutex);
        require(preflight->reads.empty(), "preflight-rejected candidate reached database I/O");
    }
    gated.stop();

    auto blocked_backend = std::make_unique<simulated_backend>();
    auto* blocked = blocked_backend.get(); blocked->block = true;
    discovery stopping(std::move(blocked_backend)); stopping.start([] {});
    {
        std::unique_lock lock(blocked->mutex);
        require(blocked->condition.wait_for(lock, std::chrono::seconds(2), [&] { return blocked->entered; }), "shutdown fixture did not enter read");
    }
    stopping.stop();
    require(stopping.current().stopped && stopping.current().devices.empty(), "blocked shutdown published late result");
}
}
int main() {
    try {
        require(registry().size() == 23, "target generations missing");
        std::set<std::string_view> ids;
        for (const auto& e : registry()) require(ids.insert(e.id).second && !e.source.empty(), "registry evidence/IDs invalid");
        require(!identify("USB\\VID_05AC&PID_1291\\X").positive, "touch incorrectly accepted");
        require(!identify("USB\\VID_05AC&PID_12FF\\X").positive, "USB prefix accepted");
        require(!identify("USB\\VID_1111&PID_1263\\X").positive, "non-Apple accepted");
        require(identify("1394\\Apple_Computer__Inc.&iPod&REV_0001\\X").positive, "FireWire omitted");
        require(identify("USB\\VID_05AC&PID_1263\\X").model_id == "nano4", "Nano route incorrect");
        require(identify("USB\\VID_05AC&PID_1261\\X", "9ZU").model_id == "classic2009", "Classic evidence ignored");
        for (const auto& entry : registry()) {
            if (entry.usb_product == 0) continue;
            char hardware[64]{};
            std::snprintf(hardware, sizeof(hardware), "USB\\VID_05AC&PID_%04X\\PUBLIC", entry.usb_product);
            require(identify(hardware).positive && identify(hardware).group == entry.group, "registered family unreachable");
        }
        for (const auto& path : {":iPod_Control:Music:F00:ABC.mp3", "iPod_Control/Music/F00/ABC.mp3"})
            require(safe_relative_path(path).has_value(), "valid path refused");
        for (const auto& path : {"../secret", "C:\\secret", ":iPod_Control:Music:..:secret", "//host/share", ":iPod_Control:Music:F00:file.mp3:stream", "iPod_Control/Music/CON", "iPod_Control/Music/file. "})
            require(!safe_relative_path(path), "unsafe path accepted");
        auto c = normal();
        file_result f{foopodbridge::tests::make_empty().original_bytes};
        require(inspect(c, f).status == state::ready_read_only, "valid empty library not readable");
        c.mounted = false;
        require(inspect(c, f).status == state::not_mounted, "unmounted state wrong");
        c = normal(); c.mapping_valid = false;
        require(inspect(c, f).status == state::unidentified, "mapping conflict ignored");
        c = normal(); c.filesystem_supported = false;
        require(inspect(c, f).status == state::unsupported_filesystem, "filesystem ignored");
        c = normal(); c.recovery_verified = true;
        require(inspect(c, f).status == state::recovery_required, "recovery state ignored");
        c = normal();
        require(inspect(c, {{}, reason::database_missing}).status == state::format_pending, "missing DB became writable empty");
        require(inspect(c, {}).status == state::database_corrupt, "zero byte DB accepted");
        require(inspect(c, {{}, reason::access_denied}).status == state::read_error, "access failure called corrupt");
        for (const auto problem : {reason::sharing_violation, reason::io_failure, reason::removed, reason::content_changed, reason::resource_limit, reason::unsafe_path, reason::cancelled})
            require(inspect(c, {{}, problem}).status == state::read_error && inspect(c, {{}, problem}).problem == problem, "read error lost its reason");
        auto corrupt = f; corrupt.bytes.resize(28);
        require(inspect(c, corrupt).status == state::database_corrupt, "truncated DB accepted");
        auto unknown_version = f; unknown_version.bytes[16] = std::byte{0xff};
        require(inspect(c, unknown_version).status == state::format_pending, "unknown version called corrupted");
        for (const auto* id : {"USB\\VID_05AC&PID_1265\\X", "USB\\VID_05AC&PID_1300\\X"}) {
            c.hardware_id = id;
            require(inspect(c, f).status == state::format_pending, "independent format falsely read");
        }
        c = normal(); c.hardware_id = "USB\\VID_05AC&PID_1263\\X";
        auto key = database::parse_hash58_device_key("0011223344556677");
        const auto signed_doc = database::writer{}.create_empty("Library", database::traditional_hash58_profile(), key.value(), foopodbridge::tests::fixed_generation());
        require(signed_doc.has_value(), "signed fixture failed");
        file_result sf{signed_doc.value().original_bytes};
        require(inspect(c, sf).signature == database::hash58_signature_status::not_checked, "missing key reported verified");
        c.signing_identity = "0011223344556677";
        require(inspect(c, sf).signature == database::hash58_signature_status::valid, "signature not verified");
        c.signing_identity = "0011223344556678";
        require(inspect(c, sf).status == state::database_corrupt, "wrong key accepted");
        c.signing_identity.clear();
        c = normal();
        auto projection_profile = database::traditional_unsigned_profile();
        projection_profile.track_header_size = 584;
        database::edit_plan add;
        add.operations.push_back(database::add_track{":iPod_Control:Music:F00:PUBLIC.mp3", "Public title", "Public artist", "Public album"});
        add.operations.push_back(database::add_ordinary_playlist{"Public list"});
        const auto edited = database::editor{}.apply(foopodbridge::tests::make_empty(), add, foopodbridge::tests::fixed_generation());
        require(edited.has_value(), "projection fixture edit failed");
        const auto written = database::writer{}.write(edited.value(), projection_profile, foopodbridge::tests::fixed_generation());
        require(written.has_value(), "projection fixture write failed");
        auto projection = inspect(c, {written.value()});
        require(projection.status == state::ready_read_only && projection.tracks.size() == 1 && projection.playlists.size() == 1, "Library projection lost items");
        require(projection.tracks[0].metadata.title == "Public title" && projection.tracks[0].relative_path == "iPod_Control/Music/F00/PUBLIC.mp3", "projection changed metadata/path");
        auto media_bytes = written.value();
        const auto document = database::reader{}.read(media_bytes, database::traditional_preserve_only_profile());
        std::size_t track_offset{};
        const auto find_track = [&](auto&& self, const database::record_node& node) -> void {
            if (node.marker == std::array<char, 4>{'m','h','i','t'}) track_offset = static_cast<std::size_t>(node.offset);
            for (const auto& child : node.children) self(self, child);
        };
        find_track(find_track, document.value().root);
        require(track_offset != 0, "projection fixture has no track record");
        for (const auto [raw, expected] : {std::pair{1U, media_kind::music}, {8U, media_kind::audiobook}, {5U, media_kind::other}, {0U, media_kind::unknown}}) {
            for (unsigned byte = 0; byte < 4; ++byte) media_bytes[track_offset + 208 + byte] = static_cast<std::byte>((raw >> (byte * 8)) & 0xffU);
            const auto view = inspect(c, {media_bytes});
            require(view.status == state::ready_read_only && view.tracks[0].kind == expected, "media kind was guessed or misclassified");
        }

        auto backend = std::make_unique<simulated_backend>();
        auto* simulation = backend.get();
        discovery d(std::move(backend));
        d.start([] {}); wait_ready(d);
        auto old = d.current().devices.at(0);
        require(d.is_current(old->token, old->generation, old->revision), "fresh snapshot invalid");
        d.refresh();
        require(!d.is_current(old->token, old->generation, old->revision), "refresh leaves old revision current");
        wait_ready(d);
        {
            std::lock_guard lock(simulation->mutex);
            simulation->block = true; simulation->entered = false;
        }
        d.refresh();
        {
            std::unique_lock lock(simulation->mutex);
            require(simulation->condition.wait_for(lock, std::chrono::seconds(2), [&] { return simulation->entered; }), "blocked read not entered");
            simulation->devices.clear();
        }
        simulation->callback();
        require(d.current().devices.empty(), "removal failed to invalidate promptly");
        wait_ready(d);
        require(d.current().devices.empty(), "late read resurrected removed device");
        {
            std::lock_guard lock(simulation->mutex);
            simulation->block = false;
            auto second = normal(); second.physical_key = "another-device"; second.volume_key = "same-reused-volume";
            simulation->devices = {normal(), second, normal()};
        }
        simulation->callback(); wait_ready(d);
        require(d.current().devices.size() == 2, "duplicate notifications or same-name devices conflated");
        require(d.current().devices[0]->generation != old->generation, "remount reused generation");
        require(d.current().devices[0]->token != d.current().devices[1]->token, "same-name devices share token");
        d.stop();
        require(d.current().stopped && d.current().devices.empty(), "shutdown retained live device");
        isolated_discovery_cases();
        recovery_association_cases();
        std::cout << "Device registry, classification, paths, signatures and lifecycle passed.\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

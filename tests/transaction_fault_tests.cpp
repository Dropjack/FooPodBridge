#include "database_test_support.h"
#include "foopodbridge/core/transaction/database_adapter.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <set>

using namespace foopodbridge::core;
using namespace foopodbridge::core::transaction;
using foopodbridge::tests::require;
namespace {
struct crash {};
struct injector {
    std::size_t count{}, stop{};
    bool after{}, io_error{}, short_write{};
    std::function<void(const std::string&)> observe;
    void before(const std::string& event) {
        ++count;
        if (observe) observe(event);
        if (stop == count && !after) { if (io_error) throw failure("injected_io"); throw crash{}; }
    }
    void done() { if (stop == count && after) { if (io_error) throw failure("injected_io"); throw crash{}; } }
};
class memory final : public filesystem {
public:
    std::map<std::string, bytes> files;
    injector* faults{};
    std::uint64_t free_bytes{1ULL << 40};
    std::size_t audio_written{}, audio_read{};
    std::optional<std::uint64_t> size(const std::string& p) override { const auto it = files.find(p); return it == files.end() ? std::nullopt : std::optional<std::uint64_t>(it->second.size()); }
    std::size_t read(const std::string& p, std::uint64_t offset, std::span<std::byte> output) override {
        auto it = files.find(p); if (it == files.end() || offset > it->second.size()) throw failure("read");
        const auto count = std::min(output.size(), it->second.size() - static_cast<std::size_t>(offset));
        std::copy_n(it->second.begin() + static_cast<std::ptrdiff_t>(offset), count, output.begin());
        if (p.find("Music/") != std::string::npos) audio_read += count;
        return count;
    }
    void create(const std::string& p) override { before("create:" + p); if (!safe_path(p) || files.contains(p)) throw failure("exists"); files[p] = {}; done(); }
    void append(const std::string& p, std::span<const std::byte> data) override {
        before("append:" + p); if (!files.contains(p)) throw failure("missing");
        auto count = data.size(); if (faults && faults->short_write && count) --count;
        files[p].insert(files[p].end(), data.begin(), data.begin() + static_cast<std::ptrdiff_t>(count));
        if (p.find("Music/") != std::string::npos) audio_written += count;
        done();
    }
    void flush(const std::string& p) override { before("flush:" + p); if (!files.contains(p)) throw failure("missing"); done(); }
    void rename(const std::string& a, const std::string& b) override {
        before("rename:" + a); if (!files.contains(a) || files.contains(b)) throw failure("rename");
        files[b] = files.at(a); files.erase(a); done();
    }
    void remove(const std::string& p) override { before("remove:" + p); files.erase(p); done(); }
    std::uint64_t available() override { return free_bytes; }
    std::vector<std::string> list(const std::string& directory) override {
        std::vector<std::string> out;
        for (const auto& [path, data] : files) {
            (void)data;
            if (path.rfind(directory + "/", 0) == 0) {
                const auto name = path.substr(directory.size() + 1);
                if (name.find('/') == std::string::npos) out.push_back(name);
            }
        }
        return out;
    }
private:
    void before(const std::string& event) { if (faults) faults->before(event); }
    void done() { if (faults) faults->done(); }
};
bytes db(bool track) {
    auto document = foopodbridge::tests::make_empty();
    if (track) {
        database::edit_plan edits;
        edits.operations.push_back(database::add_track{":iPod_Control:Music:F00:song.mp3", "Fixture", "", ""});
        auto edited = database::editor{}.apply(document, edits, foopodbridge::tests::fixed_generation());
        require(edited.has_value(), "fixture edit"); document = edited.value();
    }
    const auto output = database::writer{}.write(document, database::traditional_unsigned_profile(), foopodbridge::tests::fixed_generation());
    require(output.has_value(), "fixture DB"); return output.value();
}
const std::string formal = "iPod_Control/iTunes/iTunesDB";
const std::string media = "iPod_Control/Music/F00/song.mp3";
const auto empty_db = db(false), music_db = db(true);
const bytes audio(32768, std::byte{0x31});
struct fixture {
    memory device, host, source;
    identity current{"fixture-device", 1, 1, true};
    request input;
    bool cancel{};
    std::function<void(phase)> progress = [](phase) {};
    explicit fixture(int kind) {
        source.files["song.mp3"] = audio;
        input.id = "test-operation"; input.device = current; input.database_path = formal;
        input.new_database = kind == 1 ? empty_db : music_db;
        if (kind != 2) device.files[formal] = kind == 1 ? music_db : empty_db;
        if (kind == 1) { device.files[media] = audio; input.deletions.push_back({media, sha256(audio)}); }
        else input.additions.push_back({"song.mp3", media, audio.size(), sha256(audio)});
        if (kind == 2) input.kind = operation::initialize_library;
    }
    engine service() {
        callbacks c; c.current_identity = [&] { return current; }; c.cancelled = [&] { return cancel; };
        c.progress = [&](phase p) { progress(p); };
        return engine(device, host, source, traditional_database_validator(database::traditional_unsigned_profile()), c);
    }
    void invariant(int kind) {
        if (!device.files.contains(formal)) {
            require(kind == 2 || device.files.contains(formal + ".fpb-test-operation.old"), "old DB lost");
        } else {
            const auto& current_db = device.files.at(formal);
            require(current_db == empty_db || current_db == music_db, "invalid formal DB");
            if (current_db == music_db) require(device.files.contains(media) && device.files.at(media) == audio, "DB references missing media");
        }
        if (kind == 1 && (!device.files.contains(formal) || device.files.at(formal) != empty_db))
            require(device.files.contains(media), "audio deleted before DB commit");
    }
};
void expect_rejected(const std::function<void()>& run, const char* message) {
    bool rejected = false; try { run(); } catch (const failure&) { rejected = true; } require(rejected, message);
}
}
int main() {
    try {
        std::size_t injected{}, recovery_injected{};
        for (int kind = 0; kind != 3; ++kind) {
            fixture normal(kind); injector trace;
            normal.device.faults = &trace; normal.host.faults = &trace;
            auto service = normal.service();
            const auto completed = service.execute(service.prepare(normal.input));
            require(completed.status == outcome::completed, "normal fixture failed");
            normal.invariant(kind);
            if (kind != 1) require(normal.device.audio_written == audio.size() && normal.device.audio_read == 0, "normal media duplicated or read back");
            const auto steps = trace.count;
            for (bool after : {false, true}) for (std::size_t step = 1; step <= steps; ++step) {
                fixture f(kind); injector fault; fault.stop = step; fault.after = after;
                f.device.faults = &fault; f.host.faults = &fault;
                auto run = f.service(); const auto plan = run.prepare(f.input);
                try { (void)run.execute(plan); } catch (const crash&) {}
                ++injected; f.device.faults = nullptr; f.host.faults = nullptr; f.invariant(kind);
                if (f.host.files.contains("transactions/fixture-device/test-operation/manifest")) {
                    const auto crashed_device = f.device.files, crashed_host = f.host.files;
                    injector recovery_trace; f.device.faults = &recovery_trace; f.host.faults = &recovery_trace;
                    auto fresh = f.service(); const auto recovered = fresh.recover(f.current.device, f.input.id);
                    f.device.faults = nullptr; f.host.faults = nullptr; f.invariant(kind);
                    const auto once = f.device.files;
                    if (recovered.status == outcome::completed) {
                        require(fresh.recover(f.current.device, f.input.id).status == outcome::completed, "recovery repeat failed");
                        require(f.device.files == once, "repeated recovery mutated completed transaction");
                    }
                    for (std::size_t recovery_step = 1; recovery_step <= recovery_trace.count; ++recovery_step) {
                        f.device.files = crashed_device; f.host.files = crashed_host;
                        injector again; again.stop = recovery_step; again.after = true;
                        f.device.faults = &again; f.host.faults = &again;
                        auto recovery = f.service(); try { (void)recovery.recover(f.current.device, f.input.id); } catch (const crash&) {}
                        ++recovery_injected; f.device.faults = nullptr; f.host.faults = nullptr;
                        f.invariant(kind); (void)recovery.recover(f.current.device, f.input.id); f.invariant(kind);
                    }
                }
            }
        }
        for (const auto p : {phase::preparing, phase::copying, phase::staging_database, phase::committing, phase::cleaning}) {
            fixture f(0); f.progress = [&](phase current) { if (current == p) f.cancel = true; };
            auto service = f.service(); const auto r = service.execute(service.prepare(f.input));
            f.invariant(0);
            require(p == phase::cleaning ? r.committed : !r.committed, "cancel commit semantics");
        }
        {
            fixture f(2); f.progress = [&](phase p) { if (p == phase::committing) f.cancel = true; };
            auto service = f.service(); const auto r = service.execute(service.prepare(f.input));
            require(r.status == outcome::cancelled && !f.device.files.contains(formal), "cancel initialized DB");
            require(!service.recover(f.current.device, f.input.id).committed && !f.device.files.contains(formal), "recovery committed cancelled initialization");
        }
        {
            fixture f(0); injector fault; fault.observe = [&](const std::string& event) { if (event.rfind("rename:", 0) == 0) f.cancel = true; };
            f.device.faults = &fault; auto service = f.service(); const auto r = service.execute(service.prepare(f.input));
            require(!r.committed, "media rename cancellation should stop before DB"); f.invariant(0);
        }
        {
            fixture f(0); auto service = f.service(); const auto p = service.prepare(f.input);
            ++f.current.generation; require(service.execute(p).status == outcome::failed, "stale identity accepted");
            require(f.host.files.empty() && f.device.files.size() == 1, "identity failure wrote files");
        }
        {
            fixture f(0); auto service = f.service(); f.device.free_bytes = 1;
            expect_rejected([&] { (void)service.prepare(f.input); }, "space preflight");
        }
        {
            fixture f(0); injector fault; fault.short_write = true; f.device.faults = &fault;
            auto service = f.service(); const auto r = service.execute(service.prepare(f.input));
            require(!r.committed, "short write committed"); f.invariant(0);
        }
        {
            fixture f(2); f.device.files[formal] = empty_db; auto service = f.service();
            expect_rejected([&] { (void)service.prepare(f.input); }, "existing DB initialized");
        }
        {
            fixture f(0); auto service = f.service(); const auto p = service.prepare(f.input);
            outcome nested = outcome::failed; f.progress = [&](phase stage) { if (stage == phase::copying) nested = service.execute(p).status; };
            require(service.execute(p).status == outcome::completed && nested == outcome::busy, "global write serialization");
        }
        {
            fixture f(0); f.device.files[formal][0] ^= std::byte{1}; auto service = f.service();
            expect_rejected([&] { (void)service.prepare(f.input); }, "corrupt database accepted");
        }
        {
            fixture f(0); auto service = f.service(); f.input.new_database = empty_db; f.input.additions.clear();
            require(service.execute(service.prepare(f.input)).code == "no_changes" && f.host.files.empty(), "no-op wrote files");
        }
        {
            expect_rejected([] { (void)traditional_database_validator(database::traditional_preserve_only_profile()); }, "preserve-only profile allowed");
            expect_rejected([] { (void)traditional_database_validator(database::traditional_hash58_profile()); }, "unsigned hash58 allowed");
        }
        {
            fixture f(0); injector fault;
            fault.observe = [&](const std::string& event) { if (event == "append:" + media + ".fpb-test-operation.tmp") ++f.current.generation; };
            f.device.faults = &fault; auto service = f.service(); const auto r = service.execute(service.prepare(f.input));
            require(r.status == outcome::recovery_required && f.device.files.at(formal) == empty_db, "mid-copy identity ignored");
            const auto stopped = fault.count; f.device.faults = nullptr;
            require(!service.recover("wrong-device", f.input.id).committed && fault.count == stopped, "wrong-device recovery");
            require(f.device.files.at(formal) == empty_db, "identity recovery modified DB");
        }
        {
            fixture f(0); injector fault;
            fault.observe = [&](const std::string& event) { if (event == "rename:" + formal) f.cancel = true; };
            f.device.faults = &fault; auto service = f.service(); const auto r = service.execute(service.prepare(f.input));
            require(r.committed && r.cancel_deferred && r.status == outcome::completed, "commit cancellation was not deferred");
        }
        for (const auto* event_prefix : {"create:", "append:", "flush:", "rename:"}) {
            fixture f(0); injector fault; fault.io_error = true;
            fault.observe = [&](const std::string& event) { if (event.rfind(event_prefix, 0) == 0) fault.stop = fault.count; };
            f.device.faults = &fault; auto service = f.service(); const auto r = service.execute(service.prepare(f.input));
            require(!r.committed, "I/O error committed"); f.invariant(0);
        }
        {
            fixture f(1); injector fault; fault.io_error = true;
            fault.observe = [&](const std::string& event) { if (event == "remove:" + media) fault.stop = fault.count; };
            f.device.faults = &fault; auto service = f.service(); const auto r = service.execute(service.prepare(f.input));
            require(r.committed && r.status == outcome::cleaning_required && f.device.files.contains(media), "cleanup failure rolled back");
        }
        {
            fixture f(0); injector fault;
            fault.observe = [&](const std::string& event) { fault.short_write = event == "append:" + media + ".fpb-test-operation.tmp"; };
            f.device.faults = &fault; auto service = f.service(); const auto r = service.execute(service.prepare(f.input));
            require(!r.committed && r.code == "media_short_write", "media short write was not detected"); f.invariant(0);
        }
        {
            fixture f(0); auto service = f.service(); require(service.execute(service.prepare(f.input)).committed, "record corruption fixture");
            f.host.files.at("transactions/fixture-device/test-operation/manifest")[0] ^= std::byte{1};
            const auto saved = f.device.files;
            require(service.recover(f.current.device, f.input.id).status == outcome::recovery_required && f.device.files == saved, "corrupt record trusted");
        }
        std::cout << "Fault matrix: " << injected << " before/after interruptions, " << recovery_injected
                  << " recovery interruptions; cancellation, identity, space, short write, initialization, concurrency and corruption passed.\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

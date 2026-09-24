// SPDX-License-Identifier: LGPL-3.0-or-later
#include "foobar/device_service.h"
#include "foopodbridge/core/device/device.h"
#include <algorithm>
#include <atomic>
#include <mutex>
#include <sstream>

namespace foopodbridge::adapter {
namespace {
namespace c = contract;
namespace d = core::device;
namespace db = core::database;
struct listener {
    std::atomic<bool> active{true};
    c::device_event_callback_v1::ptr callback;
};
struct shared_state {
    std::mutex mutex;
    std::shared_ptr<d::discovery> engine;
    std::vector<std::weak_ptr<listener>> listeners;
    std::atomic<bool> running{};
    std::atomic<bool> notification_pending{};
    std::string startup_error;
};
std::shared_ptr<shared_state> state() {
    static auto value = std::make_shared<shared_state>();
    return value;
}
std::shared_ptr<d::discovery> engine() {
    auto s = state(); std::lock_guard lock(s->mutex); return s->engine;
}
void deliver(const std::shared_ptr<shared_state>& s, bool unavailable) {
    std::vector<std::shared_ptr<listener>> listeners;
    std::uint64_t revision{};
    {
        std::lock_guard lock(s->mutex);
        if (s->engine) revision = s->engine->current().revision;
        std::erase_if(s->listeners, [&](const auto& weak) {
            auto item = weak.lock();
            if (!item || !item->active) return true;
            listeners.push_back(std::move(item)); return false;
        });
    }
    for (const auto& item : listeners) if (item->active && item->callback.is_valid()) {
        if (unavailable) item->callback->on_provider_unavailable();
        else item->callback->on_snapshot_generation_changed(revision);
    }
}
void post_notification(const std::weak_ptr<shared_state>& weak) {
    const auto s = weak.lock();
    if (!s || !s->running || s->notification_pending.exchange(true)) return;
    try {
        fb2k::inMainThread([weak] {
            if (const auto current = weak.lock()) {
                current->notification_pending = false;
                if (current->running) deliver(current, false);
            }
        });
    } catch (...) { s->notification_pending = false; }
}
class subscription_impl : public c::subscription_v1 {
public:
    explicit subscription_impl(std::shared_ptr<listener> item) : item_(std::move(item)) {}
    ~subscription_impl() { cancel(); }
    void cancel() noexcept override { item_->active = false; }
private:
    std::shared_ptr<listener> item_;
};
class capabilities_impl : public c::capability_matrix_v1 {
public:
    explicit capabilities_impl(std::shared_ptr<const d::snapshot> value) : value_(std::move(value)) {}
    c::device_kind get_device_kind() noexcept override {
        if (value_->identity.model_id == "photo") return c::device_kind::ipod_photo;
        if (value_->identity.model_id.starts_with("classic") || value_->identity.name.starts_with("iPod Classic")) return c::device_kind::ipod_classic;
        return c::device_kind::unknown;
    }
    std::uint64_t get_capability_flags() noexcept override {
        return value_->status == d::state::ready_read_only ? static_cast<std::uint64_t>(c::capability::read_library) : 0;
    }
    bool is_writable() noexcept override { return false; }
private:
    std::shared_ptr<const d::snapshot> value_;
};
class library_impl : public c::library_snapshot_v1 {
public:
    explicit library_impl(std::shared_ptr<const d::snapshot> value) : value_(std::move(value)) {}
    std::uint32_t get_track_count() noexcept override { return static_cast<std::uint32_t>(value_->tracks.size()); }
    bool get_track(std::uint32_t index, std::uint32_t& id, std::uint64_t& pid, c::read_media_kind& kind) noexcept override {
        id = 0; pid = 0; kind = c::read_media_kind::unknown;
        if (index >= value_->tracks.size()) return false;
        const auto& t = value_->tracks[index]; id = t.metadata.id; pid = t.metadata.persistent_id.value_or(0);
        kind = static_cast<c::read_media_kind>(t.kind); return true;
    }
    bool get_track_text(std::uint32_t index, c::track_text field, pfc::string_base& out) override {
        out.reset(); if (index >= value_->tracks.size()) return false;
        const auto& t = value_->tracks[index];
        switch (field) {
        case c::track_text::title: out = t.metadata.title.c_str(); break;
        case c::track_text::artist: out = t.metadata.artist.c_str(); break;
        case c::track_text::album: out = t.metadata.album.c_str(); break;
        case c::track_text::relative_path: out = t.relative_path.c_str(); break;
        default: return false;
        }
        return true;
    }
    std::uint32_t get_playlist_count() noexcept override { return static_cast<std::uint32_t>(value_->playlists.size() + (value_->master ? 1 : 0)); }
    const db::playlist* playlist_at(std::uint32_t index) const {
        if (value_->master) { if (index == 0) return &*value_->master; --index; }
        return index < value_->playlists.size() ? &value_->playlists[index] : nullptr;
    }
    bool get_playlist(std::uint32_t index, std::uint64_t& id, c::read_playlist_kind& kind, std::uint32_t& count, pfc::string_base& name) override {
        id = 0; count = 0; kind = c::read_playlist_kind::opaque; name.reset();
        const auto* p = playlist_at(index); if (!p) return false;
        id = p->persistent_id; kind = static_cast<c::read_playlist_kind>(p->kind);
        count = static_cast<std::uint32_t>(p->track_ids.size()); name = p->name.c_str(); return true;
    }
    bool get_playlist_member(std::uint32_t playlist, std::uint32_t member, std::uint32_t& id) noexcept override {
        id = 0; const auto* p = playlist_at(playlist); if (!p || member >= p->track_ids.size()) return false;
        id = p->track_ids[member]; return true;
    }
private:
    std::shared_ptr<const d::snapshot> value_;
};
class snapshot_impl : public c::device_snapshot_readonly_v1 {
public:
    explicit snapshot_impl(std::shared_ptr<const d::snapshot> value) : value_(std::move(value)) {}
    void get_stable_id(pfc::string_base& out) override { out = value_->token.c_str(); }
    void get_display_name(pfc::string_base& out) override { out = value_->identity.name.c_str(); }
    std::uint64_t get_generation() noexcept override { return value_->generation; }
    c::device_state get_state() noexcept override {
        switch (value_->status) {
        case d::state::ready_read_only: return c::device_state::read_only;
        case d::state::database_corrupt: case d::state::read_error: case d::state::recovery_required: return c::device_state::error;
        default: return c::device_state::unsupported;
        }
    }
    void get_capabilities(c::capability_matrix_v1::ptr& out) noexcept override {
        try { out = new service_impl_t<capabilities_impl>(value_); } catch (...) { out.release(); }
    }
    std::uint64_t get_revision() noexcept override { return value_->revision; }
    c::read_state get_read_state() noexcept override { return static_cast<c::read_state>(value_->status); }
    c::read_evidence get_evidence() noexcept override { return static_cast<c::read_evidence>(value_->level); }
    bool has_stable_identity() noexcept override { return value_->identity_complete; }
    bool get_capacity(std::uint64_t& total, std::uint64_t& available) noexcept override {
        total = value_->capacity; available = value_->available; return value_->capacity_known;
    }
    void get_profile(pfc::string_base& out) override { out = value_->profile.c_str(); }
    void get_status_description(pfc::string_base& out) override {
        std::ostringstream text;
        text << d::state_name(value_->status) << "\r\n" << d::reason_text(value_->problem)
            << "\r\nProfile: " << (value_->profile.empty() ? "Not available" : value_->profile)
            << "\r\nModel evidence: " << (value_->level == d::evidence::unknown ? "Unknown" : "StructureKnown (reference-backed)")
            << "\r\nIdentity: " << (value_->identity_complete ? "Available (private)" : "Incomplete; current mount only")
            << "\r\nSignature: ";
        switch (value_->signature) {
        case db::hash58_signature_status::valid: text << "Verified"; break;
        case db::hash58_signature_status::invalid: text << "Invalid"; break;
        case db::hash58_signature_status::not_checked: text << "Not checked (identity unavailable)"; break;
        default: text << "Not applicable / not read"; break;
        }
        text << "\r\nArtwork database: " << (value_->artwork_present ? "Present (decoding pending)" : "Not observed")
            << "\r\nDevice writes: Disabled\r\nPending recovery records: " << value_->recovery_pending
            << "\r\nInvalid recovery records: " << value_->recovery_invalid;
        text << "\r\nComputer recovery records: ";
        switch (value_->recovery.link) {
        case d::recovery_link::identity_unavailable: text << "Stable identity unavailable"; break;
        case d::recovery_link::repository_missing: text << "No repository yet"; break;
        case d::recovery_link::unavailable: text << "Could not inspect; refresh to retry"; break;
        case d::recovery_link::available:
            text << value_->recovery.pending << " pending, " << value_->recovery.invalid << " invalid"
                << "\r\nDatabase snapshots: " << value_->recovery.snapshots
                << "\r\nLast Known Good snapshots: " << value_->recovery.last_known_good
                << "\r\nSaved external backup locations: " << value_->recovery.backups
                << " (must be reverified before use)";
            break;
        }
        out = text.str().c_str();
    }
    void get_mount_root(pfc::string_base& out) override { out = value_->mount_root.c_str(); }
    bool get_library(c::library_snapshot_v1::ptr& out) noexcept override {
        out.release(); if (value_->status != d::state::ready_read_only) return false;
        try { out = new service_impl_t<library_impl>(value_); return true; } catch (...) { return false; }
    }
private:
    std::shared_ptr<const d::snapshot> value_;
};
class list_impl : public c::device_snapshot_list_v1 {
public:
    explicit list_impl(d::catalog value) : value_(std::move(value)) {}
    std::uint32_t get_count() noexcept override { return static_cast<std::uint32_t>(value_.devices.size()); }
    bool get_item(std::uint32_t index, c::device_snapshot_v1::ptr& out) noexcept override {
        out.release(); if (index >= value_.devices.size()) return false;
        try { out = new service_impl_t<snapshot_impl>(value_.devices[index]); return true; } catch (...) { return false; }
    }
private:
    d::catalog value_;
};
class rejected_result : public c::operation_result_v1 {
public:
    c::operation_result_code get_code() noexcept override { return c::operation_result_code::unsupported; }
    void get_operation_id(pfc::string_base& out) override { out = "read-only-rejection"; }
    void get_message(pfc::string_base& out) override { out = "Device writes are disabled in this read-only provider."; }
};
class rejected_operation : public c::operation_handle_v1 {
public:
    void get_operation_id(pfc::string_base& out) override { out = "read-only-rejection"; }
    c::operation_state get_state() noexcept override { return c::operation_state::failed; }
    std::uint32_t get_progress_basis_points() noexcept override { return 0; }
    bool request_cancel() noexcept override { return false; }
    bool get_plan(c::operation_plan_v1::ptr& out) noexcept override { out.release(); return false; }
    bool get_result(c::operation_result_v1::ptr& out) noexcept override {
        try { out = new service_impl_t<rejected_result>(); return true; } catch (...) { out.release(); return false; }
    }
};
class provider_impl : public c::device_provider_readonly_v1 {
public:
    void get_snapshots(c::device_snapshot_list_v1::ptr& out) noexcept override {
        try { const auto e = engine(); out = new service_impl_t<list_impl>(e ? e->current() : d::catalog{}); } catch (...) { out.release(); }
    }
    bool subscribe(c::device_event_callback_v1::ptr callback, c::subscription_v1::ptr& out) noexcept override {
        out.release(); if (!callback.is_valid()) return false;
        try {
            auto item = std::make_shared<listener>(); item->callback = callback;
            auto s = state(); std::lock_guard lock(s->mutex);
            s->listeners.push_back(item); out = new service_impl_t<subscription_impl>(std::move(item)); return true;
        } catch (...) { return false; }
    }
    bool begin_plan(c::operation_request_v1::ptr, c::operation_handle_v1::ptr& out) noexcept override { return reject(out); }
    bool execute_plan(c::operation_plan_v1::ptr, c::operation_handle_v1::ptr& out) noexcept override { return reject(out); }
    bool refresh() noexcept override {
        try { const auto e = engine(); if (!e || !running()) return false; e->refresh(); return true; } catch (...) { return false; }
    }
    bool is_scanning() noexcept override { try { const auto e = engine(); return e && e->current().scanning; } catch (...) { return false; } }
    bool is_current(const char* token, std::uint64_t generation, std::uint64_t revision) noexcept override {
        if (!token) return false;
        try { const auto e = engine(); return e && e->is_current(token, generation, revision); } catch (...) { return false; }
    }
    void get_provider_status(pfc::string_base& out) override {
        const auto s = state(); std::lock_guard lock(s->mutex);
        if (!s->startup_error.empty()) { out = s->startup_error.c_str(); return; }
        if (!s->running || !s->engine) { out = "Device service is not running."; return; }
        const auto current = s->engine->current();
        out = current.scanning ? "Reading devices..." : current.problem != d::reason::none ? d::reason_text(current.problem)
            : current.devices.empty() ? "No iPod devices detected." : "Read-only discovery complete.";
    }
private:
    static bool reject(c::operation_handle_v1::ptr& out) noexcept {
        try { out = new service_impl_t<rejected_operation>(); } catch (...) { out.release(); }
        return false;
    }
};
class lifecycle : public initquit {
public:
    void on_init() override {
        auto s = state();
        try {
            const auto profile = filesystem::g_get_native_path(core_api::get_profile_path());
            const std::string native(profile.c_str());
            const auto root = std::filesystem::path(std::u8string(native.begin(), native.end())) / L"FooPodBridge" / L"recovery";
            auto e = std::make_shared<d::discovery>(d::make_windows_backend(), root);
            { std::lock_guard lock(s->mutex); s->engine = e; s->running = true; }
            e->start([weak = std::weak_ptr<shared_state>(s)] { post_notification(weak); });
        } catch (...) {
            auto e = engine(); if (e) e->stop();
            std::lock_guard lock(s->mutex); s->running = false; s->startup_error = "Device discovery could not start. Restart to retry.";
        }
    }
    void on_quit() override {
        auto s = state(); s->running = false;
        auto e = engine(); if (e) e->stop();
        deliver(s, true);
        std::lock_guard lock(s->mutex); s->engine.reset(); s->listeners.clear();
    }
};
initquit_factory_t<lifecycle> lifecycle_factory;
}
bool running() noexcept { return state()->running; }
c::device_provider_readonly_v1::ptr provider() {
    static c::device_provider_readonly_v1::ptr instance = new service_impl_t<provider_impl>();
    return instance;
}
} // namespace foopodbridge::adapter

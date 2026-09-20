// SPDX-License-Identifier: LGPL-3.0-or-later
#include "foobar/device_service.h"
#include "foobar/resource.h"
#include <memory>
#include <sstream>
#include <vector>

namespace {
namespace c = foopodbridge::contract;
constexpr UINT updated = WM_APP + 84;
constexpr GUID page_guid{0xe74f4dea, 0x8e6a, 0x423b, {0x8b, 0x19, 0x42, 0x60, 0x7a, 0xa6, 0x28, 0xb1}};
struct window_lifetime { HWND hwnd{}; };
std::wstring wide(std::string_view text) {
    if (text.empty()) return {};
    const auto size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) return L"Text unavailable";
    std::wstring out(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), out.data(), size);
    return out;
}
void label(HWND hwnd, int control, const char* text) { SetDlgItemTextW(hwnd, control, wide(text).c_str()); }
class page_callback : public c::device_event_callback_v1 {
public:
    explicit page_callback(std::weak_ptr<window_lifetime> target) : target_(std::move(target)) {}
    void on_snapshot_generation_changed(std::uint64_t) noexcept override { notify(); }
    void on_provider_unavailable() noexcept override { notify(); }
private:
    void notify() { if (auto p = target_.lock(); p && p->hwnd) PostMessageW(p->hwnd, updated, 0, 0); }
    std::weak_ptr<window_lifetime> target_;
};
class page_instance : public preferences_page_instance {
public:
    explicit page_instance(HWND parent) : lifetime_(std::make_shared<window_lifetime>()) {
        provider_ = foopodbridge::adapter::provider();
        const auto hwnd = CreateDialogParamW(core_api::get_my_instance(), MAKEINTRESOURCEW(IDD_DEVICE_INFO), parent, &dialog, reinterpret_cast<LPARAM>(this));
        if (!hwnd) throw std::runtime_error("device information page could not be created");
    }
    ~page_instance() {
        if (subscription_.is_valid()) subscription_->cancel();
        if (lifetime_->hwnd) DestroyWindow(lifetime_->hwnd);
    }
    t_uint32 get_state() override { return 0; }
    HWND get_wnd() override { return lifetime_->hwnd; }
    void apply() override {}
    void reset() override {}
private:
    static INT_PTR CALLBACK dialog(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        auto* self = reinterpret_cast<page_instance*>(GetWindowLongPtrW(hwnd, DWLP_USER));
        if (msg == WM_INITDIALOG) {
            self = reinterpret_cast<page_instance*>(lp);
            SetWindowLongPtrW(hwnd, DWLP_USER, reinterpret_cast<LONG_PTR>(self)); self->lifetime_->hwnd = hwnd;
        }
        if (!self) return FALSE;
        try { return self->message(hwnd, msg, wp); }
        catch (...) { label(hwnd, IDC_PROVIDER_STATUS, "Device information is unavailable. Refresh to retry."); return TRUE; }
    }
    INT_PTR message(HWND hwnd, UINT msg, WPARAM wp) {
        if (msg == WM_INITDIALOG) {
            provider_->subscribe(new service_impl_t<page_callback>(lifetime_), subscription_);
            refresh_view(); return TRUE;
        }
        if (msg == WM_NCDESTROY) {
            if (subscription_.is_valid()) subscription_->cancel();
            lifetime_->hwnd = nullptr; SetWindowLongPtrW(hwnd, DWLP_USER, 0); return FALSE;
        }
        if (msg == updated) { refresh_view(); return TRUE; }
        if (msg == WM_COMMAND) {
            if (LOWORD(wp) == IDC_DEVICE_REFRESH && HIWORD(wp) == BN_CLICKED) {
                provider_->refresh(); refresh_view(); return TRUE;
            }
            if (LOWORD(wp) == IDC_DEVICE_LIST && HIWORD(wp) == CBN_SELCHANGE) { details(); return TRUE; }
        }
        return FALSE;
    }
    void refresh_view() {
        const auto hwnd = lifetime_->hwnd;
        pfc::string8 selected;
        const auto index = SendDlgItemMessageW(hwnd, IDC_DEVICE_LIST, CB_GETCURSEL, 0, 0);
        if (index >= 0 && static_cast<std::size_t>(index) < devices_.size()) devices_[static_cast<std::size_t>(index)]->get_stable_id(selected);
        SendDlgItemMessageW(hwnd, IDC_DEVICE_LIST, CB_RESETCONTENT, 0, 0); devices_.clear();
        c::device_snapshot_list_v1::ptr list; provider_->get_snapshots(list);
        int selection = 0;
        if (list.is_valid()) for (std::uint32_t i = 0; i < list->get_count(); ++i) {
            c::device_snapshot_v1::ptr base; c::device_snapshot_readonly_v1::ptr item;
            if (!list->get_item(i, base) || !base->service_query_t(item)) continue;
            pfc::string8 name, id; item->get_display_name(name); item->get_stable_id(id);
            if (selected == id) selection = static_cast<int>(devices_.size());
            std::string text = std::to_string(devices_.size() + 1) + ". " + name.c_str();
            SendDlgItemMessageW(hwnd, IDC_DEVICE_LIST, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(wide(text).c_str()));
            devices_.push_back(item);
        }
        SendDlgItemMessageW(hwnd, IDC_DEVICE_LIST, CB_SETCURSEL, selection, 0);
        EnableWindow(GetDlgItem(hwnd, IDC_DEVICE_LIST), !devices_.empty());
        EnableWindow(GetDlgItem(hwnd, IDC_DEVICE_REFRESH), !provider_->is_scanning());
        pfc::string8 status; provider_->get_provider_status(status); label(hwnd, IDC_PROVIDER_STATUS, status.c_str());
        details();
    }
    void details() {
        const auto hwnd = lifetime_->hwnd;
        const auto index = SendDlgItemMessageW(hwnd, IDC_DEVICE_LIST, CB_GETCURSEL, 0, 0);
        if (index < 0 || static_cast<std::size_t>(index) >= devices_.size()) {
            label(hwnd, IDC_DEVICE_DETAILS, "Connect an iPod that Windows exposes as a storage volume.\r\n\r\nDiscovery is read-only. No device files are created or repaired.\r\nUse Windows Explorer to eject after reading completes."); return;
        }
        const auto& item = devices_[static_cast<std::size_t>(index)];
        pfc::string8 description; item->get_status_description(description);
        std::ostringstream text; text << description.c_str();
        std::uint64_t capacity{}, available{};
        if (item->get_capacity(capacity, available)) {
            text.setf(std::ios::fixed); text.precision(2);
            text << "\r\nCapacity: " << static_cast<double>(capacity) / 1073741824.0 << " GiB"
                << "    Free: " << static_cast<double>(available) / 1073741824.0 << " GiB";
        } else text << "\r\nCapacity: Not available";
        c::library_snapshot_v1::ptr library;
        if (item->get_library(library)) {
            unsigned music{}, books{}, other{}, unknown{}, unsafe{};
            for (std::uint32_t i = 0; i < library->get_track_count(); ++i) {
                std::uint32_t id{}; std::uint64_t pid{}; c::read_media_kind kind{};
                library->get_track(i, id, pid, kind);
                switch (kind) { case c::read_media_kind::music: ++music; break; case c::read_media_kind::audiobook: ++books; break;
                case c::read_media_kind::other: ++other; break; default: ++unknown; break; }
                pfc::string8 path; library->get_track_text(i, c::track_text::relative_path, path); if (path.is_empty()) ++unsafe;
            }
            unsigned normal{}, smart{}, opaque{}, master{};
            for (std::uint32_t i = 0; i < library->get_playlist_count(); ++i) {
                std::uint64_t id{}; c::read_playlist_kind kind{}; std::uint32_t count{}; pfc::string8 name;
                library->get_playlist(i, id, kind, count, name);
                switch (kind) { case c::read_playlist_kind::master: ++master; break; case c::read_playlist_kind::ordinary: ++normal; break;
                case c::read_playlist_kind::smart: ++smart; break; default: ++opaque; break; }
            }
            text << "\r\n\r\nLibrary tracks: " << library->get_track_count() << "\r\nMusic: " << music << "    Audiobooks: " << books
                << "    Other: " << other << "    Unknown: " << unknown
                << "\r\nPlaylists - Normal: " << normal << "    Smart: " << smart << "    Opaque: " << opaque << "    Master: " << master;
            if (unsafe) text << "\r\nTracks with unavailable/unsafe paths: " << unsafe;
        } else text << "\r\n\r\nLibrary: Not available (not an empty library).";
        text << "\r\nMount generation: " << item->get_generation() << "    Snapshot revision: " << item->get_revision();
        label(hwnd, IDC_DEVICE_DETAILS, text.str().c_str());
    }
    std::shared_ptr<window_lifetime> lifetime_;
    c::device_provider_readonly_v1::ptr provider_;
    c::subscription_v1::ptr subscription_;
    std::vector<c::device_snapshot_readonly_v1::ptr> devices_;
};
class preferences : public preferences_page_v3 {
public:
    const char* get_name() override { return "FooPodBridge"; }
    GUID get_guid() override { return page_guid; }
    GUID get_parent_guid() override { return preferences_page::guid_tools; }
    preferences_page_instance::ptr instantiate(HWND parent, preferences_page_callback::ptr) override {
        return new service_impl_t<page_instance>(parent);
    }
};
preferences_page_factory_t<preferences> page_factory;
}

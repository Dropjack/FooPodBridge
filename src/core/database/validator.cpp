#include "foopodbridge/core/database/validator.h"

#include "internal.h"

#include <algorithm>
#include <cstdint>
#include <unordered_set>

namespace foopodbridge::core::database {

result<validation_report> validator::validate(const database_document& document) const {
    using detail::make_error;
    if (!document.model.master_playlist.has_value()) {
        return result<validation_report>::failure(make_error(
            error_code::missing_master_playlist, 0U, "mhyp", "model/master", "master playlist is missing"));
    }
    if (document.model.master_playlist->kind != playlist_kind::master) {
        return result<validation_report>::failure(make_error(
            error_code::missing_master_playlist, 0U, "mhyp", "model/master", "master playlist has the wrong kind"));
    }
    if ((document.source_profile == profile_kind::traditional_unsigned ||
         document.source_profile == profile_kind::traditional_hash58) &&
        document.model.persistent_id == 0U) {
        return result<validation_report>::failure(make_error(
            error_code::duplicate_persistent_id, 0U, "mhbd", "model", "writable profile requires a nonzero database persistent ID"));
    }
    if (document.source_profile == profile_kind::traditional_hash58 &&
        document.hash58_status == hash58_signature_status::invalid) {
        return result<validation_report>::failure(make_error(
            error_code::hash58_mismatch, 0x58U, "mhbd", "model/hash58", "hash58 signature is invalid"));
    }

    std::unordered_set<std::uint32_t> track_ids;
    std::unordered_set<std::uint64_t> track_persistent_ids;
    for (const auto& value : document.model.tracks) {
        if (value.id == 0U || !track_ids.insert(value.id).second) {
            return result<validation_report>::failure(make_error(
                error_code::duplicate_track_id, 0U, "mhit", "model/tracks", "track IDs must be nonzero and unique"));
        }
        if (value.persistent_id.has_value() &&
            (*value.persistent_id == 0U || !track_persistent_ids.insert(*value.persistent_id).second)) {
            return result<validation_report>::failure(make_error(
                error_code::duplicate_persistent_id, 0U, "mhit", "model/tracks", "nonzero track persistent IDs must be unique"));
        }
    }

    std::unordered_set<std::uint64_t> playlist_persistent_ids;
    const auto master_id = document.model.master_playlist->persistent_id;
    if (master_id == 0U || !playlist_persistent_ids.insert(master_id).second) {
        return result<validation_report>::failure(make_error(
            error_code::duplicate_persistent_id, 0U, "mhyp", "model/master", "master persistent ID must be nonzero"));
    }
    std::unordered_set<std::uint32_t> master_members;
    for (const auto id : document.model.master_playlist->track_ids) {
        if (!track_ids.contains(id)) {
            return result<validation_report>::failure(make_error(
                error_code::dangling_track_reference, 0U, "mhip", "model/master/members", "master contains an unknown track ID"));
        }
        if (!master_members.insert(id).second) {
            return result<validation_report>::failure(make_error(
                error_code::duplicate_playlist_member, 0U, "mhip", "model/master/members", "master contains a duplicate track ID"));
        }
    }
    if (master_members.size() != track_ids.size()) {
        return result<validation_report>::failure(detail::make_error(
            error_code::dangling_track_reference, 0U, "mhyp", "model/master", "master does not contain every track exactly once"));
    }

    for (const auto& value : document.model.playlists) {
        if (value.kind == playlist_kind::master) {
            return result<validation_report>::failure(make_error(
                error_code::multiple_master_playlists, 0U, "mhyp", "model/playlists", "master playlist appears in the ordinary collection"));
        }
        if (value.persistent_id == 0U || !playlist_persistent_ids.insert(value.persistent_id).second) {
            return result<validation_report>::failure(make_error(
                error_code::duplicate_persistent_id, 0U, "mhyp", "model/playlists", "playlist persistent IDs must be nonzero and unique"));
        }
        if (value.kind == playlist_kind::ordinary) {
            for (const auto id : value.track_ids) {
                if (!track_ids.contains(id)) {
                    return result<validation_report>::failure(make_error(
                        error_code::dangling_track_reference, 0U, "mhip", "model/playlists/members", "ordinary playlist contains an unknown track ID"));
                }
            }
        }
    }

    return result<validation_report>::success(validation_report{document.diagnostics});
}

}  // namespace foopodbridge::core::database

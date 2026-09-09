#include "foopodbridge/core/database/edit.h"

#include "foopodbridge/core/database/validator.h"
#include "internal.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace foopodbridge::core::database {
namespace {

template <class... T>
struct overloaded : T... { using T::operator()...; };
template <class... T>
overloaded(T...) -> overloaded<T...>;

[[nodiscard]] database_error edit_error(error_code code, std::string summary) {
    return detail::make_error(code, 0U, {}, "edit", std::move(summary));
}

}  // namespace

result<database_document> editor::apply(
    const database_document& document,
    const edit_plan& plan,
    const generation_context& generation) const {
    const auto original_validation = validator{}.validate(document);
    if (!original_validation) {
        return result<database_document>::failure(original_validation.error());
    }
    if (plan.operations.empty()) {
        return result<database_document>::success(document);
    }
    if (document.source_profile != profile_kind::traditional_unsigned) {
        return result<database_document>::failure(edit_error(
            error_code::profile_not_writable, "preserve-only documents reject every semantic edit"));
    }
    if (document.has_opaque_dependency) {
        return result<database_document>::failure(edit_error(
            error_code::opaque_dependency, "an opaque dataset may depend on the requested edit"));
    }

    database_document updated = document;
    std::size_t generated_index{};
    std::unordered_set<std::uint64_t> persistent_ids;
    persistent_ids.insert(updated.model.persistent_id);
    if (updated.model.master_playlist.has_value()) {
        persistent_ids.insert(updated.model.master_playlist->persistent_id);
    }
    for (const auto& value : updated.model.tracks) {
        if (value.persistent_id.has_value()) {
            persistent_ids.insert(*value.persistent_id);
        }
    }
    for (const auto& value : updated.model.playlists) {
        persistent_ids.insert(value.persistent_id);
    }
    const auto next_persistent_id = [&]() -> result<std::uint64_t> {
        if (generated_index >= generation.new_persistent_ids.size()) {
            return result<std::uint64_t>::failure(edit_error(
                error_code::id_exhausted, "GenerationContext has no remaining persistent ID"));
        }
        const auto id = generation.new_persistent_ids[generated_index++];
        if (id == 0U || !persistent_ids.insert(id).second) {
            return result<std::uint64_t>::failure(edit_error(
                error_code::duplicate_persistent_id, "GenerationContext produced a zero or duplicate persistent ID"));
        }
        return result<std::uint64_t>::success(id);
    };

    for (const auto& operation : plan.operations) {
        auto operation_result = std::visit(overloaded{
            [&](const add_track& value) -> result<void> {
                std::uint32_t maximum_id{};
                for (const auto& existing : updated.model.tracks) {
                    maximum_id = std::max(maximum_id, existing.id);
                }
                if (maximum_id == std::numeric_limits<std::uint32_t>::max()) {
                    return result<void>::failure(edit_error(error_code::id_exhausted, "track ID space is exhausted"));
                }
                auto persistent_id = next_persistent_id();
                if (!persistent_id) {
                    return result<void>::failure(persistent_id.error());
                }
                track added;
                added.id = maximum_id + 1U;
                added.persistent_id = persistent_id.value();
                added.location = value.location;
                added.title = value.title;
                added.artist = value.artist;
                added.album = value.album;
                updated.model.tracks.push_back(std::move(added));
                updated.model.master_playlist->track_ids.push_back(maximum_id + 1U);
                updated.requested_changes.push_back("track added");
                return result<void>::success();
            },
            [&](const remove_track& value) -> result<void> {
                const auto found = std::find_if(updated.model.tracks.begin(), updated.model.tracks.end(), [&](const track& item) {
                    return item.id == value.track_id;
                });
                if (value.track_id == 0U || found == updated.model.tracks.end()) {
                    return result<void>::failure(edit_error(error_code::invalid_edit, "remove references an unknown track ID"));
                }
                updated.model.tracks.erase(found);
                const auto remove_member = [&](playlist& target) {
                    std::erase(target.track_ids, value.track_id);
                };
                remove_member(*updated.model.master_playlist);
                for (auto& target : updated.model.playlists) {
                    if (target.kind == playlist_kind::ordinary) {
                        remove_member(target);
                    }
                }
                updated.requested_changes.push_back("track removed");
                return result<void>::success();
            },
            [&](const add_ordinary_playlist& value) -> result<void> {
                auto persistent_id = next_persistent_id();
                if (!persistent_id) {
                    return result<void>::failure(persistent_id.error());
                }
                updated.model.playlists.push_back(playlist{
                    persistent_id.value(), value.name, playlist_kind::ordinary, {}});
                updated.requested_changes.push_back("ordinary playlist added");
                return result<void>::success();
            },
            [&](const remove_ordinary_playlist& value) -> result<void> {
                const auto found = std::find_if(updated.model.playlists.begin(), updated.model.playlists.end(), [&](const playlist& item) {
                    return item.persistent_id == value.persistent_id && item.kind == playlist_kind::ordinary;
                });
                if (value.persistent_id == 0U || found == updated.model.playlists.end()) {
                    return result<void>::failure(edit_error(error_code::invalid_edit, "remove references an unknown ordinary playlist"));
                }
                updated.model.playlists.erase(found);
                updated.requested_changes.push_back("ordinary playlist removed");
                return result<void>::success();
            },
            [&](const rename_ordinary_playlist& value) -> result<void> {
                const auto found = std::find_if(updated.model.playlists.begin(), updated.model.playlists.end(), [&](const playlist& item) {
                    return item.persistent_id == value.persistent_id && item.kind == playlist_kind::ordinary;
                });
                if (value.persistent_id == 0U || found == updated.model.playlists.end()) {
                    return result<void>::failure(edit_error(error_code::invalid_edit, "rename references an unknown ordinary playlist"));
                }
                found->name = value.name;
                updated.requested_changes.push_back("ordinary playlist renamed");
                return result<void>::success();
            },
            [&](const replace_ordinary_playlist_members& value) -> result<void> {
                const auto found = std::find_if(updated.model.playlists.begin(), updated.model.playlists.end(), [&](const playlist& item) {
                    return item.persistent_id == value.persistent_id && item.kind == playlist_kind::ordinary;
                });
                if (value.persistent_id == 0U || found == updated.model.playlists.end()) {
                    return result<void>::failure(edit_error(error_code::invalid_edit, "member replacement references an unknown ordinary playlist"));
                }
                std::unordered_set<std::uint32_t> seen;
                for (const auto id : value.track_ids) {
                    if (!seen.insert(id).second) {
                        return result<void>::failure(edit_error(
                            error_code::duplicate_playlist_member, "member replacement contains a duplicate track ID"));
                    }
                    const auto track_found = std::find_if(updated.model.tracks.begin(), updated.model.tracks.end(), [&](const track& item) {
                        return item.id == id;
                    });
                    if (id == 0U || track_found == updated.model.tracks.end()) {
                        return result<void>::failure(edit_error(
                            error_code::dangling_track_reference, "member replacement contains an unknown track ID"));
                    }
                }
                found->track_ids = value.track_ids;
                updated.requested_changes.push_back("ordinary playlist members replaced");
                return result<void>::success();
            }}, operation);
        if (!operation_result) {
            return result<database_document>::failure(operation_result.error());
        }
    }
    updated.modified = true;
    const auto validation = validator{}.validate(updated);
    if (!validation) {
        return result<database_document>::failure(validation.error());
    }
    return result<database_document>::success(std::move(updated));
}

}  // namespace foopodbridge::core::database

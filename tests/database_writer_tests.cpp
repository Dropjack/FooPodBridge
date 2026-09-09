#include "database_test_support.h"

#include "foopodbridge/core/database/comparator.h"
#include "foopodbridge/core/database/reader.h"

#include <exception>
#include <iostream>

namespace {

void append_u32(std::vector<std::byte>& bytes, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32U; shift += 8U) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
    }
}

void set_u32(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32U; shift += 8U) {
        bytes[offset + shift / 8U] = static_cast<std::byte>((value >> shift) & 0xffU);
    }
}

void append_marker(std::vector<std::byte>& bytes, const char* marker) {
    for (std::size_t index = 0; index < 4U; ++index) {
        bytes.push_back(static_cast<std::byte>(marker[index]));
    }
}

}  // namespace

int main() {
    using namespace foopodbridge::core::database;
    try {
        const auto generation = foopodbridge::tests::fixed_generation();
        const auto profile = traditional_unsigned_profile();
        const auto empty_a = writer{}.create_empty("媒体库", profile, generation);
        const auto empty_b = writer{}.create_empty("媒体库", profile, generation);
        foopodbridge::tests::require(empty_a.has_value() && empty_b.has_value(), "Unicode empty generation failed");
        foopodbridge::tests::require(
            comparator{}.exact_bytes(empty_a.value().original_bytes, empty_b.value().original_bytes).equal,
            "fixed GenerationContext was not deterministic");

        edit_plan additions;
        additions.operations.push_back(add_track{"F00:first.mp3", "一", "", ""});
        additions.operations.push_back(add_track{"F00:second.mp3", "二", "", ""});
        additions.operations.push_back(add_ordinary_playlist{"顺序"});
        auto edited = editor{}.apply(empty_a.value(), additions, generation);
        foopodbridge::tests::require(edited.has_value(), "add operations failed");
        const auto playlist_id = edited.value().model.playlists.front().persistent_id;

        edit_plan replace;
        replace.operations.push_back(replace_ordinary_playlist_members{playlist_id, {2U, 1U}});
        replace.operations.push_back(rename_ordinary_playlist{playlist_id, "倒序"});
        auto replaced = editor{}.apply(edited.value(), replace, generation);
        foopodbridge::tests::require(replaced.has_value(), "playlist update failed");
        const auto bytes = writer{}.write(replaced.value(), profile, generation);
        foopodbridge::tests::require(bytes.has_value(), "edited database write failed");
        const auto reread = reader{}.read(bytes.value(), profile, "public-writer-test");
        foopodbridge::tests::require(reread.has_value(), "edited database reread failed");
        foopodbridge::tests::require(
            comparator{}.semantic(replaced.value().model, reread.value().model).equal,
            "edited semantics did not round-trip");

        edit_plan duplicate;
        duplicate.operations.push_back(replace_ordinary_playlist_members{playlist_id, {1U, 1U}});
        const auto rejected_duplicate = editor{}.apply(edited.value(), duplicate, generation);
        foopodbridge::tests::require(
            !rejected_duplicate && rejected_duplicate.error().code == error_code::duplicate_playlist_member,
            "duplicate replacement member was not rejected");

        edit_plan removal;
        removal.operations.push_back(remove_track{1U});
        const auto removed = editor{}.apply(replaced.value(), removal, generation);
        foopodbridge::tests::require(removed.has_value(), "track removal failed");
        foopodbridge::tests::require(
            removed.value().model.master_playlist->track_ids == std::vector<std::uint32_t>{2U},
            "master did not remove the deleted track");
        foopodbridge::tests::require(
            removed.value().model.playlists.front().track_ids == std::vector<std::uint32_t>{2U},
            "ordinary playlist did not remove the deleted track");

        auto preserve_document = reader{}.read(empty_a.value().original_bytes, traditional_preserve_only_profile());
        foopodbridge::tests::require(preserve_document.has_value(), "preserve-only parse failed");
        edit_plan forbidden;
        forbidden.operations.push_back(add_track{"x", "x", "x", "x"});
        const auto rejected_edit = editor{}.apply(preserve_document.value(), forbidden, generation);
        foopodbridge::tests::require(
            !rejected_edit && rejected_edit.error().code == error_code::profile_not_writable,
            "preserve-only edit was not rejected");

        auto changed = reread.value().model;
        changed.tracks.front().id = 99U;
        foopodbridge::tests::require(
            !comparator{}.semantic(reread.value().model, changed).equal,
            "semantic comparator missed an unexpected change");

        auto with_unknown_dataset = empty_a.value().original_bytes;
        const auto dataset_start = with_unknown_dataset.size();
        append_marker(with_unknown_dataset, "mhsd");
        append_u32(with_unknown_dataset, 96U);
        append_u32(with_unknown_dataset, 108U);
        append_u32(with_unknown_dataset, 99U);
        with_unknown_dataset.resize(dataset_start + 96U, std::byte{0});
        append_marker(with_unknown_dataset, "zzzz");
        append_u32(with_unknown_dataset, 12U);
        append_u32(with_unknown_dataset, 0U);
        set_u32(with_unknown_dataset, 8U, static_cast<std::uint32_t>(with_unknown_dataset.size()));
        set_u32(with_unknown_dataset, 20U, 3U);
        const auto unknown = reader{}.read(with_unknown_dataset, profile, "unknown-public-fixture");
        foopodbridge::tests::require(
            unknown.has_value() && unknown.value().has_opaque_dependency,
            "unknown dataset was not preserved as an opaque dependency");
        const auto unknown_no_op = writer{}.write(unknown.value(), profile);
        foopodbridge::tests::require(
            unknown_no_op.has_value() &&
                comparator{}.exact_bytes(with_unknown_dataset, unknown_no_op.value()).equal,
            "unknown dataset changed during no-op round-trip");
        const auto blocked_unknown_edit = editor{}.apply(unknown.value(), forbidden, generation);
        foopodbridge::tests::require(
            !blocked_unknown_edit && blocked_unknown_edit.error().code == error_code::opaque_dependency,
            "opaque dependency did not block a semantic edit");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

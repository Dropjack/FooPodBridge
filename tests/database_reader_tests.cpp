#include "database_test_support.h"

#include "foopodbridge/core/database/comparator.h"
#include "foopodbridge/core/database/reader.h"
#include "foopodbridge/core/database/validator.h"

#include <exception>
#include <iostream>

int main() {
    using namespace foopodbridge::core::database;
    try {
        const auto profile = traditional_unsigned_profile();
        const auto generation = foopodbridge::tests::fixed_generation();
        const auto empty = foopodbridge::tests::make_empty();
        foopodbridge::tests::require(empty.model.tracks.empty(), "empty library contains tracks");
        foopodbridge::tests::require(empty.model.master_playlist.has_value(), "empty library has no master");
        foopodbridge::tests::require(empty.root.children.size() == 2U, "empty library has the wrong dataset count");
        foopodbridge::tests::require(validator{}.validate(empty).has_value(), "empty library validation failed");

        const auto rewritten = writer{}.write(empty, profile, generation);
        foopodbridge::tests::require(rewritten.has_value(), "empty library rewrite failed");
        foopodbridge::tests::require(
            comparator{}.exact_bytes(empty.original_bytes, rewritten.value()).equal,
            "deterministic no-op bytes changed");

        edit_plan plan;
        plan.operations.push_back(add_track{"F00:audio.mp3", "Title", "Artist", "Album"});
        const auto edited = editor{}.apply(empty, plan, generation);
        foopodbridge::tests::require(edited.has_value(), "track edit failed");
        const auto bytes = writer{}.write(edited.value(), profile, generation);
        foopodbridge::tests::require(bytes.has_value(), "track serialization failed");
        const auto reread = reader{}.read(bytes.value(), profile, "public-synthetic");
        foopodbridge::tests::require(reread.has_value(), "track reread failed");
        foopodbridge::tests::require(reread.value().model.tracks.size() == 1U, "track count did not round-trip");
        foopodbridge::tests::require(
            reread.value().model.master_playlist->track_ids == std::vector<std::uint32_t>{1U},
            "master membership did not round-trip");
        foopodbridge::tests::require(validator{}.validate(reread.value()).has_value(), "reread validation failed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

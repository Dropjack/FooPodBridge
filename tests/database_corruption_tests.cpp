#include "database_test_support.h"

#include "foopodbridge/core/database/reader.h"
#include "foopodbridge/core/database/validator.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string_view>

namespace {

void set_u32(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32U; shift += 8U) {
        bytes[offset + shift / 8U] = static_cast<std::byte>((value >> shift) & 0xffU);
    }
}

[[nodiscard]] std::size_t find_marker(
    const std::vector<std::byte>& bytes,
    std::string_view marker) {
    for (std::size_t offset = 0; offset + marker.size() <= bytes.size(); ++offset) {
        bool matches = true;
        for (std::size_t index = 0; index < marker.size(); ++index) {
            if (bytes[offset + index] != static_cast<std::byte>(marker[index])) {
                matches = false;
                break;
            }
        }
        if (matches) {
            return offset;
        }
    }
    throw std::runtime_error("required marker was not found");
}

}  // namespace

int main() {
    using namespace foopodbridge::core::database;
    try {
        const auto valid = foopodbridge::tests::make_empty().original_bytes;
        const auto profile = traditional_unsigned_profile();
        for (std::size_t length = 0; length < valid.size(); ++length) {
            const auto parsed = reader{}.read(std::span<const std::byte>(valid).first(length), profile);
            foopodbridge::tests::require(!parsed, "a truncated database was accepted");
        }

        auto out_of_bounds = valid;
        set_u32(out_of_bounds, 8U, static_cast<std::uint32_t>(valid.size() + 1U));
        foopodbridge::tests::require(!reader{}.read(out_of_bounds, profile), "out-of-bounds root was accepted");

        auto excessive_count = valid;
        set_u32(excessive_count, 20U, 0xffffffffU);
        const auto count_result = reader{}.read(excessive_count, profile);
        foopodbridge::tests::require(
            !count_result && count_result.error().code == error_code::count_exceeds_section,
            "excessive dataset count was not classified");

        auto unsupported = valid;
        unsupported[12U] = std::byte{2};
        const auto unsupported_result = reader{}.read(unsupported, profile);
        foopodbridge::tests::require(
            !unsupported_result && unsupported_result.error().code == error_code::unsupported_container,
            "unsupported container was not classified");

        auto limited_profile = profile;
        limited_profile.maximum_input_bytes = valid.size() - 1U;
        const auto limited = reader{}.read(valid, limited_profile);
        foopodbridge::tests::require(
            !limited && limited.error().code == error_code::resource_limit_exceeded,
            "input resource limit was not enforced");

        edit_plan add_one;
        add_one.operations.push_back(add_track{"F00:test.mp3", "text", "", ""});
        const auto one_track = editor{}.apply(
            foopodbridge::tests::make_empty(), add_one, foopodbridge::tests::fixed_generation());
        foopodbridge::tests::require(one_track.has_value(), "failed to prepare text corruption fixture");
        const auto one_track_bytes = writer{}.write(
            one_track.value(), profile, foopodbridge::tests::fixed_generation());
        foopodbridge::tests::require(one_track_bytes.has_value(), "failed to serialize text corruption fixture");
        auto invalid_text = one_track_bytes.value();
        const auto mhod = find_marker(invalid_text, "mhod");
        set_u32(invalid_text, mhod + 28U, 1U);
        const auto invalid_text_result = reader{}.read(invalid_text, profile);
        foopodbridge::tests::require(
            !invalid_text_result && invalid_text_result.error().code == error_code::invalid_text,
            "odd UTF-16 byte length was not classified");

        auto invalid_model = foopodbridge::tests::make_empty();
        invalid_model.model.tracks.push_back(track{1U, 7U, {}, {}, {}, {}});
        invalid_model.model.tracks.push_back(track{1U, 8U, {}, {}, {}, {}});
        invalid_model.model.master_playlist->track_ids = {1U};
        const auto duplicate = validator{}.validate(invalid_model);
        foopodbridge::tests::require(
            !duplicate && duplicate.error().code == error_code::duplicate_track_id,
            "duplicate track ID was not classified");

        auto missing_master = foopodbridge::tests::make_empty();
        missing_master.model.master_playlist.reset();
        const auto invalid_edit = editor{}.apply(missing_master, add_one, foopodbridge::tests::fixed_generation());
        foopodbridge::tests::require(
            !invalid_edit && invalid_edit.error().code == error_code::missing_master_playlist,
            "editor did not validate its original model");

        auto output_profile = profile;
        output_profile.maximum_output_bytes = 16U;
        const auto too_large = writer{}.write(
            foopodbridge::tests::make_empty(), output_profile, foopodbridge::tests::fixed_generation());
        foopodbridge::tests::require(
            !too_large && too_large.error().code == error_code::resource_limit_exceeded,
            "output resource limit was not enforced");

        auto changed = foopodbridge::tests::make_empty();
        changed.modified = true;
        generation_context no_time = foopodbridge::tests::fixed_generation();
        no_time.timestamp = 0U;
        const auto missing_time = writer{}.write(changed, profile, no_time);
        foopodbridge::tests::require(
            !missing_time && missing_time.error().code == error_code::invalid_edit,
            "zero injected timestamp was not rejected");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

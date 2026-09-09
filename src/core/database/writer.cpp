#include "foopodbridge/core/database/writer.h"

#include "foopodbridge/core/database/comparator.h"
#include "foopodbridge/core/database/reader.h"
#include "foopodbridge/core/database/validator.h"
#include "internal.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace foopodbridge::core::database {
namespace {

constexpr std::uint32_t root_header_size = 244U;
constexpr std::uint32_t dataset_header_size = 96U;
constexpr std::uint32_t list_header_size = 92U;
constexpr std::uint32_t track_header_size = 156U;
constexpr std::uint32_t playlist_header_size = 140U;
constexpr std::uint32_t playlist_item_header_size = 76U;

[[nodiscard]] database_error write_error(error_code code, std::string summary) {
    return detail::make_error(code, 0U, {}, "writer", std::move(summary));
}

[[nodiscard]] bool fits_u32(std::size_t value) noexcept {
    return value <= std::numeric_limits<std::uint32_t>::max();
}

[[nodiscard]] result<void> ensure_limit(std::size_t size, const format_profile& profile) {
    if (size > profile.maximum_output_bytes) {
        return result<void>::failure(write_error(
            error_code::resource_limit_exceeded, "generated output exceeds the configured byte limit"));
    }
    if (!fits_u32(size)) {
        return result<void>::failure(write_error(
            error_code::length_overflow, "generated traditional database exceeds its 32-bit length field"));
    }
    return result<void>::success();
}

[[nodiscard]] std::size_t append_fixed_record(
    std::vector<std::byte>& output,
    std::string_view marker,
    std::uint32_t header_size,
    std::uint32_t section_size) {
    const auto start = output.size();
    detail::append_marker(output, marker);
    detail::append_u32(output, header_size);
    detail::append_u32(output, section_size);
    output.resize(start + header_size, std::byte{0});
    return start;
}

[[nodiscard]] result<std::vector<std::byte>> make_string_record(
    std::uint32_t type,
    const std::string& text,
    std::string path) {
    auto encoded = detail::utf8_to_utf16(text, std::move(path));
    if (!encoded) {
        return result<std::vector<std::byte>>::failure(encoded.error());
    }
    constexpr std::size_t payload_header_size = 40U;
    if (encoded.value().size() >
        (std::numeric_limits<std::uint32_t>::max() - payload_header_size) / 2U) {
        return result<std::vector<std::byte>>::failure(write_error(
            error_code::length_overflow, "encoded string is too large for a traditional record"));
    }
    const auto byte_length = encoded.value().size() * 2U;
    const auto section_size = payload_header_size + byte_length;
    std::vector<std::byte> output;
    output.reserve(section_size);
    const auto start = append_fixed_record(output, "mhod", 24U, static_cast<std::uint32_t>(section_size));
    detail::set_u32(output, start + 12U, type);
    detail::append_u32(output, 0U);
    detail::append_u32(output, static_cast<std::uint32_t>(byte_length));
    detail::append_u32(output, 1U);
    detail::append_u32(output, 0U);
    for (const auto unit : encoded.value()) {
        output.push_back(static_cast<std::byte>(unit & 0xffU));
        output.push_back(static_cast<std::byte>(unit >> 8U));
    }
    return result<std::vector<std::byte>>::success(std::move(output));
}

[[nodiscard]] result<std::vector<std::byte>> make_track_record(const track& value) {
    struct text_field { std::uint32_t type; const std::string* value; const char* path; };
    const text_field fields[] = {
        {1U, &value.title, "writer/track/title"},
        {2U, &value.location, "writer/track/location"},
        {3U, &value.album, "writer/track/album"},
        {4U, &value.artist, "writer/track/artist"},
    };
    std::vector<std::vector<std::byte>> children;
    std::size_t section_size = track_header_size;
    for (const auto& field : fields) {
        auto child = make_string_record(field.type, *field.value, field.path);
        if (!child) {
            return result<std::vector<std::byte>>::failure(child.error());
        }
        if (child.value().size() > std::numeric_limits<std::uint32_t>::max() - section_size) {
            return result<std::vector<std::byte>>::failure(write_error(
                error_code::length_overflow, "track record length overflow"));
        }
        section_size += child.value().size();
        children.push_back(std::move(child.value()));
    }
    std::vector<std::byte> output;
    output.reserve(section_size);
    const auto start = append_fixed_record(
        output, "mhit", track_header_size, static_cast<std::uint32_t>(section_size));
    detail::set_u32(output, start + 12U, static_cast<std::uint32_t>(children.size()));
    detail::set_u32(output, start + 16U, value.id);
    detail::set_u64(output, start + 112U, value.persistent_id.value_or(0U));
    for (const auto& child : children) {
        output.insert(output.end(), child.begin(), child.end());
    }
    return result<std::vector<std::byte>>::success(std::move(output));
}

[[nodiscard]] result<std::vector<std::byte>> make_playlist_record(
    const playlist& value,
    std::uint32_t timestamp) {
    auto name = make_string_record(1U, value.name, "writer/playlist/name");
    if (!name) {
        return result<std::vector<std::byte>>::failure(name.error());
    }
    if (value.track_ids.size() >
        (std::numeric_limits<std::uint32_t>::max() - playlist_header_size - name.value().size()) /
            playlist_item_header_size) {
        return result<std::vector<std::byte>>::failure(write_error(
            error_code::length_overflow, "playlist record length overflow"));
    }
    const auto section_size = playlist_header_size + name.value().size() +
        value.track_ids.size() * playlist_item_header_size;
    std::vector<std::byte> output;
    output.reserve(section_size);
    const auto start = append_fixed_record(
        output, "mhyp", playlist_header_size, static_cast<std::uint32_t>(section_size));
    detail::set_u32(output, start + 12U, 1U);
    detail::set_u32(output, start + 16U, static_cast<std::uint32_t>(value.track_ids.size()));
    output[start + 20U] = value.kind == playlist_kind::master ? std::byte{1} : std::byte{0};
    detail::set_u32(output, start + 24U, timestamp);
    detail::set_u64(output, start + 28U, value.persistent_id);
    output.insert(output.end(), name.value().begin(), name.value().end());
    for (const auto track_id : value.track_ids) {
        const auto item_start = append_fixed_record(
            output, "mhip", playlist_item_header_size, playlist_item_header_size);
        detail::set_u32(output, item_start + 12U, 0U);
        detail::set_u32(output, item_start + 24U, track_id);
    }
    return result<std::vector<std::byte>>::success(std::move(output));
}

[[nodiscard]] result<std::vector<std::byte>> serialize(
    const database_document& document,
    const format_profile& profile,
    const generation_context& generation) {
    if (generation.timestamp == 0U) {
        return result<std::vector<std::byte>>::failure(write_error(
            error_code::invalid_edit, "unsigned serialization requires a nonzero injected timestamp"));
    }

    std::vector<std::vector<std::byte>> track_records;
    for (const auto& value : document.model.tracks) {
        auto record = make_track_record(value);
        if (!record) {
            return result<std::vector<std::byte>>::failure(record.error());
        }
        track_records.push_back(std::move(record.value()));
    }
    std::vector<std::vector<std::byte>> playlist_records;
    auto master = make_playlist_record(*document.model.master_playlist, generation.timestamp);
    if (!master) {
        return result<std::vector<std::byte>>::failure(master.error());
    }
    playlist_records.push_back(std::move(master.value()));
    for (const auto& value : document.model.playlists) {
        if (value.kind != playlist_kind::ordinary) {
            return result<std::vector<std::byte>>::failure(write_error(
                error_code::opaque_dependency, "unsigned writer cannot serialize Smart or opaque playlists"));
        }
        auto record = make_playlist_record(value, generation.timestamp);
        if (!record) {
            return result<std::vector<std::byte>>::failure(record.error());
        }
        playlist_records.push_back(std::move(record.value()));
    }

    std::vector<std::byte> output;
    const auto root_start = append_fixed_record(output, "mhbd", root_header_size, 0U);
    output[root_start + 12U] = std::byte{1};
    detail::set_u32(output, root_start + 16U, 42U);
    detail::set_u32(output, root_start + 20U, 2U);
    detail::set_u64(output, root_start + 24U, document.model.persistent_id);

    const auto track_dataset_start = append_fixed_record(output, "mhsd", dataset_header_size, 0U);
    detail::set_u32(output, track_dataset_start + 12U, 1U);
    const auto track_list_start = append_fixed_record(output, "mhlt", list_header_size, 0U);
    detail::set_u32(output, track_list_start + 8U, static_cast<std::uint32_t>(track_records.size()));
    for (const auto& record : track_records) {
        output.insert(output.end(), record.begin(), record.end());
        auto limit = ensure_limit(output.size(), profile);
        if (!limit) {
            return result<std::vector<std::byte>>::failure(limit.error());
        }
    }
    detail::set_u32(output, track_dataset_start + 8U,
        static_cast<std::uint32_t>(output.size() - track_dataset_start));

    const auto playlist_dataset_start = append_fixed_record(output, "mhsd", dataset_header_size, 0U);
    detail::set_u32(output, playlist_dataset_start + 12U, 2U);
    const auto playlist_list_start = append_fixed_record(output, "mhlp", list_header_size, 0U);
    detail::set_u32(output, playlist_list_start + 8U, static_cast<std::uint32_t>(playlist_records.size()));
    for (const auto& record : playlist_records) {
        output.insert(output.end(), record.begin(), record.end());
        auto limit = ensure_limit(output.size(), profile);
        if (!limit) {
            return result<std::vector<std::byte>>::failure(limit.error());
        }
    }
    detail::set_u32(output, playlist_dataset_start + 8U,
        static_cast<std::uint32_t>(output.size() - playlist_dataset_start));
    auto limit = ensure_limit(output.size(), profile);
    if (!limit) {
        return result<std::vector<std::byte>>::failure(limit.error());
    }
    detail::set_u32(output, root_start + 8U, static_cast<std::uint32_t>(output.size()));
    return result<std::vector<std::byte>>::success(std::move(output));
}

}  // namespace

result<std::vector<std::byte>> writer::write(
    const database_document& document,
    const format_profile& profile,
    const generation_context& generation) const {
    if (document.source_profile != profile.kind) {
        return result<std::vector<std::byte>>::failure(write_error(
            error_code::profile_mismatch, "document capability does not match the requested writer profile"));
    }
    const auto validation = validator{}.validate(document);
    if (!validation) {
        return result<std::vector<std::byte>>::failure(validation.error());
    }
    if (profile.kind == profile_kind::traditional_preserve_only) {
        if (document.modified) {
            return result<std::vector<std::byte>>::failure(write_error(
                error_code::profile_not_writable, "preserve-only document contains semantic changes"));
        }
        if (document.original_bytes.size() > profile.maximum_output_bytes) {
            return result<std::vector<std::byte>>::failure(write_error(
                error_code::resource_limit_exceeded, "preserved output exceeds the configured byte limit"));
        }
        return result<std::vector<std::byte>>::success(document.original_bytes);
    }
    if (!document.modified && !document.original_bytes.empty()) {
        if (document.original_bytes.size() > profile.maximum_output_bytes) {
            return result<std::vector<std::byte>>::failure(write_error(
                error_code::resource_limit_exceeded, "unchanged output exceeds the configured byte limit"));
        }
        return result<std::vector<std::byte>>::success(document.original_bytes);
    }
    if (document.has_opaque_dependency) {
        return result<std::vector<std::byte>>::failure(write_error(
            error_code::opaque_dependency, "unsigned rewrite is blocked by an opaque dependency"));
    }

    auto output = serialize(document, profile, generation);
    if (!output) {
        return output;
    }
    auto reread = reader{}.read(output.value(), profile, "generated-output");
    if (!reread) {
        return result<std::vector<std::byte>>::failure(write_error(
            error_code::generated_output_invalid, "fresh Reader rejected generated output"));
    }
    const auto output_validation = validator{}.validate(reread.value());
    if (!output_validation) {
        return result<std::vector<std::byte>>::failure(write_error(
            error_code::generated_output_invalid, "independent Validator rejected generated output"));
    }
    const auto comparison = comparator{}.semantic(document.model, reread.value().model);
    if (!comparison.equal) {
        return result<std::vector<std::byte>>::failure(write_error(
            error_code::comparison_mismatch, "semantic comparison rejected generated output"));
    }
    return output;
}

result<database_document> writer::create_empty(
    std::string library_name,
    const format_profile& profile,
    const generation_context& generation) const {
    if (profile.kind != profile_kind::traditional_unsigned) {
        return result<database_document>::failure(write_error(
            error_code::profile_not_writable, "empty database generation is only available to TraditionalUnsigned"));
    }
    if (generation.database_persistent_id == 0U || generation.master_playlist_persistent_id == 0U ||
        generation.timestamp == 0U || generation.database_persistent_id == generation.master_playlist_persistent_id) {
        return result<database_document>::failure(write_error(
            error_code::duplicate_persistent_id, "empty database GenerationContext contains zero or duplicate values"));
    }
    database_document initial;
    initial.source_profile = profile_kind::traditional_unsigned;
    initial.model.persistent_id = generation.database_persistent_id;
    initial.model.master_playlist = playlist{
        generation.master_playlist_persistent_id,
        std::move(library_name),
        playlist_kind::master,
        {}};
    auto output = write(initial, profile, generation);
    if (!output) {
        return result<database_document>::failure(output.error());
    }
    return reader{}.read(output.value(), profile, "generated-empty-library");
}

}  // namespace foopodbridge::core::database

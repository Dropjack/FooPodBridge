#include "foopodbridge/core/database/reader.h"

#include "internal.h"

#include <algorithm>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace foopodbridge::core::database {
namespace {

using detail::make_error;
using detail::marker_is;
using detail::marker_string;
using detail::range_fits;
using detail::read_marker;
using detail::read_u32;
using detail::read_u64;

struct dataset_view {
    std::size_t offset{};
    std::size_t end{};
    std::size_t list_offset{};
    std::size_t list_header_size{};
    std::uint32_t type{};
    std::uint32_t count{};
    std::size_t root_child_index{};
};

[[nodiscard]] result<void> require_envelope(
    std::span<const std::byte> bytes,
    std::size_t offset,
    std::size_t parent_end,
    std::size_t minimum_header,
    std::string_view expected_marker,
    std::string path,
    std::uint32_t& header_size,
    std::uint32_t& section_size) {
    if (!range_fits(offset, 12U, parent_end) || parent_end > bytes.size()) {
        return result<void>::failure(make_error(
            error_code::truncated_input, offset, {}, std::move(path), "record envelope is truncated"));
    }
    const auto marker = read_marker(bytes, offset);
    if (!marker_is(marker, expected_marker)) {
        return result<void>::failure(make_error(
            error_code::invalid_marker, offset, marker_string(marker), std::move(path), "record marker does not match its parent"));
    }
    header_size = read_u32(bytes, offset + 4U);
    section_size = read_u32(bytes, offset + 8U);
    if (header_size < minimum_header || section_size < header_size) {
        return result<void>::failure(make_error(
            error_code::header_too_small, offset, marker_string(marker), std::move(path), "record header or section length is invalid"));
    }
    if (!range_fits(offset, section_size, parent_end)) {
        return result<void>::failure(make_error(
            error_code::section_out_of_bounds, offset, marker_string(marker), std::move(path), "record extends beyond its parent"));
    }
    return result<void>::success();
}

[[nodiscard]] result<std::pair<std::uint32_t, std::string>> parse_mhod(
    std::span<const std::byte> bytes,
    std::size_t offset,
    std::size_t parent_end,
    std::string path,
    record_node& node) {
    std::uint32_t header{};
    std::uint32_t section{};
    auto envelope = require_envelope(bytes, offset, parent_end, 24U, "mhod", path, header, section);
    if (!envelope) {
        return result<std::pair<std::uint32_t, std::string>>::failure(envelope.error());
    }
    const auto type = read_u32(bytes, offset + 12U);
    node = record_node{read_marker(bytes, offset), header, section, offset, true};
    if (type != 1U && type != 2U && type != 3U && type != 4U) {
        node.understood = false;
        return result<std::pair<std::uint32_t, std::string>>::success({type, {}});
    }
    if (header != 24U || section < 40U) {
        return result<std::pair<std::uint32_t, std::string>>::failure(make_error(
            error_code::unsupported_encoding, offset, "mhod", std::move(path), "known string record uses an unsupported envelope"));
    }
    const auto byte_length = static_cast<std::size_t>(read_u32(bytes, offset + 28U));
    if (!range_fits(offset + 40U, byte_length, offset + section)) {
        return result<std::pair<std::uint32_t, std::string>>::failure(make_error(
            error_code::section_out_of_bounds, offset, "mhod", std::move(path), "known string payload extends beyond its record"));
    }
    auto decoded = detail::utf16le_to_utf8(bytes.subspan(offset + 40U, byte_length), offset + 40U, path);
    if (!decoded) {
        return result<std::pair<std::uint32_t, std::string>>::failure(decoded.error());
    }
    return result<std::pair<std::uint32_t, std::string>>::success({type, std::move(decoded.value())});
}

[[nodiscard]] result<track> parse_track(
    std::span<const std::byte> bytes,
    std::size_t offset,
    std::size_t parent_end,
    std::string path,
    record_node& node,
    std::vector<diagnostic>& diagnostics) {
    std::uint32_t header{};
    std::uint32_t section{};
    auto envelope = require_envelope(bytes, offset, parent_end, 20U, "mhit", path, header, section);
    if (!envelope) {
        return result<track>::failure(envelope.error());
    }
    const auto child_count = read_u32(bytes, offset + 12U);
    const auto child_bytes = static_cast<std::size_t>(section - header);
    if (child_count > child_bytes / 24U) {
        return result<track>::failure(make_error(
            error_code::count_exceeds_section, offset, "mhit", path, "track child count cannot fit in its section"));
    }

    track parsed;
    parsed.id = read_u32(bytes, offset + 16U);
    if (header >= 120U) {
        const auto persistent_id = read_u64(bytes, offset + 112U);
        if (persistent_id != 0U) {
            parsed.persistent_id = persistent_id;
        }
    }
    node = record_node{read_marker(bytes, offset), header, section, offset, true};
    auto child_offset = offset + static_cast<std::size_t>(header);
    const auto record_end = offset + static_cast<std::size_t>(section);
    for (std::uint32_t index = 0; index < child_count; ++index) {
        record_node child;
        auto child_result = parse_mhod(bytes, child_offset, record_end, path + "/mhod", child);
        if (!child_result) {
            return result<track>::failure(child_result.error());
        }
        const auto& [type, text] = child_result.value();
        switch (type) {
        case 1U: parsed.title = text; break;
        case 2U: parsed.location = text; break;
        case 3U: parsed.album = text; break;
        case 4U: parsed.artist = text; break;
        default:
            diagnostics.push_back({child_offset, path + "/mhod", "unknown track child preserved"});
            break;
        }
        child_offset += static_cast<std::size_t>(child.section_size);
        node.children.push_back(std::move(child));
    }
    if (child_offset != record_end) {
        diagnostics.push_back({child_offset, path, "unmodeled track payload preserved"});
    }
    return result<track>::success(std::move(parsed));
}

[[nodiscard]] result<playlist> parse_playlist(
    std::span<const std::byte> bytes,
    std::size_t offset,
    std::size_t parent_end,
    std::string path,
    record_node& node,
    std::vector<diagnostic>& diagnostics) {
    std::uint32_t header{};
    std::uint32_t section{};
    auto envelope = require_envelope(bytes, offset, parent_end, 36U, "mhyp", path, header, section);
    if (!envelope) {
        return result<playlist>::failure(envelope.error());
    }
    const auto data_child_count = read_u32(bytes, offset + 12U);
    const auto item_count = read_u32(bytes, offset + 16U);
    const auto child_bytes = static_cast<std::size_t>(section - header);
    if (data_child_count > child_bytes / 24U || item_count > child_bytes / 12U) {
        return result<playlist>::failure(make_error(
            error_code::count_exceeds_section, offset, "mhyp", path, "playlist child count cannot fit in its section"));
    }

    playlist parsed;
    parsed.kind = bytes[offset + 20U] == std::byte{1} ? playlist_kind::master : playlist_kind::ordinary;
    parsed.persistent_id = read_u64(bytes, offset + 28U);
    node = record_node{read_marker(bytes, offset), header, section, offset, true};
    auto child_offset = offset + static_cast<std::size_t>(header);
    const auto record_end = offset + static_cast<std::size_t>(section);
    for (std::uint32_t index = 0; index < data_child_count; ++index) {
        record_node child;
        auto child_result = parse_mhod(bytes, child_offset, record_end, path + "/mhod", child);
        if (!child_result) {
            return result<playlist>::failure(child_result.error());
        }
        const auto& [type, text] = child_result.value();
        if (type == 1U) {
            parsed.name = text;
        } else {
            if (type == 50U || type == 51U) {
                parsed.kind = playlist_kind::smart;
            }
            diagnostics.push_back({child_offset, path + "/mhod", "opaque playlist child preserved"});
        }
        child_offset += static_cast<std::size_t>(child.section_size);
        node.children.push_back(std::move(child));
    }
    parsed.track_ids.reserve(std::min<std::size_t>(item_count, child_bytes / 12U));
    for (std::uint32_t index = 0; index < item_count; ++index) {
        std::uint32_t item_header{};
        std::uint32_t item_section{};
        auto item_envelope = require_envelope(
            bytes, child_offset, record_end, 28U, "mhip", path + "/mhip", item_header, item_section);
        if (!item_envelope) {
            return result<playlist>::failure(item_envelope.error());
        }
        parsed.track_ids.push_back(read_u32(bytes, child_offset + 24U));
        node.children.push_back(record_node{
            read_marker(bytes, child_offset), item_header, item_section, child_offset, true});
        child_offset += static_cast<std::size_t>(item_section);
    }
    if (child_offset != record_end) {
        diagnostics.push_back({child_offset, path, "unmodeled playlist payload preserved"});
    }
    return result<playlist>::success(std::move(parsed));
}

}  // namespace

namespace {

[[nodiscard]] result<database_document> read_database(
    std::span<const std::byte> bytes,
    const format_profile& profile,
    const hash58_device_key* device_key,
    std::string_view source_label) {
    (void)source_label;
    if (bytes.size() > profile.maximum_input_bytes) {
        return result<database_document>::failure(make_error(
            error_code::resource_limit_exceeded, 0U, {}, "root", "input exceeds the configured byte limit"));
    }
    if (bytes.size() >= 16U) {
        constexpr std::string_view sqlite_marker = "SQLite format 3";
        bool sqlite = true;
        for (std::size_t index = 0; index < sqlite_marker.size(); ++index) {
            if (bytes[index] != static_cast<std::byte>(sqlite_marker[index])) {
                sqlite = false;
                break;
            }
        }
        if (sqlite) {
            return result<database_document>::failure(make_error(
                error_code::unsupported_container, 0U, {}, "root", "SQLite databases are outside the traditional profile"));
        }
    }

    std::uint32_t root_header{};
    std::uint32_t root_section{};
    auto root_envelope = require_envelope(bytes, 0U, bytes.size(), 32U, "mhbd", "root", root_header, root_section);
    if (!root_envelope) {
        return result<database_document>::failure(root_envelope.error());
    }
    if (bytes[12U] != std::byte{1}) {
        return result<database_document>::failure(make_error(
            error_code::unsupported_container, 12U, "mhbd", "root/format", "only the uncompressed traditional format is supported"));
    }
    if (profile.kind == profile_kind::traditional_hash58) {
        if (root_header != profile.root_header_size) {
            return result<database_document>::failure(make_error(
                error_code::unsupported_signed_profile, 4U, "mhbd", "root/header",
                "hash58 profile requires the selected fixed mhbd header variant"));
        }
        if (bytes[0x30U] != std::byte{1} || bytes[0x31U] != std::byte{0}) {
            return result<database_document>::failure(make_error(
                error_code::invalid_hash_scheme, 0x30U, "mhbd", "root/hash58",
                "hash58 profile requires scheme 1"));
        }
        const auto version = read_u32(bytes, 16U);
        if (version != 49U && version != 115U) {
            return result<database_document>::failure(make_error(
                error_code::unsupported_signed_profile, 16U, "mhbd", "root/version",
                "hash58 reader supports only the observed version 49 and 115 variants"));
        }
    }

    database_document document;
    document.source_profile = profile.kind;
    document.original_bytes.assign(bytes.begin(), bytes.end());
    document.model.persistent_id = read_u64(bytes, 24U);
    document.root = record_node{read_marker(bytes, 0U), root_header, root_section, 0U, true};
    if (profile.kind == profile_kind::traditional_hash58) {
        document.hash58_status = hash58_signature_status::not_checked;
        if (device_key != nullptr) {
            document.hash58_status = verify_hash58(*device_key, bytes)
                ? hash58_signature_status::valid
                : hash58_signature_status::invalid;
        }
    }
    const auto dataset_count = read_u32(bytes, 20U);
    const auto root_end = static_cast<std::size_t>(root_section);
    if (dataset_count > (root_end - root_header) / 16U) {
        return result<database_document>::failure(make_error(
            error_code::count_exceeds_section, 20U, "mhbd", "root", "dataset count cannot fit in the root section"));
    }

    std::vector<dataset_view> datasets;
    datasets.reserve(std::min<std::size_t>(dataset_count, (root_end - root_header) / 16U));
    auto dataset_offset = static_cast<std::size_t>(root_header);
    for (std::uint32_t index = 0; index < dataset_count; ++index) {
        std::uint32_t dataset_header{};
        std::uint32_t dataset_section{};
        auto envelope = require_envelope(
            bytes, dataset_offset, root_end, 16U, "mhsd", "root/dataset", dataset_header, dataset_section);
        if (!envelope) {
            return result<database_document>::failure(envelope.error());
        }
        const auto dataset_end = dataset_offset + static_cast<std::size_t>(dataset_section);
        const auto list_offset = dataset_offset + static_cast<std::size_t>(dataset_header);
        if (!range_fits(list_offset, 12U, dataset_end)) {
            return result<database_document>::failure(make_error(
                error_code::truncated_input, list_offset, {}, "root/dataset/list", "dataset list envelope is truncated"));
        }
        const auto type = read_u32(bytes, dataset_offset + 12U);
        const auto list_marker = read_marker(bytes, list_offset);
        const bool recognized_list = (type == 1U && marker_is(list_marker, "mhlt")) ||
            ((type == 2U || type == 3U || type == 5U) && marker_is(list_marker, "mhlp")) ||
            (type == 4U && marker_is(list_marker, "mhla"));
        if (!recognized_list && (type == 1U || type == 2U || type == 3U || type == 4U || type == 5U)) {
            return result<database_document>::failure(make_error(
                error_code::invalid_marker, list_offset, marker_string(list_marker), "root/dataset/list", "known dataset has the wrong list marker"));
        }
        const auto list_header = read_u32(bytes, list_offset + 4U);
        const auto count = read_u32(bytes, list_offset + 8U);
        if (list_header < 12U || !range_fits(list_offset, list_header, dataset_end)) {
            return result<database_document>::failure(make_error(
                error_code::header_too_small, list_offset, marker_string(list_marker), "root/dataset/list", "list header is invalid"));
        }
        if (profile.kind == profile_kind::traditional_hash58 &&
            (dataset_header != profile.dataset_header_size || list_header != profile.list_header_size)) {
            return result<database_document>::failure(make_error(
                error_code::unsupported_signed_profile, dataset_offset, "mhsd", "root/dataset",
                "hash58 dataset or list header does not match the selected profile"));
        }

        record_node dataset_node{read_marker(bytes, dataset_offset), dataset_header, dataset_section, dataset_offset, recognized_list};
        dataset_node.children.push_back(record_node{
            list_marker, list_header, list_header, list_offset, recognized_list});
        const auto child_index = document.root.children.size();
        document.root.children.push_back(std::move(dataset_node));
        datasets.push_back(dataset_view{
            dataset_offset,
            dataset_end,
            list_offset,
            static_cast<std::size_t>(list_header),
            type,
            count,
            child_index});
        if (type != 1U && type != 2U && type != 3U && !(type == 4U && count == 0U)) {
            document.diagnostics.push_back({dataset_offset, "root/dataset", "opaque dataset preserved"});
            document.has_opaque_dependency = true;
        }
        dataset_offset = dataset_end;
    }
    if (profile.kind == profile_kind::traditional_hash58) {
        constexpr std::array<std::uint32_t, 4> required_types{4U, 1U, 3U, 2U};
        if (datasets.size() < required_types.size() ||
            !std::equal(required_types.begin(), required_types.end(), datasets.begin(),
                [](std::uint32_t expected, const dataset_view& actual) { return expected == actual.type; })) {
            return result<database_document>::failure(make_error(
                error_code::unsupported_signed_profile, root_header, "mhsd", "root/datasets",
                "hash58 profile requires leading dataset types 4, 1, 3, 2 in that order"));
        }
    }
    if (dataset_offset != root_end) {
        document.diagnostics.push_back({dataset_offset, "root", "root trailing bytes preserved"});
        document.has_opaque_dependency = true;
    }
    if (root_end != bytes.size()) {
        document.diagnostics.push_back({root_end, "file", "file trailing bytes preserved"});
        document.has_opaque_dependency = true;
    }

    const auto track_dataset = std::find_if(datasets.begin(), datasets.end(), [](const dataset_view& value) {
        return value.type == 1U;
    });
    if (track_dataset != datasets.end()) {
        auto record_offset = track_dataset->list_offset + track_dataset->list_header_size;
        auto& list_node = document.root.children[track_dataset->root_child_index].children.front();
        document.model.tracks.reserve(std::min<std::size_t>(
            track_dataset->count, (track_dataset->end - record_offset) / 20U));
        for (std::uint32_t index = 0; index < track_dataset->count; ++index) {
            record_node node;
            auto parsed = parse_track(
                bytes, record_offset, track_dataset->end, "root/tracks/track", node, document.diagnostics);
            if (!parsed) {
                return result<database_document>::failure(parsed.error());
            }
            if (profile.kind == profile_kind::traditional_hash58 && node.header_size != 584U) {
                return result<database_document>::failure(make_error(
                    error_code::unsupported_signed_profile, record_offset, "mhit", "root/tracks/track",
                    "hash58 track record does not use the observed 584-byte header"));
            }
            record_offset += static_cast<std::size_t>(node.section_size);
            list_node.children.push_back(std::move(node));
            document.model.tracks.push_back(std::move(parsed.value()));
        }
        if (record_offset != track_dataset->end) {
            return result<database_document>::failure(make_error(
                error_code::count_exceeds_section, record_offset, "mhlt", "root/tracks", "track count does not consume its dataset"));
        }
    }

    auto playlist_dataset = std::find_if(datasets.begin(), datasets.end(), [](const dataset_view& value) {
        return value.type == 2U;
    });
    if (playlist_dataset == datasets.end()) {
        playlist_dataset = std::find_if(datasets.begin(), datasets.end(), [](const dataset_view& value) {
            return value.type == 3U;
        });
    }
    if (playlist_dataset != datasets.end()) {
        auto record_offset = playlist_dataset->list_offset + playlist_dataset->list_header_size;
        auto& list_node = document.root.children[playlist_dataset->root_child_index].children.front();
        for (std::uint32_t index = 0; index < playlist_dataset->count; ++index) {
            record_node node;
            auto parsed = parse_playlist(
                bytes, record_offset, playlist_dataset->end, "root/playlists/playlist", node, document.diagnostics);
            if (!parsed) {
                return result<database_document>::failure(parsed.error());
            }
            if (profile.kind == profile_kind::traditional_hash58 &&
                node.header_size != 140U && node.header_size != 184U) {
                return result<database_document>::failure(make_error(
                    error_code::unsupported_signed_profile, record_offset, "mhyp", "root/playlists/playlist",
                    "hash58 playlist record does not use an observed 140-byte or 184-byte header"));
            }
            record_offset += static_cast<std::size_t>(node.section_size);
            list_node.children.push_back(std::move(node));
            if (parsed.value().kind == playlist_kind::master) {
                if (document.model.master_playlist.has_value()) {
                    return result<database_document>::failure(make_error(
                        error_code::multiple_master_playlists, record_offset, "mhyp", "root/playlists", "multiple master playlists found"));
                }
                document.model.master_playlist = std::move(parsed.value());
            } else {
                document.model.playlists.push_back(std::move(parsed.value()));
            }
        }
        if (record_offset != playlist_dataset->end) {
            return result<database_document>::failure(make_error(
                error_code::count_exceeds_section, record_offset, "mhlp", "root/playlists", "playlist count does not consume its dataset"));
        }
    }
    if (!document.diagnostics.empty()) {
        document.has_opaque_dependency = true;
    }
    return result<database_document>::success(std::move(document));
}

}  // namespace

result<database_document> reader::read(
    std::span<const std::byte> bytes,
    const format_profile& profile,
    std::string_view source_label) const {
    return read_database(bytes, profile, nullptr, source_label);
}

result<database_document> reader::read(
    std::span<const std::byte> bytes,
    const format_profile& profile,
    const hash58_device_key& device_key,
    std::string_view source_label) const {
    return read_database(bytes, profile, &device_key, source_label);
}

}  // namespace foopodbridge::core::database

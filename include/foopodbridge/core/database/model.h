#pragma once

#include "foopodbridge/core/database/format_profile.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace foopodbridge::core::database {

enum class playlist_kind {
    master,
    ordinary,
    smart,
    opaque,
};

struct track {
    std::uint32_t id{};
    std::optional<std::uint64_t> persistent_id;
    std::string location;
    std::string title;
    std::string artist;
    std::string album;

    friend bool operator==(const track&, const track&) = default;
};

struct playlist {
    std::uint64_t persistent_id{};
    std::string name;
    playlist_kind kind{playlist_kind::ordinary};
    std::vector<std::uint32_t> track_ids;

    friend bool operator==(const playlist&, const playlist&) = default;
};

struct database_model {
    std::uint64_t persistent_id{};
    std::vector<track> tracks;
    std::optional<playlist> master_playlist;
    std::vector<playlist> playlists;

    friend bool operator==(const database_model&, const database_model&) = default;
};

enum class node_state {
    unchanged,
    changed,
    inserted,
    removed,
};

struct record_node {
    std::array<char, 4> marker{};
    std::uint32_t header_size{};
    std::uint64_t section_size{};
    std::uint64_t offset{};
    bool understood{};
    node_state state{node_state::unchanged};
    std::vector<record_node> children;
};

struct diagnostic {
    std::uint64_t offset{};
    std::string path;
    std::string summary;
};

struct database_document {
    database_model model;
    record_node root;
    std::vector<std::byte> original_bytes;
    profile_kind source_profile{profile_kind::traditional_preserve_only};
    std::vector<diagnostic> diagnostics;
    std::vector<std::string> requested_changes;
    bool modified{};
    bool has_opaque_dependency{};
};

}  // namespace foopodbridge::core::database

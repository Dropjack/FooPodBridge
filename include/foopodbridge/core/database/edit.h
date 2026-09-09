#pragma once

#include "foopodbridge/core/database/error.h"
#include "foopodbridge/core/database/model.h"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace foopodbridge::core::database {

struct add_track {
    std::string location;
    std::string title;
    std::string artist;
    std::string album;
};
struct remove_track { std::uint32_t track_id{}; };
struct add_ordinary_playlist { std::string name; };
struct remove_ordinary_playlist { std::uint64_t persistent_id{}; };
struct rename_ordinary_playlist { std::uint64_t persistent_id{}; std::string name; };
struct replace_ordinary_playlist_members {
    std::uint64_t persistent_id{};
    std::vector<std::uint32_t> track_ids;
};

using edit_operation = std::variant<
    add_track,
    remove_track,
    add_ordinary_playlist,
    remove_ordinary_playlist,
    rename_ordinary_playlist,
    replace_ordinary_playlist_members>;

struct edit_plan { std::vector<edit_operation> operations; };

struct generation_context {
    std::uint64_t database_persistent_id{};
    std::uint64_t master_playlist_persistent_id{};
    std::uint32_t timestamp{};
    std::vector<std::uint64_t> new_persistent_ids;
};

class editor final {
public:
    [[nodiscard]] result<database_document> apply(
        const database_document& document,
        const edit_plan& plan,
        const generation_context& generation) const;
};

}  // namespace foopodbridge::core::database

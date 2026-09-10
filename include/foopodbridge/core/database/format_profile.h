#pragma once

#include <cstddef>
#include <cstdint>

namespace foopodbridge::core::database {

enum class profile_kind {
    traditional_unsigned,
    traditional_preserve_only,
    traditional_hash58,
};

struct format_profile {
    profile_kind kind{profile_kind::traditional_preserve_only};
    std::size_t maximum_input_bytes{512U * 1024U * 1024U};
    std::size_t maximum_output_bytes{512U * 1024U * 1024U};
    std::uint32_t root_header_size{244U};
    std::uint32_t database_version{42U};
    std::uint32_t dataset_header_size{96U};
    std::uint32_t list_header_size{92U};
    std::uint32_t track_header_size{156U};
    std::uint32_t playlist_header_size{140U};
    std::uint32_t playlist_item_header_size{76U};
};

[[nodiscard]] constexpr format_profile traditional_unsigned_profile() noexcept {
    return format_profile{profile_kind::traditional_unsigned};
}

[[nodiscard]] constexpr format_profile traditional_preserve_only_profile() noexcept {
    return format_profile{profile_kind::traditional_preserve_only};
}

[[nodiscard]] constexpr format_profile traditional_hash58_profile() noexcept {
    format_profile profile{profile_kind::traditional_hash58};
    profile.root_header_size = 244U;
    profile.database_version = 115U;
    profile.track_header_size = 584U;
    profile.playlist_header_size = 184U;
    return profile;
}

}  // namespace foopodbridge::core::database

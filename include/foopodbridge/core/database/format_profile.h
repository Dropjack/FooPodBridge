#pragma once

#include <cstddef>

namespace foopodbridge::core::database {

enum class profile_kind {
    traditional_unsigned,
    traditional_preserve_only,
};

struct format_profile {
    profile_kind kind{profile_kind::traditional_preserve_only};
    std::size_t maximum_input_bytes{512U * 1024U * 1024U};
    std::size_t maximum_output_bytes{512U * 1024U * 1024U};
};

[[nodiscard]] constexpr format_profile traditional_unsigned_profile() noexcept {
    return format_profile{profile_kind::traditional_unsigned};
}

[[nodiscard]] constexpr format_profile traditional_preserve_only_profile() noexcept {
    return format_profile{profile_kind::traditional_preserve_only};
}

}  // namespace foopodbridge::core::database

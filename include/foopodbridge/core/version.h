// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <cstdint>

namespace foopodbridge::core {

struct version final {
    std::uint32_t major;
    std::uint32_t minor;
    std::uint32_t patch;

    friend constexpr bool operator==(const version&, const version&) = default;
};

[[nodiscard]] version current_version() noexcept;

} // namespace foopodbridge::core

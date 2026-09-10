#pragma once

#include "foopodbridge/core/database/error.h"

#include <array>
#include <cstddef>
#include <span>
#include <string_view>

namespace foopodbridge::core::database {

using hash58_digest = std::array<std::byte, 20>;
using hash58_derived_key = std::array<std::byte, 20>;

class hash58_device_key final {
private:
    explicit hash58_device_key(std::array<std::byte, 8> bytes) noexcept : bytes_(bytes) {}

    std::array<std::byte, 8> bytes_{};

    friend result<hash58_device_key> parse_hash58_device_key(std::string_view text);
    friend hash58_derived_key derive_hash58_key(const hash58_device_key& device_key) noexcept;
};

[[nodiscard]] result<hash58_device_key> parse_hash58_device_key(std::string_view text);
[[nodiscard]] hash58_derived_key derive_hash58_key(const hash58_device_key& device_key) noexcept;
[[nodiscard]] result<hash58_digest> compute_hash58(
    const hash58_device_key& device_key,
    std::span<const std::byte> database_bytes);
[[nodiscard]] result<void> verify_hash58(
    const hash58_device_key& device_key,
    std::span<const std::byte> database_bytes);

}  // namespace foopodbridge::core::database

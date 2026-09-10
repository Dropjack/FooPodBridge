#pragma once

#include "foopodbridge/core/database/error.h"
#include "foopodbridge/core/database/format_profile.h"
#include "foopodbridge/core/database/hash58.h"
#include "foopodbridge/core/database/model.h"

#include <cstddef>
#include <span>
#include <string_view>

namespace foopodbridge::core::database {

class reader final {
public:
    [[nodiscard]] result<database_document> read(
        std::span<const std::byte> bytes,
        const format_profile& profile,
        std::string_view source_label = {}) const;

    [[nodiscard]] result<database_document> read(
        std::span<const std::byte> bytes,
        const format_profile& profile,
        const hash58_device_key& device_key,
        std::string_view source_label = {}) const;
};

}  // namespace foopodbridge::core::database

#pragma once

#include "foopodbridge/core/database/edit.h"
#include "foopodbridge/core/database/error.h"
#include "foopodbridge/core/database/format_profile.h"
#include "foopodbridge/core/database/hash58.h"
#include "foopodbridge/core/database/model.h"

#include <cstddef>
#include <string>
#include <vector>

namespace foopodbridge::core::database {

class writer final {
public:
    [[nodiscard]] result<std::vector<std::byte>> write(
        const database_document& document,
        const format_profile& profile,
        const generation_context& generation = {}) const;

    [[nodiscard]] result<std::vector<std::byte>> write(
        const database_document& document,
        const format_profile& profile,
        const hash58_device_key& device_key,
        const generation_context& generation = {}) const;

    [[nodiscard]] result<database_document> create_empty(
        std::string library_name,
        const format_profile& profile,
        const generation_context& generation) const;

    [[nodiscard]] result<database_document> create_empty(
        std::string library_name,
        const format_profile& profile,
        const hash58_device_key& device_key,
        const generation_context& generation) const;
};

}  // namespace foopodbridge::core::database

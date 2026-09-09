#pragma once

#include "foopodbridge/core/database/model.h"

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace foopodbridge::core::database {

struct comparison_report {
    bool equal{};
    std::vector<std::string> differences;
};

class comparator final {
public:
    [[nodiscard]] comparison_report exact_bytes(
        std::span<const std::byte> expected,
        std::span<const std::byte> actual) const;
    [[nodiscard]] comparison_report semantic(
        const database_model& expected,
        const database_model& actual) const;
    [[nodiscard]] comparison_report preservation(
        const database_document& original,
        std::span<const std::byte> output) const;
};

}  // namespace foopodbridge::core::database

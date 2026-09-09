#pragma once

#include "foopodbridge/core/database/error.h"
#include "foopodbridge/core/database/model.h"

#include <vector>

namespace foopodbridge::core::database {

struct validation_report {
    std::vector<diagnostic> warnings;
};

class validator final {
public:
    [[nodiscard]] result<validation_report> validate(const database_document& document) const;
};

}  // namespace foopodbridge::core::database

#pragma once
#include "foopodbridge/core/transaction/transaction.h"
#include "foopodbridge/core/database/reader.h"
#include "foopodbridge/core/database/validator.h"

namespace foopodbridge::core::transaction {
// The caller supplies the approved profile and, when required, its signing key.
database_validator traditional_database_validator(database::format_profile profile,
                                                  std::optional<database::hash58_device_key> key = {});
}

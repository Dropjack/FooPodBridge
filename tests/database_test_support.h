#pragma once

#include "foopodbridge/core/database/edit.h"
#include "foopodbridge/core/database/writer.h"

#include <stdexcept>
#include <string>

namespace foopodbridge::tests {

inline void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

[[nodiscard]] inline core::database::generation_context fixed_generation() {
    return core::database::generation_context{
        0x0102030405060708ULL,
        0x1112131415161718ULL,
        1'700'000'000U,
        {0x2122232425262728ULL, 0x3132333435363738ULL, 0x4142434445464748ULL,
         0x5152535455565758ULL, 0x6162636465666768ULL}};
}

[[nodiscard]] inline core::database::database_document make_empty() {
    const auto created = core::database::writer{}.create_empty(
        "Library", core::database::traditional_unsigned_profile(), fixed_generation());
    require(created.has_value(), "failed to create the synthetic empty database");
    return created.value();
}

}  // namespace foopodbridge::tests

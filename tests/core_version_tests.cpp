// SPDX-License-Identifier: LGPL-3.0-or-later
#include "foopodbridge/core/version.h"

int main() {
    const auto actual = foopodbridge::core::current_version();
    constexpr foopodbridge::core::version expected{0, 1, 0};
    return actual == expected ? 0 : 1;
}

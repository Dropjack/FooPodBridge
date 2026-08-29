// SPDX-License-Identifier: LGPL-3.0-or-later
#include "foobar/component_identity.h"

#include <cstring>

int main() {
    using namespace foopodbridge::identity;

    if (std::strcmp(component_name, "FooPodBridge") != 0) return 1;
    if (std::strcmp(component_filename, "foo_pod_bridge.dll") != 0) return 2;
    if (std::strcmp(display_version, "0.1.0-beta.1") != 0) return 3;
    static_assert(file_version_major == 0);
    static_assert(file_version_minor == 1);
    static_assert(file_version_patch == 0);
    static_assert(file_version_build == 0);
    static_assert(component_guid.Data1 != 0);
    return 0;
}

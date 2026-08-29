// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <guiddef.h>

namespace foopodbridge::identity {

inline constexpr char component_name[] = "FooPodBridge";
inline constexpr char component_filename[] = "foo_pod_bridge.dll";
inline constexpr char display_version[] = "0.1.0-beta.1";
inline constexpr char about_message[] =
    "Manual management bridge for supported disk-mode click-wheel iPods.\n"
    "Service contract: 1.0; device provider is introduced by a later task.";

inline constexpr unsigned file_version_major = 0;
inline constexpr unsigned file_version_minor = 1;
inline constexpr unsigned file_version_patch = 0;
inline constexpr unsigned file_version_build = 0;

inline constexpr GUID component_guid{
    0x3eac7bdd,
    0xebc1,
    0x4a2b,
    {0xbc, 0x28, 0x7a, 0x80, 0x6e, 0x5a, 0xfe, 0x37}
};

} // namespace foopodbridge::identity

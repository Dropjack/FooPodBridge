// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "foopodbridge/core/device/device.h"

namespace foopodbridge::core::device::detail {
// Internal query API, also exercised against disposable computer-side directories.
file_result read_relative(const std::wstring& root, const std::wstring& relative, std::size_t limit, std::stop_token cancel);
struct recovery_probe { std::uint32_t pending{}; std::uint32_t invalid{}; reason problem{reason::none}; };
recovery_probe probe_recovery(const std::wstring& root, std::stop_token cancel);
}

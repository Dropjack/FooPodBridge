// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <string>

namespace foopodbridge::core::device::detail {
// Both volume discovery and unmounted-node discovery must use the same ancestor.
// Node IDs are case-insensitive; keep the serial-bearing key internal only.
template<typename Node, typename ReadId, typename ReadParent>
std::wstring physical_ancestor(Node node, ReadId read_id, ReadParent read_parent) {
    for (unsigned depth = 0; depth < 16; ++depth) {
        auto id = read_id(node);
        for (auto& ch : id) if (ch >= L'a' && ch <= L'z') ch -= L'a' - L'A';
        if (id.starts_with(L"USB\\VID_05AC&PID_") || id.starts_with(L"1394\\APPLE_COMPUTER__INC.&IPOD")) return id;
        Node parent{};
        if (!read_parent(node, parent)) break;
        node = parent;
    }
    return {};
}
}

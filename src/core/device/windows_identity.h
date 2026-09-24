// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <string>
#include <utility>
#include <vector>

namespace foopodbridge::core::device::detail {
inline bool apple_node_id(std::wstring& id) {
    for (auto& ch : id) if (ch >= L'a' && ch <= L'z') ch -= L'a' - L'A';
    return id.starts_with(L"USB\\VID_05AC&PID_") || id.starts_with(L"1394\\APPLE_COMPUTER__INC.&IPOD");
}

// A composite iPod can have multiple Apple nodes on one disk's parent chain.
// Keep every alias so an already mounted parent is not reported as NotMounted.
template<typename Node, typename ReadId, typename ReadParent>
std::vector<std::wstring> physical_ancestors(Node node, ReadId read_id, ReadParent read_parent) {
    std::vector<std::wstring> ids;
    for (unsigned depth = 0; depth < 16; ++depth) {
        auto id = read_id(node);
        if (apple_node_id(id)) ids.push_back(std::move(id));
        Node parent{};
        if (!read_parent(node, parent)) break;
        node = parent;
    }
    return ids;
}

// Both volume discovery and unmounted-node discovery must use the same ancestor.
// Node IDs are case-insensitive; keep the serial-bearing key internal only.
template<typename Node, typename ReadId, typename ReadParent>
std::wstring physical_ancestor(Node node, ReadId read_id, ReadParent read_parent) {
    for (unsigned depth = 0; depth < 16; ++depth) {
        auto id = read_id(node);
        if (apple_node_id(id)) return id;
        Node parent{};
        if (!read_parent(node, parent)) break;
        node = parent;
    }
    return {};
}
}

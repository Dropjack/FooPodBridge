// SPDX-License-Identifier: 0BSD
#include "foopodbridge/service_v1.h"

#include <array>
#include <type_traits>

namespace {

bool all_guids_are_unique() {
    constexpr GUID null_guid{};
    const std::array guids{
        foopodbridge::contract::service_v1::class_guid,
        foopodbridge::contract::device_provider_v1::class_guid,
        foopodbridge::contract::device_snapshot_list_v1::class_guid,
        foopodbridge::contract::device_snapshot_v1::class_guid,
        foopodbridge::contract::capability_matrix_v1::class_guid,
        foopodbridge::contract::operation_request_v1::class_guid,
        foopodbridge::contract::operation_plan_v1::class_guid,
        foopodbridge::contract::operation_handle_v1::class_guid,
        foopodbridge::contract::operation_result_v1::class_guid,
        foopodbridge::contract::device_event_callback_v1::class_guid,
        foopodbridge::contract::subscription_v1::class_guid,
    };

    for (std::size_t left = 0; left < guids.size(); ++left) {
        if (IsEqualGUID(guids[left], null_guid)) return false;
        for (std::size_t right = left + 1; right < guids.size(); ++right) {
            if (IsEqualGUID(guids[left], guids[right])) return false;
        }
    }
    return true;
}

} // namespace

int main() {
    using namespace foopodbridge::contract;

    static_assert(abi_major_v1 == 1);
    static_assert(abi_minor_v1 == 0);
    static_assert(std::is_abstract_v<service_v1>);
    static_assert(std::is_base_of_v<service_base, service_v1>);
    static_assert(std::is_abstract_v<device_provider_v1>);
    static_assert(std::is_abstract_v<device_snapshot_v1>);
    static_assert(std::is_abstract_v<operation_handle_v1>);

    static_assert(static_cast<std::uint32_t>(runtime_state::ready_no_device_provider) == 1);
    static_assert(static_cast<std::uint64_t>(capability::read_library) == 1);
    if (!all_guids_are_unique()) return 3;
    return 0;
}

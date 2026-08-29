// SPDX-License-Identifier: LGPL-3.0-or-later
#include <windows.h>

#include <mmsystem.h>
#include <objidl.h>

#include <foobar2000/SDK/foobar2000.h>

#include "foopodbridge/core/version.h"
#include "foopodbridge/service_v1.h"
#include "foobar/component_identity.h"

DECLARE_COMPONENT_VERSION(
    foopodbridge::identity::component_name,
    foopodbridge::identity::display_version,
    foopodbridge::identity::about_message)

VALIDATE_COMPONENT_FILENAME(foopodbridge::identity::component_filename)

namespace {

class foo_pod_bridge_service_v1_impl : public foopodbridge::contract::service_v1 {
public:
    std::uint32_t get_contract_major() noexcept override {
        return foopodbridge::contract::abi_major_v1;
    }

    std::uint32_t get_contract_minor() noexcept override {
        return foopodbridge::contract::abi_minor_v1;
    }

    void get_component_version(pfc::string_base& out) override {
        out = foopodbridge::identity::display_version;
    }

    foopodbridge::contract::runtime_state get_runtime_state() noexcept override {
        const auto core_version = foopodbridge::core::current_version();
        PFC_ASSERT(core_version.major == 0 && core_version.minor == 1);
        return foopodbridge::contract::runtime_state::ready_no_device_provider;
    }

    bool get_device_provider(
        service_ptr_t<foopodbridge::contract::device_provider_v1>& out) noexcept override {
        out.release();
        return false;
    }
};

service_factory_single_t<foo_pod_bridge_service_v1_impl> g_service_factory;

} // namespace

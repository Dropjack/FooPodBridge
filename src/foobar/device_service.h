// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "foopodbridge/service_readonly.h"

namespace foopodbridge::adapter {
service_ptr_t<contract::device_provider_readonly_v1> provider();
bool running() noexcept;
}

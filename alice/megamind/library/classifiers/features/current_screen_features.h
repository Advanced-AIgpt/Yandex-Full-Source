#pragma once

#include <alice/megamind/protos/common/device_state.pb.h>

#include <kernel/factor_storage/factor_storage.h>

namespace NAlice {

void FillCurrentScreen(const TDeviceState& deviceState, const TFactorView view);

} // namespace NAlice

#pragma once

#include <alice/library/logger/logger.h>
#include <alice/megamind/protos/common/device_state.pb.h>

namespace NAlice::NHollywood {

std::pair<double, bool> VideoLastPlayTimestamp(TRTLogger& logger, const TDeviceState& deviceState);
std::pair<double, bool> MusicLastPlayTimestamp(TRTLogger& logger, const TDeviceState& deviceState);
std::pair<double, bool> BluetoothLastPlayTimestamp(TRTLogger& logger, const TDeviceState& deviceState);
std::pair<double, bool> RadioLastPlayTimestamp(TRTLogger& logger, const TDeviceState& deviceState);
std::pair<double, bool> AudioPlayerLastPlayTimestamp(TRTLogger& logger, const TDeviceState& deviceState);

} // namespace NAlice::NHollywood

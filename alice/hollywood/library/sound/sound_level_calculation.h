#pragma once

#include "device_volume.h"
#include "sound_common.h"

namespace NAlice::NHollywood::NSound {

i64 CalculateSoundLevelForSetLevel(const TFrame& frame, const TDeviceVolume& deviceSound, const ui64 onboardingUsageCounter = 0);

TMaybe<i64> CalculateSoundLevelForSetLevelOnOtherDevice(const TFrame& frame);

std::pair<i64, bool> CalculateSoundLevelForLouderOrQuiter(const TFrame& frame, const TDeviceVolume& deviceSound, bool isLouder, i64 defaultChangeSteps = 1);

} // namespace NAlice::NHollywood::NSound

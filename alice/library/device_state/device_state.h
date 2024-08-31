#pragma once

#include <alice/megamind/protos/common/device_state.pb.h>

namespace NAlice {

bool IsVideoPlaying(const TDeviceState& deviceState);
bool IsMusicPlaying(const TDeviceState& deviceState);
bool IsRadioPlaying(const TDeviceState& deviceState);
bool IsAudioPlaying(const TDeviceState& deviceState);
bool IsBluetoothPlaying(const TDeviceState& deviceState);
bool IsAlarmPlaying(const TDeviceState& deviceState);
bool IsTimerPlaying(const TDeviceState& deviceState);

enum class ECurrentlyPlaying {
    Nothing,
    Alarm,
    Timer,
    Video,
    Music,
    Audio,
    Radio,
    Bluetooth,
};

ECurrentlyPlaying GetCurrentlyPlaying(const TDeviceState& ds);

} // namespace NAlice

#include "device_state.h"

#include <google/protobuf/struct.pb.h>

namespace NAlice {

bool IsVideoPlaying(const TDeviceState& deviceState) {
    const auto& videoCurrentlyPlaying = deviceState.GetVideo().GetCurrentlyPlaying();
    return videoCurrentlyPlaying.HasPaused() && !videoCurrentlyPlaying.GetPaused();
}

bool IsMusicPlaying(const TDeviceState& deviceState) {
    const auto& music = deviceState.GetMusic();
    const auto& musicPlayer = music.GetPlayer();
    return musicPlayer.HasPause() && !musicPlayer.GetPause() && music.HasCurrentlyPlaying();
}

bool IsRadioPlaying(const TDeviceState& deviceState) {
    const auto& radio = deviceState.GetRadio();
    if (!radio.fields().count("player") || !radio.fields().count("currently_playing")) {
        return false;
    }

    const auto& radioPlayer = radio.fields().at("player").struct_value();
    if (!radioPlayer.fields().count("pause")) {
        return false;
    }

    return !radioPlayer.fields().at("pause").bool_value();
}

bool IsAudioPlaying(const TDeviceState& deviceState) {
    if (!deviceState.HasAudioPlayer()) {
        return false;
    }

    const auto& audioPlayerState = deviceState.GetAudioPlayer();
    return audioPlayerState.GetPlayerState() == TDeviceState_TAudioPlayer_TPlayerState_Playing;
}

bool IsBluetoothPlaying(const TDeviceState& deviceState) {
    const auto& bluetooth = deviceState.GetBluetooth();
    const auto& bluetoothPlayer = bluetooth.GetPlayer();
    return bluetoothPlayer.HasPause() && !bluetoothPlayer.GetPause() && bluetooth.HasCurrentlyPlaying();
}

bool IsAlarmPlaying(const TDeviceState& deviceState) {
    return deviceState.GetAlarmState().GetCurrentlyPlaying();
}

bool IsTimerPlaying(const TDeviceState& deviceState) {
    for (const auto& activeTimer : deviceState.GetTimers().GetActiveTimers()) {
        if (activeTimer.GetCurrentlyPlaying()) {
            return true;
        }
    }
    return false;
}

ECurrentlyPlaying GetCurrentlyPlaying(const TDeviceState& ds) {
    if (IsAlarmPlaying(ds)) {
        return ECurrentlyPlaying::Alarm;
    } else if (IsTimerPlaying(ds)) {
        return ECurrentlyPlaying::Timer;
    } else if (IsVideoPlaying(ds)) {
        return ECurrentlyPlaying::Video;
    } else if (IsMusicPlaying(ds)) {
        return ECurrentlyPlaying::Music;
    } else if (IsRadioPlaying(ds)) {
        return ECurrentlyPlaying::Radio;
    } else if (IsAudioPlaying(ds)) {
        return ECurrentlyPlaying::Audio;
    } else if (IsBluetoothPlaying(ds)) {
        return ECurrentlyPlaying::Bluetooth;
    }

    return ECurrentlyPlaying::Nothing;
}


} // namespace NAlice

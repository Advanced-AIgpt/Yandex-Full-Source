#include "last_play_timestamp.h"

#include <google/protobuf/struct.pb.h>

namespace NAlice::NHollywood {

std::pair<double, bool> VideoLastPlayTimestamp(TRTLogger& logger, const TDeviceState& deviceState) {
    if (!deviceState.HasVideo()) {
        return {0, false};
    }
    const auto& video = deviceState.GetVideo();
    if (!video.HasPlayer() || video.GetPlayer().GetPause()) {
        return {0, false};
    }
    LOG_INFO(logger) << "Video player LastPlayTimestamp=" << video.GetLastPlayTimestamp();
    return {video.GetLastPlayTimestamp(), true};
}

std::pair<double, bool> MusicLastPlayTimestamp(TRTLogger& logger, const TDeviceState& deviceState) {
    if (!deviceState.HasMusic()) {
        return {0, false};
    }
    const auto& music = deviceState.GetMusic();
    if (!music.HasPlayer() || music.GetPlayer().GetPause()) {
        return {0, false};
    }
    LOG_INFO(logger) << "Music player LastPlayTimestamp=" << music.GetLastPlayTimestamp();
    return {music.GetLastPlayTimestamp(), true};
}

std::pair<double, bool> BluetoothLastPlayTimestamp(TRTLogger& logger, const TDeviceState& deviceState) {
    if (!deviceState.HasBluetooth()) {
        return {0, false};
    }
    const auto& bluetooth = deviceState.GetBluetooth();
    if (!bluetooth.HasPlayer() || bluetooth.GetPlayer().GetPause()) {
        return {0, false};
    }
    LOG_INFO(logger) << "Bluetooth player LastPlayTimestamp=" << bluetooth.GetLastPlayTimestamp();
    return {bluetooth.GetLastPlayTimestamp(), true};
}

std::pair<double, bool> RadioLastPlayTimestamp(TRTLogger& logger, const TDeviceState& deviceState) {
    if (!deviceState.HasRadio()) {
        return {0, false};
    }
    const auto& radio = deviceState.GetRadio();
    if (radio.fields().count("player") == 0 || radio.fields().count("last_play_timestamp") == 0) {
        return {0, false};
    }
    const auto& player = radio.fields().at("player").struct_value();
    if (player.fields().count("pause") > 0 && player.fields().at("pause").bool_value()) {
        return {0, false};
    }
    auto result = radio.fields().at("last_play_timestamp").number_value();
    LOG_INFO(logger) << "Radio player LastPlayTimestamp=" << result;
    return {result, true};
}

std::pair<double, bool> AudioPlayerLastPlayTimestamp(TRTLogger& logger, const TDeviceState& deviceState) {
    if (!deviceState.HasAudioPlayer()) {
        return {0, false};
    }
    const auto& audioPlayer = deviceState.GetAudioPlayer();
    if (audioPlayer.GetPlayerState() != TDeviceState_TAudioPlayer_TPlayerState_Playing) {
        return {0, false};
    }
    LOG_INFO(logger) << "AudioPlayer (ThinClient) player LastPlayTimestamp=" << audioPlayer.GetLastPlayTimestamp();
    return {audioPlayer.GetLastPlayTimestamp(), true};
}

} // namespace NAlice::NHollywood

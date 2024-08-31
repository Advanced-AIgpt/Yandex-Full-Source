#include "player_features.h"
#include "util/generic/yexception.h"
#include <algorithm>

#include <alice/megamind/protos/common/device_state.pb.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

const auto ONE_WEEK_DURATION = TDuration::Days(7);

ui64 GetPlayerLastPlayTimestamp(const TScenarioBaseRequestWrapper& request, EPlayer player) {
    const auto& deviceState = request.BaseRequestProto().GetDeviceState();
    switch (player) {
        case EPlayer::MUSIC_PLAYER:
            return deviceState.GetMusic().GetLastPlayTimestamp();
        case EPlayer::BLUETOOTH_PLAYER:
            return deviceState.GetBluetooth().GetLastPlayTimestamp();
    }
    return 0;
}

} // namespace

TScenarioRunResponse_TFeatures_TPlayerFeatures CalcPlayerFeatures(TRTLogger& logger,
    TInstant nowTimestamp, TInstant lastPlayTimestamp)
{
    TScenarioRunResponse_TFeatures_TPlayerFeatures result;

    LOG_INFO(logger) << "Calculating PlayerFeatures with now=" << nowTimestamp << ", lastPlayTimestamp="
                     << lastPlayTimestamp;

    if (nowTimestamp < lastPlayTimestamp) {
        LOG_INFO(logger) << "durationSinceLastPlay is negative, so we adjust it to zero";
        result.SetRestorePlayer(true);
        result.SetSecondsSincePause(0);
        return result;
    }
    auto durationSinceLastPlay = nowTimestamp - lastPlayTimestamp;
    if (durationSinceLastPlay >= ONE_WEEK_DURATION) {
        LOG_INFO(logger) << "durationSinceLastPlay >= ONE_WEEK_DURATION is unacceptable";
        return result;
    }

    ui64 secondsSinceLastPlay = durationSinceLastPlay.Seconds();
    LOG_INFO(logger) << "Calculated seconds since last play for PlayerFeatures: " << secondsSinceLastPlay;
    result.SetRestorePlayer(true);

    // TODO(vitvlkv): At the moment everybody use secondsSinceLastPlay, because not all players support
    // LastStopTimestamp field in their device state. Of course it is better to calculate secondsSincePause...
    // But if you want to change this then make sure that every scenario will do the same.
    result.SetSecondsSincePause(secondsSinceLastPlay);
    return result;
}

TScenarioRunResponse_TFeatures_TPlayerFeatures CalcPlayerFeatures(TRTLogger& logger,
    const TScenarioBaseRequestWrapper& request, TInstant lastPlayTimestamp)
{
    return CalcPlayerFeatures(logger, TInstant::Seconds(request.ClientInfo().Epoch), lastPlayTimestamp);
}

TScenarioRunResponse_TFeatures_TPlayerFeatures CalcPlayerFeaturesForAudioPlayer(TRTLogger& logger,
    const TScenarioBaseRequestWrapper& request, IsOwnerOfPlayer isOwnerOfPlayer)
{
    TScenarioRunResponse_TFeatures_TPlayerFeatures result;
    const auto& deviceState = request.BaseRequestProto().GetDeviceState();
    if (!deviceState.HasAudioPlayer()) {
        LOG_INFO(logger) << "DeviceState has no AudioPlayer";
        return result;
    }
    if (!isOwnerOfPlayer(deviceState)) {
        LOG_INFO(logger) << "Not owner of the AudioPlayer";
        return result;
    }
    return CalcPlayerFeatures(logger, request,
                              TInstant::MilliSeconds(deviceState.GetAudioPlayer().GetLastPlayTimestamp()));
}

TScenarioRunResponse_TFeatures_TPlayerFeatures CalcPlayerFeaturesForMusicPlayer(TRTLogger& logger,
    const TScenarioBaseRequestWrapper& request, IsOwnerOfPlayer isOwnerOfPlayer)
{
    TScenarioRunResponse_TFeatures_TPlayerFeatures result;
    const auto& deviceState = request.BaseRequestProto().GetDeviceState();
    if (!deviceState.HasMusic()) {
        LOG_INFO(logger) << "DeviceState has no MusicPlayer";
        return result;
    }
    if (!isOwnerOfPlayer(deviceState)) {
        LOG_INFO(logger) << "Not owner of the MusicPlayer";
        return result;
    }

    return CalcPlayerFeatures(logger, request,
                              TInstant::MilliSeconds(deviceState.GetMusic().GetLastPlayTimestamp()));
}

TScenarioRunResponse_TFeatures_TPlayerFeatures
CalcPlayerFeaturesFor(TRTLogger& logger, const TScenarioBaseRequestWrapper& request, const THashSet<EPlayer>& players) {
    Y_ENSURE(!players.empty());
    TVector<ui64> lastPlayTimestamps;
    for (auto player : players) {
        lastPlayTimestamps.push_back(GetPlayerLastPlayTimestamp(request, player));
    }
    const auto mostRecentLastPlayTimestamp = *std::max_element(lastPlayTimestamps.begin(), lastPlayTimestamps.end());
    return CalcPlayerFeatures(logger, request, TInstant::MilliSeconds(mostRecentLastPlayTimestamp));
}

} // namespace NAlice::NHollywood

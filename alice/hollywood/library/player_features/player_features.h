#pragma once

#include <alice/library/logger/logger.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/hollywood/library/request/request.h>

namespace NAlice::NHollywood {

using IsOwnerOfPlayer = bool(*)(const TDeviceState&);

NScenarios::TScenarioRunResponse_TFeatures_TPlayerFeatures
CalcPlayerFeatures(TRTLogger& logger, TInstant nowTimestamp, TInstant lastPlayTimestamp);

NScenarios::TScenarioRunResponse_TFeatures_TPlayerFeatures
CalcPlayerFeatures(TRTLogger& logger, const TScenarioBaseRequestWrapper& request, TInstant lastPlayTimestamp);

NScenarios::TScenarioRunResponse_TFeatures_TPlayerFeatures
CalcPlayerFeaturesForAudioPlayer(TRTLogger& logger, const TScenarioBaseRequestWrapper& request, IsOwnerOfPlayer isOwnerOfPlayer);

NScenarios::TScenarioRunResponse_TFeatures_TPlayerFeatures
CalcPlayerFeaturesForMusicPlayer(TRTLogger& logger, const TScenarioBaseRequestWrapper& request, IsOwnerOfPlayer isOwnerOfPlayer);

enum class EPlayer {
    MUSIC_PLAYER,
    BLUETOOTH_PLAYER,
    // Feel free to add more here...
};

NScenarios::TScenarioRunResponse_TFeatures_TPlayerFeatures
CalcPlayerFeaturesFor(TRTLogger& logger, const TScenarioBaseRequestWrapper& request, const THashSet<EPlayer>& players);


} // namespace NAlice::NHollywood

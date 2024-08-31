#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/music/fm_radio_resources.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic::NFmRadio {

TMaybe<TMusicScenarioSceneArgsFmRadio> TryBuildSceneArgs(
    const TRunRequest&,
    const NHollywood::NMusic::TFmRadioResources&,
    const NHollywood::NMusic::TMusicQueueWrapper&);

TMaybe<TMusicScenarioSceneArgsFmRadio> TryBuildSceneArgsFromNextTrackCommand(
    const TRunRequest&,
    const TPlayerNextTrackSemanticFrame&,
    const NHollywood::NMusic::TMusicQueueWrapper&,
    NHollywood::NMusic::ETrackChangeResult);

TMaybe<TMusicScenarioSceneArgsFmRadio> TryBuildSceneArgsFromPrevTrackCommand(
    const TRunRequest&,
    const TPlayerPrevTrackSemanticFrame&,
    const NHollywood::NMusic::TMusicQueueWrapper&,
    NHollywood::NMusic::ETrackChangeResult);

TMaybe<TMusicScenarioSceneArgsFmRadio> TryBuildSceneArgsFromContinueCommand(
    const TRunRequest&,
    const TPlayerContinueSemanticFrame&,
    const NHollywood::NMusic::TMusicQueueWrapper&);

} // namespace NAlice::NHollywoodFw::NMusic::NFmRadio

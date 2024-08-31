#pragma once

#include <alice/hollywood/library/request/request.h>

namespace NAlice::NHollywood::NMusic {

ui32 GetTracksReaskCount(const TScenarioBaseRequestWrapper& request);

ui32 GetTracksReaskDelaySeconds(const TScenarioBaseRequestWrapper& request);
ui32 GetTracksReaskDelaySeconds(const TScenarioBaseRequestWrapper& request, ui32 defaultDelaySeconds);

NScenarios::TServerDirective MakeTracksMidrollDirective(const TScenarioBaseRequestWrapper& request, TStringBuf puid, ui32 trackIndex);
NScenarios::TServerDirective MakeTracksMidrollDirective(
    const TScenarioBaseRequestWrapper& request,
    TStringBuf puid,
    ui32 trackIndex,
    ui32 reaskDelaySeconds
);

} // namespace NAlice::NHollywood::NMusic

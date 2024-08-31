#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/response/response_builder.h>


namespace NAlice::NHollywood::NSound {

TMaybe<TString> RenderSoundChangeIfExists(TRTLogger& logger, const TScenarioBaseRequestWrapper& request,
                                          const TScenarioInputWrapper& requestInput, IResponseBuilder& builder);

void FillAnalyticsInfoForSoundChangeIfExists(TMaybe<TString> soundFrameName, TResponseBodyBuilder& bodyBuilder);

} // namespace NAlice::NHollywood::NSound

#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>

namespace NAlice::NHollywood::NMusicalClips {

void AddIrrelevantResponse(NAlice::NHollywood::TScenarioHandleContext& ctx);

bool CheckScenarioAppropriateSlots(TScenarioHandleContext& ctx, const TFrame* frame = nullptr);
NJson::TJsonValue GetCurrentlyPlaying(const TScenarioRunRequestWrapper& request);
inline TString MakeNumberedName (const TStringBuf& name, ui64 number) {
    return name + ToString("_") + ToString(number);
}

} // namespace NAlice::NHollywood::NMusicalClips

#pragma once

#include "fast_command_scenario_run_context.h"
#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NSound {

TMaybe<TFrame> GetSoundFrame(const TMaybe<TFrame>& frame, const TScenarioInputWrapper& input);
TMaybe<TFrame> GetNluHintFrame(const TScenarioInputWrapper& input);

void ProcessSoundRequest(TFastCommandScenarioRunContext& fastCommandScenarioRunContext, const TFrame& frame);

} // namespace NAlice::NHollywood::NSound

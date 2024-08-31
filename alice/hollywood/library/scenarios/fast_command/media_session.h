#pragma once

#include "fast_command_scenario_run_context.h"
#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/frame/frame.h>

namespace NAlice::NHollywood::NMediaSession {

TMaybe<TFrame> GetMediaSessionFrame(const TMaybe<TFrame>& frame, const TScenarioInputWrapper& input);

void ProcessMediaSessionCommand(TFastCommandScenarioRunContext& fastCommandScenarioRunContext, const TFrame& frame);

} // namespace NAlice::NHollywood::NMediaSession

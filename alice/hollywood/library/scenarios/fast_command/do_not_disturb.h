#pragma once

#include "fast_command_scenario_run_context.h"
#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/frame/frame.h>

namespace NAlice::NHollywood::NDoNotDisturb {

TMaybe<TFrame> GetDoNotDisturbFrame(const TMaybe<TFrame>& frame, const TScenarioInputWrapper& input, const TScenarioRunRequestWrapper& request);

void ProcessDoNotDisturbCommand(TFastCommandScenarioRunContext& fastCommandScenarioRunContext, const TFrame& frame);

} // namespace NAlice::NHollywood::NDoNotDisturb

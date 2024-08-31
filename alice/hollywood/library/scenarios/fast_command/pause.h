#pragma once

#include "fast_command_scenario_run_context.h"
#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/frame/frame.h>

namespace NAlice::NHollywood::NPause {

TMaybe<TFrame> GetPauseFrame(const TMaybe<TFrame>& frame, const TScenarioInputWrapper& input,
                             const TClientInfo& clientInfo, const TScenarioRunRequestWrapper& request);

void ProcessFastPauseCommand(TFastCommandScenarioRunContext& fastCommandScenarioRunContext, const TFrame& frame);

} // namespace NAlice::NHollywood::NPause

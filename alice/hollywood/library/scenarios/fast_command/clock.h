#pragma once

#include "fast_command_scenario_run_context.h"
#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NClock {

bool TryCreateClockResponse(TFastCommandScenarioRunContext& fastCommandScenarioRunContext);

} // namespace NAlice::NHollywood::NClock

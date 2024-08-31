#pragma once

#include "common.h"

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood {

bool CanLaunchWebOSAppDirective(const TScenarioRunRequestWrapper& request);

EOpenExternalAppResult MakeWebOSLaunchAppDirective(
    const TScenarioRunRequestWrapper& request, TRTLogger& logger,
    const TString& recognizedPackageName, TMaybe<NScenarios::TDirective>& maybeDirective);

} // namespace NAlice::NHollywood

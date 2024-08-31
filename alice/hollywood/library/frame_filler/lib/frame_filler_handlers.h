#pragma once

#include "frame_filler_scenario_handlers.h"

#include <alice/hollywood/library/request/request.h>

#include <alice/library/logger/logger.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NAlice {
namespace NFrameFiller {

inline const TString ON_SUBMIT_CALLBACK_NAME = "mm_on_submit";

NScenarios::TScenarioRunResponse Run(
    const NHollywood::TScenarioRunRequestWrapper& request,
    const IFrameFillerScenarioRunHandler& handler,
    TRTLogger& logger
);

NScenarios::TScenarioCommitResponse Commit(
    const NHollywood::TScenarioApplyRequestWrapper& request,
    const IFrameFillerScenarioCommitHandler& handler,
    TRTLogger& logger
);

NScenarios::TScenarioApplyResponse Apply(
    const NHollywood::TScenarioApplyRequestWrapper& request,
    const IFrameFillerScenarioApplyHandler& handler,
    TRTLogger& logger
);

} // namespace NFrameFiller
} // namespace NAlice

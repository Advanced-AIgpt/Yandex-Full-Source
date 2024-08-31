#include "requestctx.h"

#include "request_frame_to_scenario_matcher.h"
#include "talkien.h"

#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/new_modifiers/utils.h>

#include <alice/megamind/protos/modifiers/modifier_response.pb.h>

#include <alice/library/network/headers.h>

namespace NAlice {

void IRunWalkerRequestCtx::SavePostClassifyState(
    const TWalkerResponse& /* walkerResponse */,
    const NMegamind::TMegamindAnalyticsInfoBuilder& /* analyticsInforBuilder */, TStatus /* postClassifyError */,
    TScenarioWrapperPtr /* winnerScenario */, const TRequest& /* request */) {
    ythrow yexception() << "Should not be called except for WALKER_POSTCLASSIFY apphost node";
}

void IRunWalkerRequestCtx::SaveCombinatorState(const NMegamind::TCombinatorResponse& /* combinatorResponse */,
                                               const TRequest& /* request */) {
    ythrow yexception() << "Should not be called except for WALKER_POST_CLASSIFY apphost node";
}

NMegamind::IPostClassifyState& ILightWalkerRequestCtx::PostClassifyState() {
    ythrow yexception() << "Called postClassify state from node that was not supposed to do that";
}

} // namespace NAlice

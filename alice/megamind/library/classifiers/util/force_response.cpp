#include "force_response.h"

namespace NAlice {

bool ShouldForceResponseForEmptyRequestedSlot(const IContext& ctx, const TString& name) {
    const i32 maxConsequentIrrelevantResponseCount =
        ctx.ScenarioConfig(name).GetDialogManagerParams().GetMaxConsequentIrrelevantResponseCount();
    const bool infiniteConsequentIrrelevantResponseCountAllowed = (maxConsequentIrrelevantResponseCount == -1);
    return infiniteConsequentIrrelevantResponseCountAllowed ||
        (ctx.Session()->GetPreviousScenarioSession().GetConsequentIrrelevantResponseCount() <
            maxConsequentIrrelevantResponseCount);
}

bool ShouldForceResponseForFilledUntypedRequestedSlot(const IContext& ctx, const TString& name) {
    const i32 maxConsequentUntypedSlotRequests =
        ctx.ScenarioConfig(name).GetDialogManagerParams().GetMaxConsequentUntypedSlotRequests();
    const bool infiniteConsequentUntypedSlotRequests = (maxConsequentUntypedSlotRequests == -1);
    return infiniteConsequentUntypedSlotRequests ||
        (ctx.Session()->GetPreviousScenarioSession().GetConsequentUntypedSlotRequests() <
            maxConsequentUntypedSlotRequests);
}

} // namespace NAlice

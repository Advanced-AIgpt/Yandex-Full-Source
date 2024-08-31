#include "scenario_visitor.h"

namespace NAlice {

void TScenarioWrapperFactory::Visit(const TConfigBasedAppHostProxyProtocolScenario& scenario) const {
    Result = MakeIntrusive<TAppHostProxyProtocolScenarioWrapper>(scenario, Ctx, SemanticFrames,
                                                                 GuidGenerator, DeferApplyMode,
                                                                 false /* restoreAllFromSession */,
                                                                 ItemProxyAdapter);
}

void TScenarioWrapperFactory::Visit(const TConfigBasedAppHostPureProtocolScenario& scenario) const {
    Result = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(scenario, Ctx, SemanticFrames,
                                                                GuidGenerator, DeferApplyMode,
                                                                false /* restoreAllFromSession */,
                                                                ItemProxyAdapter);
}

} // namespace NAlice

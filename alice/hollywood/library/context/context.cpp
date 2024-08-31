#include "context.h"

namespace NAlice::NHollywood {

TContext::TContext(IGlobalContext& globalContext, TRTLogger& logger, const IResourceContainer* scenarioResources,
                   const TCompiledNlgComponent* scenarioNlg)
    : GlobalContext_(globalContext)
    , Logger_(logger)
    , ScenarioResources_(scenarioResources)
    , ScenarioNlg_(scenarioNlg)
{
}

} // namespace NAlice::NHollywood

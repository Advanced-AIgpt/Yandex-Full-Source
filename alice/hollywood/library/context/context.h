#pragma once

#include <alice/library/logger/logger.h>

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg.h>
#include <alice/hollywood/library/resources/resources.h>

namespace NAlice::NHollywood {

// We've desided that TContext can only have fields and functions that
// have solely infrastructural value and can be easily stubbed as NoOp.
// Everything else goes to TRequest
class TContext {
public:
    TContext(IGlobalContext& globalContext, TRTLogger& logger, const IResourceContainer* scenarioResources,
             const TCompiledNlgComponent* scenarioNlg);

    IGlobalContext& GlobalContext() {
        return GlobalContext_;
    }

    TRTLogger& Logger() {
        return Logger_;
    }

    const TCompiledNlgComponent& Nlg() {
        Y_ENSURE(ScenarioNlg_, "TContext object got nullptr for ScenarioNlg_");
        return *ScenarioNlg_;
    }

    template <typename TScenarioResources>
    const TScenarioResources& ScenarioResources() const {
        static_assert(std::is_base_of_v<IResourceContainer, TScenarioResources>);
        Y_ENSURE(ScenarioResources_);
        return dynamic_cast<const TScenarioResources&>(*ScenarioResources_);
    }

private:
    IGlobalContext& GlobalContext_;
    TRTLogger& Logger_;
    const IResourceContainer* ScenarioResources_;
    const TCompiledNlgComponent* ScenarioNlg_;
};

} // namespace NAlice::NHollywood

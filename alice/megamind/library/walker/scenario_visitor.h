#pragma once

#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/response/builder.h>
#include <alice/megamind/library/scenarios/helpers/interface/scenario_ref.h>
#include <alice/megamind/library/scenarios/helpers/interface/scenario_wrapper.h>
#include <alice/megamind/library/scenarios/helpers/scenario_wrapper.h>


namespace NAlice {

class TScenarioWrapperFactory : public IScenarioVisitor {
public:
    TScenarioWrapperFactory(const IContext& ctx,
                            const IScenarioWrapper::TSemanticFrames& semanticFrames,
                            const NMegamind::IGuidGenerator& guidGenerator,
                            EDeferredApplyMode deferApplyMode,
                            TScenarioWrapperPtr& wrapper,
                            NMegamind::TItemProxyAdapter& itemProxyAdapter)
        : Ctx(ctx)
        , SemanticFrames(semanticFrames)
        , GuidGenerator(guidGenerator)
        , DeferApplyMode(deferApplyMode)
        , Result(wrapper)
        , ItemProxyAdapter(itemProxyAdapter)
    {
    }

    void Visit(const TConfigBasedAppHostProxyProtocolScenario& scenario) const override;
    void Visit(const TConfigBasedAppHostPureProtocolScenario& scenario) const override;

private:
    const IContext& Ctx;
    const IScenarioWrapper::TSemanticFrames& SemanticFrames;
    const NMegamind::IGuidGenerator& GuidGenerator;
    EDeferredApplyMode DeferApplyMode;
    TScenarioWrapperPtr& Result;
    NMegamind::TItemProxyAdapter& ItemProxyAdapter;
};

} // namespace NAlice

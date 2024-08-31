#include "apply.h"
#include "run.h"
#include <alice/hollywood/library/scenarios/market/how_much/nlg/register.h>
#include <alice/hollywood/library/scenarios/market/common/fast_data.h>
#include <alice/hollywood/library/scenarios/market/common/report/handle.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

namespace NAlice::NHollywood::NMarket::NHowMuch {

class TRunHandle : public TScenario::THandleBase {
public:
    TRunHandle()
    {
        NNlu::TRequestNormalizer::WarmUpSingleton();
    }

    TString Name() const override
    {
        return "run";
    }

    void Do(TScenarioHandleContext& ctx) const final
    {
        TMarketRunContext marketCtx { ctx };
        TRunImpl impl { marketCtx };
        impl.Do();
    }
};

class TApplyPrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override
    {
        return "apply_prepare";
    }

    void Do(TScenarioHandleContext& ctx) const final
    {
        TMarketApplyContext marketCtx { ctx };
        TApplyPrepareImpl impl { marketCtx };
        impl.Do();
    }
};

class TApplyRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override
    {
        return "apply_render";
    }

    void Do(TScenarioHandleContext& ctx) const final
    {
        TMarketApplyContext marketCtx { ctx };
        TApplyRenderImpl impl { marketCtx };
        impl.Do();
    }

};

REGISTER_SCENARIO(
    "market_how_much",
    AddHandle<TRunHandle>()
    .AddHandle<TApplyPrepareHandle>()
    .AddHandle<TApplyRenderHandle>()
    .AddHandle<TReportResponseHandle>()
    .AddHandle<TReportResponseHandleNoRedir>()
    .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NMarket::NHowMuch::NNlg::RegisterAll)
    .AddFastData<NProto::TMarketFastData, TMarketFastData>("market/market.pb")
);

}  // namespace NAlice::NHollywood::NMarket::NHowMuch

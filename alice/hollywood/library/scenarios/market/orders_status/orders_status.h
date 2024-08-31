#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

#include <alice/hollywood/library/scenarios/market/common/handle.h>
#include <alice/hollywood/library/scenarios/market/nlg/register.h>

namespace NAlice::NHollywood::NMarket {

class TOrdersStatusRunHandle : public TScenario::THandleBase {
    class TImpl : public TDeprecatedMarketRunHandleImplBase {
    public:
        using TDeprecatedMarketRunHandleImplBase::TDeprecatedMarketRunHandleImplBase;
        void Do() override final;
    private:
        void ReturnIrrelevant();
    };
public:
    TString Name() const override {
        return "run";
    }

    void Do(TScenarioHandleContext& ctx) const override
    {
        TImpl(ctx).Do();
    }
};

class TOrdersStatusApplyPrepareHandle : public TScenario::THandleBase {
    class TImpl : public TDeprecatedMarketApplyHandleImplBase {
    public:
        using TDeprecatedMarketApplyHandleImplBase::TDeprecatedMarketApplyHandleImplBase;
        void Do() override final;
    };
public:
    TString Name() const override {
        return "apply_prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override
    {
        TImpl(ctx).Do();
    }
};

class TOrdersStatusApplyRenderHandle : public TScenario::THandleBase {
    class TImpl : public TDeprecatedMarketApplyHandleImplBase {
    public:
        using TDeprecatedMarketApplyHandleImplBase::TDeprecatedMarketApplyHandleImplBase;
        void Do() override final;
    };
public:
    TString Name() const override {
        return "apply_render";
    }

    void Do(TScenarioHandleContext& ctx) const override
    {
        TImpl(ctx).Do();
    }
};

}  // namespace NAlice::NHollywood::NMarket


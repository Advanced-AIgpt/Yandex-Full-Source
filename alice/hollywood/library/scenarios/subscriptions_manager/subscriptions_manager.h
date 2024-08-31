#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

#include <alice/hollywood/library/scenarios/subscriptions_manager/nlg/register.h>

namespace NAlice::NHollywood {

class TSubscriptionsManagerRunHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "run";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

class TSubscriptionsManagerContinueHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "continue";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

class TSubscriptionsManagerRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood

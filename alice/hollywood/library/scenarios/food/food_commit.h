#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NFood {

class TCommitDispatchHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "commit_dispatch";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

class TSyncCartProxyPrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "sync_cart_proxy_prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

class TSyncCartResponseHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "sync_cart_response";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NFood

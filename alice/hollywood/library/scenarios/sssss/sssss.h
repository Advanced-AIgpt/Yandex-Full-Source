#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood {

class TSimpleStupidRunHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "run";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

class TSimpleStupidApplyHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "apply";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

class TSimpleStupidCommitHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "commit";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood

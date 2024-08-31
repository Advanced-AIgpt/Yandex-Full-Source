#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NVideo {

class TVideoSearchPrepare : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "search/prepare";
    }
    void Do(TScenarioHandleContext& ctx) const override;
};

class TVideoSearchProcess : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "search/process";
    }
    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NVideo

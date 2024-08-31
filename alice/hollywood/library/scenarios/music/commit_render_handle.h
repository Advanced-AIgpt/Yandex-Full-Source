#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NMusic {

class TCommitRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "commit_render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

class TCommitMusicHardcodedRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "commit_render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusic

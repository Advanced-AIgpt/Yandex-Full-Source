#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NMusic {

class TCommitPrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "commit_prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

class TCommitMusicHardcodedPrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "commit_prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusic

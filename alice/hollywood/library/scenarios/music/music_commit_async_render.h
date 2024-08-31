#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NMusic {

class TMusicCommitAsyncRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "commit_async_render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusic

#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NTvChannelsEfir {

class TTvChannelsEfirRenderHandle final : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NTvChannelsEfir

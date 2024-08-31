#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NTvChannelsEfir {

class TPlayTvChannelPlayerSetup final : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "vh_player_setup";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NTvChannelsEfir

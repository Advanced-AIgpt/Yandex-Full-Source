#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/scenarios/goods/nlg/register.h>

namespace NAlice::NHollywood {
    class TGoodsHandle: public TScenario::THandleBase {
    public:
        TString Name() const override {
            return "run";
        }

        void Do(TScenarioHandleContext& ctx) const override;
    };

}

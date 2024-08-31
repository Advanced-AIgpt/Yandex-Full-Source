#pragma once

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/base_scenario/scenario.h>

#include <alice/megamind/protos/scenarios/request.pb.h>

namespace NAlice::NHollywood {

class TBassShowTrafficPrepareHandle : public TScenario::THandleBase {
    TString Name() const override {
        return "prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood

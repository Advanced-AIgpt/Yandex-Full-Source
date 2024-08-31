#pragma once

#include <alice/nlu/libs/request_normalizer/request_normalizer.h>
#include <alice/hollywood/library/base_scenario/scenario.h>

using namespace NAlice::NHollywood;
namespace NAlice::NHollywood::NTvChannels {

class TSwitchTvChannelPrepareHandle : public TScenario::THandleBase {
public:
    TSwitchTvChannelPrepareHandle() {
        NNlu::TRequestNormalizer::WarmUpSingleton();
    }

    TString Name() const override {
        return "prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NTvChannels

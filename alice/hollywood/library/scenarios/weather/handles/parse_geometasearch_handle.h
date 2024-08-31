#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NWeather {

class TWeatherParseGeometasearchHandle final : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "parse_geometasearch";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

}  // namespace NAlice::NHollywood::NWeather

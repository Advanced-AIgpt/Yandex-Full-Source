#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NWeather {

class TWeatherPrepareForecastHandle final : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "prepare_forecast";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

}  // namespace NAlice::NHollywood::NWeather

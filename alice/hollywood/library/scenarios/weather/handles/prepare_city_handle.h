#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/scenarios/weather/util/error.h>

namespace NAlice::NHollywood::NWeather {

class TWeatherPrepareCityHandle final : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "prepare_city";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

// GeoId clarifiers
NGeobase::TId ClarifyGeoId(const NGeobase::TLookup& lookup, NGeobase::TId id);
NGeobase::TId FixGeoIdIfCityState(NGeobase::TId id);

}  // namespace NAlice::NHollywood::NWeather

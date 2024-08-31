#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/scenarios/weather/util/error.h>

#include "request_helper.h"

namespace NAlice::NHollywood::NWeather {

class TBeforeGeometasearchRequestHelper {
public:
    TBeforeGeometasearchRequestHelper(TScenarioHandleContext& ctx);
    void AddRequest(const TStringBuf text);

private:
    TScenarioHandleContext& Ctx_;
};

class TAfterGeometasearchRequestHelper {
public:
    TAfterGeometasearchRequestHelper(TScenarioHandleContext& ctx);
    TWeatherErrorOr<NGeobase::TId> TryParseGeoId(const TStringBuf text) const;

private:
    TScenarioHandleContext& Ctx_;
};

template<ERequestPhase RequestPhase>
using TGeometasearchRequestHelper = TRequestHelperChooser<RequestPhase,
                                                          TBeforeGeometasearchRequestHelper,
                                                          TAfterGeometasearchRequestHelper>;

} // NAlice::NHollywood::NWeather

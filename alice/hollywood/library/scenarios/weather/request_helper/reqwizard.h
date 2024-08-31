#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/scenarios/weather/util/error.h>

#include "request_helper.h"

namespace NAlice::NHollywood::NWeather {

class TBeforeReqwizardRequestHelper {
public:
    TBeforeReqwizardRequestHelper(TScenarioHandleContext& ctx);
    void AddRequest(const TStringBuf text);

private:
    TScenarioHandleContext& Ctx_;
};

class TAfterReqwizardRequestHelper {
public:
    TAfterReqwizardRequestHelper(TScenarioHandleContext& ctx);
    TWeatherErrorOr<NGeobase::TId> TryParseGeoId() const;

private:
    TScenarioHandleContext& Ctx_;
};

template<ERequestPhase RequestPhase>
using TReqwizardRequestHelper = TRequestHelperChooser<RequestPhase,
                                                      TBeforeReqwizardRequestHelper,
                                                      TAfterReqwizardRequestHelper>;

} // NAlice::NHollywood::NWeather

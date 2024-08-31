#include "parse_reqwizard_handle.h"

#include <alice/hollywood/library/scenarios/weather/request_helper/reqwizard.h>
#include <alice/hollywood/library/scenarios/weather/util/util.h>

namespace NAlice::NHollywood::NWeather {

void TWeatherParseReqwizardHandle::Do(TScenarioHandleContext& ctx) const {
    const TReqwizardRequestHelper<ERequestPhase::After> reqWizard{ctx};
    TryAddWeatherGeoId(ctx.Ctx.Logger(), ctx.ServiceCtx, reqWizard.TryParseGeoId());
}

}  // namespace NAlice::NHollywood::NWeather

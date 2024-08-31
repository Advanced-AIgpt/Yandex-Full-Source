#include "prepare_handle.h"
#include "render_handle.h"

#include <alice/hollywood/library/registry/registry.h>

#include <alice/hollywood/library/scenarios/tr_navi/get_weather/nlg/register.h>

namespace NAlice::NHollywood::NTrNavi {

REGISTER_SCENARIO("get_weather_tr",
                  AddHandle<TGetWeatherTrPrepareHandle>()
                  .AddHandle<TGetWeatherTrRenderHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NTrNavi::NGetWeather::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NTrNavi

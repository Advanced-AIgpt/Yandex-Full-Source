#include <alice/hollywood/library/scenarios/weather/handles/parse_geometasearch_handle.h>
#include <alice/hollywood/library/scenarios/weather/handles/parse_reqwizard_handle.h>
#include <alice/hollywood/library/scenarios/weather/handles/prepare_city_handle.h>
#include <alice/hollywood/library/scenarios/weather/handles/prepare_common_handle.h>
#include <alice/hollywood/library/scenarios/weather/handles/prepare_forecast_handle.h>
#include <alice/hollywood/library/scenarios/weather/handles/render_handle.h>

#include <alice/hollywood/library/registry/registry.h>

#include <alice/hollywood/library/scenarios/weather/nlg/register.h>

namespace NAlice::NHollywood::NWeather {

REGISTER_SCENARIO("weather",
                  AddHandle<TWeatherPrepareCommonHandle>()
                  .AddHandle<TWeatherPrepareCityHandle>()
                  .AddHandle<TWeatherPrepareForecastHandle>()
                  .AddHandle<TWeatherRenderHandle>()
                  .AddHandle<TWeatherParseGeometasearchHandle>()
                  .AddHandle<TWeatherParseReqwizardHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NWeather::NNlg::RegisterAll));

} // namespace NAlice::NHollywood

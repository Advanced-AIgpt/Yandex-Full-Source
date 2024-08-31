#pragma once

#include "api.h"
#include <alice/bass/forms/context/fwd.h>
#include <alice/bass/util/error.h>

namespace NBASS::NWeather {

bool IsCurrentWeatherScenario(TContext& ctx, const TForecast& forecast);
TResultValue PrepareCurrentForecastDivCard(TContext& ctx, const TForecast& forecast);
TResultValue PrepareCurrentForecastSlots(TContext& ctx, const TForecast& forecast);

} // namespace NBASS::NWeather

#pragma once

#include "api.h"
#include <alice/bass/forms/context/fwd.h>
#include <alice/bass/util/error.h>

namespace NBASS::NWeather {

bool IsDayWeatherScenario(TContext& ctx, const TForecast& forecast);
TResultValue PrepareDayForecastSlots(TContext& ctx, const TForecast& forecast);

} // namespace NBASS::NWeather

#pragma once

#include "api.h"
#include <alice/bass/forms/context/fwd.h>
#include <alice/bass/util/error.h>

namespace NBASS::NWeather {

bool IsDaysRangeWeatherScenario(TContext& ctx, const TForecast& forecast);
TResultValue PrepareDaysRangeForecastSlots(TContext& ctx, const TForecast& forecast);

} // namespace NBASS::NWeather

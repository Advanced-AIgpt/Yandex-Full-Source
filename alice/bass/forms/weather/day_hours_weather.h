#pragma once

#include "api.h"
#include <alice/bass/forms/context/fwd.h>
#include <alice/bass/util/error.h>

namespace NBASS::NWeather {

bool IsDayHoursWeatherScenario(TContext& ctx, const TForecast& forecast);
TResultValue PrepareDayHoursForecastSlots(TContext& ctx, const TForecast& forecast);

} // namespace NBASS::NWeather

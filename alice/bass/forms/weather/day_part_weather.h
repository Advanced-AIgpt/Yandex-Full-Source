#pragma once

#include "api.h"
#include <alice/bass/forms/context/fwd.h>
#include <alice/bass/util/error.h>

namespace NBASS::NWeather {

bool IsDayPartScenario(TContext& ctx, const TForecast& forecast);
TResultValue PrepareDayPartForecastSlots(TContext& ctx, const TForecast& forecast);

} // namespace NBASS::NWeather

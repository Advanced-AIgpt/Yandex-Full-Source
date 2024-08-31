#pragma once

#include "api.h"
#include <alice/bass/forms/context/fwd.h>
#include <alice/bass/util/error.h>
#include <util/generic/maybe.h>

namespace NBASS::NWeather {

bool IsTodayWeatherScenario(TContext& ctx, const TForecast& forecast);
TResultValue PrepareTodayForecastsSlots(TContext& ctx, const TForecast& forecast, const TMaybe<TNowcast> nowcastMaybe);

} // namespace NBASS::NWeather


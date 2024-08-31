#pragma once

#include <alice/hollywood/library/scenarios/weather/context/context.h>

namespace NAlice::NHollywood::NWeather {

bool IsDaysRangeWeatherScenario(const TWeatherContext& ctx);
[[nodiscard]] TWeatherStatus PrepareDaysRangeForecastSlots(TWeatherContext& ctx);

} // namespace NAlice::NHollywood::NWeather

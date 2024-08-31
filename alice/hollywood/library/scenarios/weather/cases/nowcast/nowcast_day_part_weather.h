#pragma once

#include <alice/hollywood/library/scenarios/weather/context/context.h>

namespace NAlice::NHollywood::NWeather {

bool IsDayPartForecastCase(const TWeatherContext& ctx);
[[nodiscard]] TWeatherStatus PrepareNowcastDayPartForecastSlot(TWeatherContext& ctx);

} // namespace NAlice::NHollywood::NWeather

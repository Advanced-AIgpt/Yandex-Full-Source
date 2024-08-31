#pragma once

#include <alice/hollywood/library/scenarios/weather/context/context.h>

namespace NAlice::NHollywood::NWeather {

bool IsByHoursForecastCase(const TWeatherContext& ctx);
[[nodiscard]] TWeatherStatus PrepareNowcastByHoursForecastSlot(TWeatherContext& ctx);

} // namespace NAlice::NHollywood::NWeather

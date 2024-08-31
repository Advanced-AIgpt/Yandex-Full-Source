#pragma once

#include <alice/hollywood/library/scenarios/weather/context/context.h>

namespace NAlice::NHollywood::NWeather {

bool IsDayPartWeatherScenario(const TWeatherContext& ctx);
[[nodiscard]] TWeatherStatus PrepareDayPartForecastSlots(TWeatherContext& ctx);

} // namespace NAlice::NHollywood::NWeather

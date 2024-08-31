#pragma once

#include <alice/hollywood/library/scenarios/weather/context/context.h>

namespace NAlice::NHollywood::NWeather {

bool IsDayWeatherScenario(const TWeatherContext& ctx);
[[nodiscard]] TWeatherStatus PrepareDayForecastSlots(TWeatherContext& ctx);

} // namespace NAlice::NHollywood::NWeather

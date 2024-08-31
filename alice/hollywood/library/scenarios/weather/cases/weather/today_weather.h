#pragma once

#include <alice/hollywood/library/scenarios/weather/context/context.h>

namespace NAlice::NHollywood::NWeather {

bool IsTodayWeatherScenario(const TWeatherContext& ctx);
[[nodiscard]] TWeatherStatus PrepareTodayForecastSlots(TWeatherContext& ctx);

} // namespace NAlice::NHollywood::NWeather

#pragma once

#include <alice/hollywood/library/scenarios/weather/context/context.h>

namespace NAlice::NHollywood::NWeather {

[[nodiscard]] TWeatherStatus PrepareDayPartWindForecastSlots(TWeatherContext& ctx);

} // namespace NAlice::NHollywood::NWeather

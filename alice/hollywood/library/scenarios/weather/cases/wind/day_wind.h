#pragma once

#include <alice/hollywood/library/scenarios/weather/context/context.h>

namespace NAlice::NHollywood::NWeather {

[[nodiscard]] TWeatherStatus PrepareDayWindForecastSlots(TWeatherContext& ctx);

} // namespace NAlice::NHollywood::NWeather

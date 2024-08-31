#pragma once

#include <alice/hollywood/library/scenarios/weather/context/context.h>

namespace NAlice::NHollywood::NWeather {

bool IsDayHoursWeatherScenario(const TWeatherContext& ctx);
[[nodiscard]] TWeatherStatus PrepareDayHoursForecastSlots(TWeatherContext& ctx);

} // namespace NAlice::NHollywood::NWeather

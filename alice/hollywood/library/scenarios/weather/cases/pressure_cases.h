#pragma once

#include <alice/hollywood/library/scenarios/weather/context/context.h>

namespace NAlice::NHollywood::NWeather {

[[nodiscard]] TWeatherStatus PrepareCurrentPressureForecastSlots(TWeatherContext& ctx);
[[nodiscard]] TWeatherStatus PrepareDayPartPressureForecastSlots(TWeatherContext& ctx);
[[nodiscard]] TWeatherStatus PrepareDayPressureForecastSlots(TWeatherContext& ctx);
[[nodiscard]] TWeatherStatus PrepareDaysRangePressureForecastSlots(TWeatherContext& ctx);
[[nodiscard]] TWeatherStatus PrepareTodayPressureForecastSlots(TWeatherContext& ctx);

} // namespace NAlice::NHollywood::NWeather

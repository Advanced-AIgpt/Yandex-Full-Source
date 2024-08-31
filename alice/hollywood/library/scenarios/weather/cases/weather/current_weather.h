#pragma once

#include <alice/hollywood/library/scenarios/weather/context/context.h>
#include <alice/hollywood/library/scenarios/weather/context/weather_protos.h>

namespace NAlice::NHollywood::NWeather {

constexpr size_t DAYS_COUNT_IN_CURRENT_WEATHER = 7;

bool IsCurrentWeatherScenario(const TWeatherContext& ctx);
[[nodiscard]] TMaybe<TWeatherError> PrepareCurrentForecastDivCard(TWeatherContext& ctx);
[[nodiscard]] TWeatherStatus PrepareCurrentForecastSlots(TWeatherContext& ctx);
NRenderer::TDivRenderData GenerateCurrentWeatherRenderData(const TWeatherContext& ctx, const THour& hour);

} // namespace NAlice::NHollywood::NWeather

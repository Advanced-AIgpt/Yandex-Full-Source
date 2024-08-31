#pragma once

#include <alice/hollywood/library/scenarios/weather/context/context.h>

namespace NAlice::NHollywood::NWeather {

TWeatherErrorOr<bool> IsGetWeatherScenario(const TWeatherContext& ctx);

} // namespace NAlice::NHollywood::NWeather

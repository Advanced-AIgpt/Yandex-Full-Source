#pragma once

#include <alice/hollywood/library/scenarios/weather/context/context.h>

namespace NAlice::NHollywood::NWeather {

TWeatherErrorOr<bool> IsNowcastForNowCase(const TWeatherContext& ctx);
[[nodiscard]] TWeatherStatus PrepareNowcastForNow(TWeatherContext& ctx);

} // namespace NAlice::NHollywood::NWeather

#pragma once

#include <alice/library/util/status.h>

namespace NAlice::NHollywood::NWeather {

enum class EWeatherErrorCode {
    SYSTEM /* "system" */,
    NOGEOFOUND /* "nogeo" */,
    INVALIDPARAM /* "invalidparam" */,
    NOWEATHER /* "noweather" */,
    WEATHERERROR /* "weathererror" */,
    NOPRESSURE /* "nopressure" */,
    NOWIND /* "nowind" */,
    NOCHANGE /* "nochange" */,
};

enum class EWeatherSkipBranchCode {
    SKIP_CURRENT_SCENARIO_BRANCH, // used when switching get_weather_nowcast -> get_weather or get_weather_nowcast_prec_map -> get_weather_nowcast
};

enum class EWeatherOkCode {
    RESPONSE_ALREADY_RENDERED,
    NEED_RENDER_RESPONSE,
};

using TWeatherError = NAlice::TGenericError<EWeatherErrorCode>;
using TWeatherStatus = std::variant<TWeatherError, EWeatherSkipBranchCode, EWeatherOkCode>;

template<typename T>
using TWeatherErrorOr = std::variant<TWeatherError, T>;

} // namespace NAlice::NHollywood::NWeather

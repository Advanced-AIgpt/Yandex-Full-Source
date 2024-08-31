#include "s3_animations.h"

#include <util/generic/hash_set.h>

namespace NAlice::NHollywood::NWeather::NS3Animations {

namespace {

const THashSet<TStringBuf> RAINY_CONDITIONS = {
    "drizzle",
    "light-rain",
    "rain",
    "moderate-rain",
    "heavy-rain",
    "continuous-heavy-rain",
    "showers",
    "thunderstorm",
    "thunderstorm-with-rain",
    "thunderstorm-with-hail",
};

const THashSet<TStringBuf> SNOWY_CONDITIONS = {
    "wet-snow",
    "light-snow",
    "snow",
    "snow-showers",
    "hail",
};

const THashSet<TStringBuf> CLOUDY_CONDITIONS = {
    "cloudy",
    "partly-cloudy",
    "overcast",
};

const THashSet<TStringBuf> CLEAR_CONDITIONS = {
    "clear",
};

} // namespace

TMaybe<TStringBuf> TryGetS3AnimationPathFromCondition(TStringBuf condition) {
    if (RAINY_CONDITIONS.contains(condition)) {
        return "animations/weather/rain";
    } else if (SNOWY_CONDITIONS.contains(condition)) {
        return "animations/weather/snow";
    } else if (CLOUDY_CONDITIONS.contains(condition)) {
        return "animations/weather/cloudy";
    } else if (CLEAR_CONDITIONS.contains(condition)) {
        return "animations/weather/clear";
    } else {
        return Nothing();
    }
}

} // namespace NAlice::NHollywood::NWeather::NS3Animations

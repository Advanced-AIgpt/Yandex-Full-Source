#pragma once

#include <util/generic/string.h>
#include <util/generic/maybe.h>

#include <alice/library/util/rng.h>

namespace NAlice::NHollywood::NWeather {

class TBackgroundSounds {
public:
    enum class ETypes {
        Hail /* "hail" */,
        ThunderstormWithRain /* "thunderstorm_with_rain" */,
        WinterDay /* "winter_day" */,
        WinterNight /* "winter_night" */,
        SummerDay /* "summer_day" */,
        SummerNight /* "summer_night" */,
        BlowingSnow /* "blowing_snow" */,
        DriftingSnow /* "drifting_snow" */,
        Cloudy /* "cloudy" */,
        VeryHot /* "very_hot" */,
        VeryCold /* "very_cold" */,
        LightRain /* "light_rain" */,
        LightSnow /* "light_snow" */,
        WetSnow /* "wet_snow" */,
        Snow /* "snow" */,
        Fog /* "fog" */,
    };

    // not for using outside of this class
    enum class ETemperatureCondition {
        Other,
        VeryHot,
        VeryCold,
    };

    enum class ESeason {
        Other,
        Winter /* "winter" */,
        Spring /* "spring" */ ,
        Summer /* "summer" */,
        Autumn /* "autumn" */,
    };

    enum class EDayTime {
        Other,
        Day /* "d" */,
        Night /* "n" */,
    };

public:
    TBackgroundSounds(IRng& rng);

    TBackgroundSounds& SetWeatherCondition(const TStringBuf weatherCondition);
    TBackgroundSounds& SetTemperature(const double temperature);
    TBackgroundSounds& SetSeason(const ESeason season);
    TBackgroundSounds& SetDayTime(const EDayTime dayTime);

    TMaybe<ETypes> TryCalculateBackgroundType();
    TMaybe<TString> TryCalculateBackgroundFilename();

private:
    IRng& Rng_;

    TMaybe<TStringBuf> WeatherCondition_;
    ETemperatureCondition TemperatureCondition_ = ETemperatureCondition::Other;
    ESeason Season_ = ESeason::Other;
    EDayTime DayTime_ = EDayTime::Other;
};

} // namespace NAlice::NHollywood::NWeather

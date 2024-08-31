#include "background_sounds.h"

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>

namespace NAlice::NHollywood::NWeather {

using ETypes = TBackgroundSounds::ETypes;
using ETemperatureCondition = TBackgroundSounds::ETemperatureCondition;
using ESeason = TBackgroundSounds::ESeason;
using EDayTime = TBackgroundSounds::EDayTime;

namespace {

constexpr double VERY_HOT_TEMPERATURE_LOWER_BOUND = 30.0;
constexpr double VERY_COLD_TEMPERATURE_UPPER_BOUND = -10.0;

const THashMap<TStringBuf, TVector<ETypes>> WEATHER_CONDITION_TO_BACKGROUND_MAPPING = {
    {"blowing-snow", {ETypes::BlowingSnow}}, // метель
    //{"clear", {"???"}}, // ясно
    {"cloudy", {ETypes::Cloudy}}, // облачно с прояснениями
    {"cloudy-and-light-rain", {ETypes::Cloudy, ETypes::LightRain}}, // небольшой дождь
    {"cloudy-and-light-snow", {ETypes::Cloudy, ETypes::LightSnow}}, // небольшой снег
    {"cloudy-and-rain", {ETypes::Cloudy, ETypes::LightRain}}, // дождь
    {"cloudy-and-snow", {ETypes::Cloudy, ETypes::Snow}}, // снег
    {"continuous-heavy-rain", {ETypes::ThunderstormWithRain}}, // сильный дождь
    {"drifting-snow", {ETypes::DriftingSnow}}, // слабая метель
    {"drizzle", {ETypes::LightRain}}, // морось
    //{"dust", {"???"}}, // пыль
    //{"dust-suspension", {"???"}}, // пылевая взвесь
    //{"duststorm", {"???"}}, // пыльная буря
    {"fog", {ETypes::Fog}}, // туман
    {"freezing-rain", {ETypes::WetSnow}}, // ледяной дождь
    {"hail", {ETypes::Hail}}, // град
    {"heavy-rain", {ETypes::ThunderstormWithRain}}, // ливень
    //{"ice-pellets", {"???"}}, // ледяная крупа
    {"light-rain", {ETypes::LightRain}}, // небольшой дождь
    {"light-snow", {ETypes::LightSnow}}, // небольшой снег
    //{"mist", {"???"}}, // дымка
    {"moderate-rain", {ETypes::LightRain}}, // дожди
    //{"overcast", {"???"}}, // пасмурно
    {"overcast-and-light-rain", {ETypes::LightRain}}, // небольшой дождь
    {"overcast-and-light-snow", {ETypes::LightSnow}}, // небольшой снег
    {"overcast-and-rain", {ETypes::ThunderstormWithRain}}, // сильный дождь
    {"overcast-and-snow", {ETypes::Snow}}, // снегопад
    {"overcast-and-wet-snow", {ETypes::WetSnow}}, // дождь со снегом
    {"overcast-thunderstorms-with-rain", {ETypes::ThunderstormWithRain}}, // сильный дождь, гроза
    {"partly-cloudy", {ETypes::Cloudy}}, // малооблачно
    {"partly-cloudy-and-light-rain", {ETypes::Cloudy, ETypes::LightRain}}, // небольшой дождь
    {"partly-cloudy-and-light-snow", {ETypes::Cloudy, ETypes::LightSnow}}, // небольшой снег
    {"partly-cloudy-and-rain", {ETypes::Cloudy, ETypes::LightSnow}}, // дождь
    {"partly-cloudy-and-snow", {ETypes::Cloudy, ETypes::Snow}}, // снег
    {"rain", {ETypes::LightRain}}, // дождь
    {"showers", {ETypes::ThunderstormWithRain}}, // ливни
    //{"smoke", {"???"}}, // смог
    {"snow", {ETypes::Snow}}, // снег
    {"snow-showers", {ETypes::BlowingSnow}}, // снегопад
    {"thunderstorm", {ETypes::BlowingSnow}}, // гроза
    {"thunderstorm-wet-snow", {ETypes::ThunderstormWithRain, ETypes::WetSnow}}, // гроза, дождь со снегом
    {"thunderstorm-with-duststorm", {ETypes::ThunderstormWithRain}}, // пыльная буря с грозой
    {"thunderstorm-with-hail", {ETypes::ThunderstormWithRain, ETypes::Hail}}, // гроза с градом
    {"thunderstorm-with-rain", {ETypes::ThunderstormWithRain}}, // дождь с грозой
    {"thunderstorm-with-snow", {ETypes::ThunderstormWithRain, ETypes::Snow}}, // гроза, снег
    //{"tornado", {"???"}}, // торнадо
    //{"volcanic-ash", {"???"}}, // вулканический пепел
    {"wet-snow", {ETypes::WetSnow}}, // дождь со снегом
};

static const THashMap<ETemperatureCondition, ETypes> TEMPERATURE_CONDITION_TO_BACKGROUND_MAPPING = {
    {ETemperatureCondition::VeryHot, ETypes::VeryHot},
    {ETemperatureCondition::VeryCold, ETypes::VeryCold},
};

const THashMap<std::pair<ESeason, EDayTime>, ETypes> SEASON_AND_DAYTIME_TO_BACKGROUND_MAPPING = {
    {{ESeason::Winter, EDayTime::Day}, ETypes::WinterDay},
    {{ESeason::Winter, EDayTime::Night}, ETypes::WinterNight},
    {{ESeason::Summer, EDayTime::Day}, ETypes::SummerDay},
    {{ESeason::Summer, EDayTime::Night}, ETypes::SummerNight},
};

} // namespace

TBackgroundSounds::TBackgroundSounds(IRng& rng)
    : Rng_{rng}
{}

TBackgroundSounds& TBackgroundSounds::SetWeatherCondition(const TStringBuf weatherCondition) {
    WeatherCondition_ = weatherCondition;
    return *this;
}

TBackgroundSounds& TBackgroundSounds::SetTemperature(const double temperature) {
    if (temperature >= VERY_HOT_TEMPERATURE_LOWER_BOUND) {
        TemperatureCondition_ = ETemperatureCondition::VeryHot;
    } else if (temperature <= VERY_COLD_TEMPERATURE_UPPER_BOUND) {
        TemperatureCondition_ = ETemperatureCondition::VeryCold;
    }
    return *this;
}

TBackgroundSounds& TBackgroundSounds::SetSeason(const ESeason season) {
    Season_ = season;
    return *this;
}

TBackgroundSounds& TBackgroundSounds::SetDayTime(const EDayTime dayTime) {
    DayTime_ = dayTime;
    return *this;
}

TMaybe<ETypes> TBackgroundSounds::TryCalculateBackgroundType() {
    // there shall be no background sound at cases without weather condition
    if (!WeatherCondition_.Defined()) {
        return Nothing();
    }

    TVector<ETypes> possibleTypes;

    // add values from weather condition
    if (const auto* ptr = WEATHER_CONDITION_TO_BACKGROUND_MAPPING.FindPtr(*WeatherCondition_)) {
        std::copy(ptr->begin(), ptr->end(), std::back_inserter(possibleTypes));
    }

    // add value from temperature condition
    if (const auto* ptr = TEMPERATURE_CONDITION_TO_BACKGROUND_MAPPING.FindPtr(TemperatureCondition_)) {
        possibleTypes.push_back(*ptr);
    }

    // add value from season and daytime
    if (const auto* ptr = SEASON_AND_DAYTIME_TO_BACKGROUND_MAPPING.FindPtr(std::make_pair(Season_, DayTime_))) {
        possibleTypes.push_back(*ptr);
    }

    // choose random background speaker
    if (!possibleTypes.empty()) {
        return possibleTypes[Rng_.RandomInteger(possibleTypes.size())];
    }
    return Nothing();
}

TMaybe<TString> TBackgroundSounds::TryCalculateBackgroundFilename() {
    if (const auto type = TryCalculateBackgroundType()) {
        return TString::Join("weather_backgrounds/", ToString(type), ".pcm");
    }
    return Nothing();
}

} // namespace NAlice::NHollywood::NWeather

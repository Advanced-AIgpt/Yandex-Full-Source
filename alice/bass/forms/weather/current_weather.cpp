#include "current_weather.h"
#include "util.h"
#include "suggests_maker.h"

#include <alice/bass/forms/urls_builder.h>

using NAlice::TDateTime;
using NAlice::TDateTimeList;

namespace NBASS::NWeather {

namespace {

TString GetBackgroundType(const TForecast& forecast) {
    auto userTime = forecast.UserTime.ToString("%H:%M");

    const auto& sunrise = forecast.Today().Sunrise;
    const auto& sunset = forecast.Today().Sunset;

    const auto style = CloudinessPrecCssStyle(forecast.Fact.Cloudness, forecast.Fact.PrecStrength);
    if (userTime > sunrise && userTime < sunset) {
        return style + "_day";
    }

    return style + "_night";
}

} // namespace

bool IsCurrentWeatherScenario(TContext& ctx, const TForecast& forecast) {
    auto userTime = forecast.UserTime;
    auto dtlVariant = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TError>(&dtlVariant)) {
        return false;
    }

    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtlVariant);

    return dateTimeList->IsNow();
}

TResultValue PrepareCurrentForecastDivCard(TContext& ctx, const TForecast& forecast) {
    const auto userTime = forecast.UserTime;

    auto hour = forecast.FindHour(TDateTime(userTime));

    if (!hour) {
        return TError(TError::EType::NOWEATHER, "No weather found for the given time");
    }

    if (hour->Next) {
        hour = *hour->Next;
    }

    const auto& fact = forecast.Fact;
    const NSc::TValue tzName(userTime.TimeZone().name());

    NSc::TValue card;
    card["day"]["date"] = hour->DayPart->Day->Date.ToString("%F");
    card["day"]["tz"] = tzName;
    card["day"]["background"] = GetBackgroundType(forecast);
    card["day"]["temp"]["avg"] = fact.Temp;
    card["day"]["icon"] = fact.IconUrl();
    card["day"]["condition"]["title"] = forecast.L10n[fact.Condition];
    card["day"]["condition"]["feels_like"] = fact.FeelsLike;
    card["day"]["hours"] = GetHours(*hour);
    ctx.AddDivCardBlock("weather__curday_v2", card);

    return ResultSuccess();
}

TResultValue PrepareCurrentForecastSlots(TContext& ctx, const TForecast& forecast) {
    auto forecastLocationSlotVariant = GetForecastLocationSlot(ctx);
    if (auto err = std::get_if<TError>(&forecastLocationSlotVariant)) {
        return *err;
    }
    auto forecastLocationSlot = std::get<NSc::TValue>(forecastLocationSlotVariant);

    auto urlVariant = GetWeatherUrl(ctx);
    if (auto err = std::get_if<TError>(&urlVariant)) {
        return *err;
    }

    auto userTime = forecast.UserTime;
    const auto& fact = forecast.Fact;
    const NSc::TValue tzName(userTime.TimeZone().name());

    if (ctx.ClientFeatures().SupportsDivCards()) {
        const auto errorMaybe = PrepareCurrentForecastDivCard(ctx, forecast);
        if (errorMaybe) {
            return *errorMaybe;
        }
    }

    AddForecastLocationSlot(ctx, forecastLocationSlot);

    NSc::TValue forecastSlot;
    forecastSlot["condition"] = forecast.L10n[fact.Condition];
    forecastSlot["date"] = userTime.ToString("%F");
    forecastSlot["temperature"] = fact.Temp;
    forecastSlot["type"] = "weather_current";
    forecastSlot["tz"] = tzName;
    forecastSlot["uri"] = std::get<TString>(urlVariant);

    ctx.CreateSlot("weather_forecast", "forecast", true, forecastSlot);

    TVector<ESuggestType> suggests;
    suggests.push_back(ESuggestType::Tomorrow);
    suggests.push_back(ESuggestType::Weekend);
    if (fact.PrecStrength > 0) {
        suggests.push_back(ESuggestType::NowcastWhenEnds);
    } else {
        suggests.push_back(ESuggestType::NowcastWhenStarts);
    }
    suggests.push_back(ESuggestType::SearchFallback);
    WriteSuggests(ctx, suggests);

    if (ctx.ClientFeatures().SupportsLedDisplay()) {
        AddLedDirective(ctx, MakeWeatherGifUris(ctx, fact.Temp, fact.PrecType, fact.PrecStrength, fact.Cloudness));
    }

    return ResultSuccess();
}

} // namespace NBASS::NWeather

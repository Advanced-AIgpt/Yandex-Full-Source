#include "day_part_weather.h"
#include "util.h"
#include "suggests_maker.h"

#include <alice/bass/forms/context/fwd.h>
#include <alice/bass/forms/urls_builder.h>
#include <library/cpp/scheme/fwd.h>

using NAlice::TDateTime;
using NAlice::TDateTimeList;

namespace NBASS::NWeather {

bool IsDayPartScenario(TContext& ctx, const TForecast& forecast) {
    auto userTime = forecast.UserTime;
    auto dtlVariant = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TError>(&dtlVariant)) {
        return false;
    }

    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtlVariant);
    return dateTimeList->TotalDays() == 1 && !IsSlotEmpty(ctx.GetSlot("day_part"));
}

TResultValue PrepareDayPartForecastSlots(TContext& ctx, const TForecast& forecast) {
    const auto& userTime = forecast.UserTime;
    const auto& dtl = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TError>(&dtl)) {
        return *err;
    }

    const auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtl);
    const auto& dayPartMaybe = forecast.FindDayPart(*dateTimeList->cbegin());
    if (!dayPartMaybe) {
        return TError(TError::EType::NOWEATHER, "No weather found for the given time");;
    }
    const auto& dayPart = *dayPartMaybe;

    auto forecastLocationSlotVariant = GetForecastLocationSlot(ctx);
    if (auto err = std::get_if<TError>(&forecastLocationSlotVariant)) {
        return *err;
    }
    auto forecastLocationSlot = std::get<NSc::TValue>(forecastLocationSlotVariant);

    auto urlVariant = GetWeatherUrl(ctx, dateTimeList->cbegin()->SplitTime().MDay());
    if (auto err = std::get_if<TError>(&urlVariant)) {
        return *err;
    }

    const NSc::TValue tzName(userTime.TimeZone().name());

    if (ctx.ClientFeatures().SupportsDivCards()) {
        NSc::TValue card;
        card["day"]["date"] = dayPart.Day->Date.ToString("%F");
        card["day"]["tz"] = tzName;
        card["day"]["background"] = dayPart.BackgroundStyleClass();
        card["day"]["temp"]["avg"] = dayPart.TempAvg;
        card["day"]["icon"] = dayPart.IconUrl();
        card["day"]["condition"]["title"] = forecast.L10n[dayPart.Condition];
        card["day"]["condition"]["feels_like"] = dayPart.FeelsLike;

        ctx.AddDivCardBlock("weather__1day_v2", card);
    }

    AddForecastLocationSlot(ctx, forecastLocationSlot);

    NSc::TValue temperatures;
    auto& temperaturesArray = temperatures.GetArrayMutable();
    temperaturesArray.push_back(dayPart.TempMin);
    temperaturesArray.push_back(dayPart.TempMax);

    NSc::TValue forecastSlot;
    forecastSlot["condition"] = forecast.L10n[dayPart.Condition];
    forecastSlot["date"] = dayPart.Day->Date.ToString("%F");
    forecastSlot["temperature"] = temperatures;
    forecastSlot["type"] = "weather_for_date";
    forecastSlot["tz"] = tzName;
    forecastSlot["uri"] = std::get<TString>(urlVariant);

    ctx.CreateSlot("weather_forecast", "forecast", true, forecastSlot);

    TVector<ESuggestType> suggests;
    if (dayPart.Day->Date.ToString("%F") != userTime.ToString("%F")) {
        suggests.push_back(ESuggestType::Today);
    } else {
        suggests.push_back(ESuggestType::Tomorrow);
    }
    if (forecast.Fact.PrecStrength > 0) {
        suggests.push_back(ESuggestType::NowcastWhenEnds);
    } else {
        suggests.push_back(ESuggestType::NowcastWhenStarts);
    }
    suggests.push_back(ESuggestType::SearchFallback);
    WriteSuggests(ctx, suggests);

    if (ctx.ClientFeatures().SupportsLedDisplay()) {
        AddLedDirective(ctx, MakeWeatherGifUris(ctx, dayPart.TempAvg, dayPart.PrecType, dayPart.PrecStrength, dayPart.Cloudness));
    }

    return ResultSuccess();
}

} // namespace NBASS::NWeather

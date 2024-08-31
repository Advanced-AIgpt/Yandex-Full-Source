#include "day_weather.h"
#include "util.h"
#include "suggests_maker.h"

#include <alice/bass/forms/urls_builder.h>

using NAlice::TDateTime;
using NAlice::TDateTimeList;

namespace NBASS::NWeather {

bool IsDayWeatherScenario(TContext& ctx, const TForecast& forecast) {
    auto userTime = forecast.UserTime;
    auto dtlVariant = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TError>(&dtlVariant)) {
        return false;
    }

    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtlVariant);
    return dateTimeList->TotalDays() == 1;
}

NSc::TValue MakePart(TString type, TDayPart part, const TForecast& forecast) {
    NSc::TValue card_part;
    card_part["type"] = type;
    card_part["icon"] = part.IconUrl(48);
    card_part["temp"]["avg"] = part.TempAvg;
    card_part["condition"]["title"] = forecast.L10n[part.Condition];
    card_part["condition"]["code"] = part.Condition;
    return card_part;
}

TResultValue PrepareDayForecastSlots(TContext& ctx, const TForecast& forecast) {
    auto userTime = forecast.UserTime;
    auto dtl = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TError>(&dtl)) {
        return *err;
    }
    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtl);

    auto forecastLocationSlotVariant = GetForecastLocationSlot(ctx);
    if (auto err = std::get_if<TError>(&forecastLocationSlotVariant)) {
        return *err;
    }

    const NSc::TValue tzName(userTime.TimeZone().name());

    NSc::TValue forecastSlot;

    auto& dayMaybe = forecast.FindDay(*dateTimeList->begin());
    if (!dayMaybe) {
        return TError(TError::EType::NOWEATHER, "Requested date not found");
    }
    auto& day = *dayMaybe;

    auto urlVariant = GetWeatherUrl(ctx, day.Date.MDay());
    if (auto err = std::get_if<TError>(&urlVariant)) {
        return *err;
    }

    auto dayShortTemp = day.Parts.DayShort.Temp;
    auto nightShortTemp = day.Parts.NightShort.Temp;
    auto& temperaturesArray = forecastSlot["temperature"].GetArrayMutable();
    temperaturesArray.push_back(Min(nightShortTemp, dayShortTemp));
    temperaturesArray.push_back(Max(nightShortTemp, dayShortTemp));

    forecastSlot["date"] = day.Date.ToString("%F");
    forecastSlot["tz"] = tzName;
    forecastSlot["condition"] = forecast.L10n[day.Parts.DayShort.Condition];
    forecastSlot["type"] = "weather_for_date";
    forecastSlot["uri"] = std::get<TString>(urlVariant);

    if (ctx.ClientFeatures().SupportsDivCards()) {
        NSc::TValue card;
        auto& cardPartsArray = card["day"]["parts"].GetArrayMutable();

        card["day"]["tz"] = tzName;
        card["day"]["background"] = day.Parts.DayShort.BackgroundStyleClass();
        card["day"]["date"] = day.Date.ToString("%F");
        card["day"]["temp"]["avg"] = day.Parts.Day.TempAvg;
        card["day"]["icon"] = day.Parts.Day.IconUrl(48);
        card["day"]["condition"]["title"] = forecast.L10n[day.Parts.Day.Condition];
        card["day"]["condition"]["code"] = day.Parts.Day.Condition;
        card["day"]["condition"]["feels_like"] = day.Parts.Day.FeelsLike;

        cardPartsArray.push_back(MakePart("morning", day.Parts.Morning, forecast));
        cardPartsArray.push_back(MakePart("day", day.Parts.Day, forecast));
        cardPartsArray.push_back(MakePart("evening", day.Parts.Evening, forecast));
        if (day.Next) {
            cardPartsArray.push_back(MakePart("night", day.Next->Parts.Night, forecast));
        }

        ctx.AddDivCardBlock("weather__1day_v2", card);
    }

    auto forecastLocationSlot = std::get<NSc::TValue>(forecastLocationSlotVariant);
    AddForecastLocationSlot(ctx, forecastLocationSlot);
    ctx.CreateSlot("weather_forecast", "forecast", true, forecastSlot);

    TVector<ESuggestType> suggests;
    suggests.push_back(ESuggestType::Today);
    suggests.push_back(ESuggestType::Tomorrow);
    if (forecast.Fact.PrecStrength > 0) {
        suggests.push_back(ESuggestType::NowcastWhenEnds);
    } else {
        suggests.push_back(ESuggestType::NowcastWhenStarts);
    }
    suggests.push_back(ESuggestType::SearchFallback);
    WriteSuggests(ctx, suggests);

    if (ctx.ClientFeatures().SupportsLedDisplay()) {
        AddLedDirective(ctx, MakeWeatherGifUris(ctx, day.Parts.Day.TempAvg, day.Parts.Day.PrecType, day.Parts.Day.PrecStrength, day.Parts.Day.Cloudness));
    }

    return ResultSuccess();
}

} // namespace NBASS::NWeather

#include "days_range_weather.h"
#include "util.h"
#include "suggests_maker.h"

#include <alice/bass/forms/urls_builder.h>
#include <util/generic/algorithm.h>

using NAlice::TDateTime;
using NAlice::TDateTimeList;

namespace NBASS::NWeather {

bool IsDaysRangeWeatherScenario(TContext& ctx, const TForecast& forecast) {
    auto userTime = forecast.UserTime;
    auto dtlVariant = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TError>(&dtlVariant)) {
        return false;
    }

    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtlVariant);
    return dateTimeList->TotalDays() > 1;
}

void FixWhenSlotForNextWeekend(TContext& ctx, const TForecast& forecast) {
    auto userTime = forecast.UserTime;
    TContext::TSlot* whenSlot = ctx.GetSlot("when");

    // special case: when user asks for the next weekend, but it is not available, answer with the nearest weekend
    if (IsSlotEmpty(whenSlot)) {
        return;
    }
    if (TStringBuf("datetime_range_raw") != whenSlot->Type && TStringBuf("datetime_range") != whenSlot->Type) {
        return;
    }
    NSc::TValue& startSrc = whenSlot->Value["start"];
    // check if it is really next weekend
    if (startSrc["weekend"].IsNull() || startSrc["weeks_relative"].IsNull() || startSrc["weeks"] != 1) {
        return;
    }
    // check whether these dates are not available (monday, tuesday, wednesday)
    if (userTime.WDay() <= 0 || userTime.WDay() > 3) {
        return;
    }
    startSrc["weeks"] = 0;
    whenSlot->Value["end"]["weeks"] = 0;
    ctx.AddAttention("no_weather_for_next_weekend");
}

TResultValue PrepareDaysRangeForecastSlots(TContext& ctx, const TForecast& forecast) {
    auto userTime = forecast.UserTime;

    FixWhenSlotForNextWeekend(ctx, forecast);

    auto dtl = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TError>(&dtl)) {
        return *err;
    }
    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtl);

    auto forecastLocationSlotVariant = GetForecastLocationSlot(ctx);
    if (auto err = std::get_if<TError>(&forecastLocationSlotVariant)) {
        return *err;
    }
    auto forecastLocationSlot = std::get<NSc::TValue>(forecastLocationSlotVariant);

    auto urlVariant = GetWeatherMonthUrl(ctx);
    if (auto err = std::get_if<TError>(&urlVariant)) {
        return *err;
    }

    const NSc::TValue tzName(userTime.TimeZone().name());

    NSc::TValue card;
    NSc::TValue forecastSlot;

    auto& cardDaysArray = card["days"].GetArrayMutable();
    auto& forecastDaysArray = forecastSlot["days"].GetArrayMutable();
    for (const auto& dt: *dateTimeList) {
        const auto& dayForecastMaybe = forecast.FindDay(dt);

        if (!dayForecastMaybe || !dayForecastMaybe->Next) {
            continue;
        }

        const auto& day = *dayForecastMaybe;
        const auto& dayShortTemp = day.Parts.DayShort.Temp;
        const auto& nightShortTemp = day.Next->Parts.NightShort.Temp;

        NSc::TValue card_day;
        card_day["date"] = day.Date.ToString("%F");

        const auto& week_day = day.Date.WDay();
        card_day["is_red"].SetBool(week_day == 0 || week_day == 6);

        card_day["tz"] = tzName;
        card_day["temp"]["min"] = Min(nightShortTemp, dayShortTemp);
        card_day["temp"]["max"] = Max(nightShortTemp, dayShortTemp);
        card_day["icon"] = day.Parts.DayShort.IconUrl(48, "dark");
        card_day["condition"]["title"] = forecast.L10n[day.Parts.DayShort.Condition];
        card_day["condition"]["code"] = day.Parts.DayShort.Condition;

        auto urlVariant = GetWeatherUrl(ctx, day.Date.MDay());
        if (auto uri = std::get_if<TString>(&urlVariant)) {
            card_day["uri"] = *uri;
        }
        cardDaysArray.push_back(card_day);

        NSc::TValue slot_day;
        slot_day["date"] = day.Date.ToString("%F");
        slot_day["tz"] = tzName;
        slot_day["condition"] = forecast.L10n[day.Parts.DayShort.Condition];

        auto& temperaturesArray = slot_day["temperature"].GetArrayMutable();
        temperaturesArray.push_back(Min(nightShortTemp, dayShortTemp));
        temperaturesArray.push_back(Max(nightShortTemp, dayShortTemp));

        forecastDaysArray.push_back(slot_day);
    }

    if (forecastDaysArray.size() == 0) {
        return TError(TError::EType::NOWEATHER, "Requested range not found");
    }

    if (ctx.ClientFeatures().SupportsDivCards()) {
        ctx.AddDivCardBlock("weather__days_list_v2", card);
    }
    AddForecastLocationSlot(ctx, forecastLocationSlot);

    forecastSlot["type"] = "weather_for_range";
    forecastSlot["uri"] = std::get<TString>(urlVariant);
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

    return ResultSuccess();
}

} // namespace NBASS::NWeather

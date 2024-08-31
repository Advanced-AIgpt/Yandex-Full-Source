#include "day_hours_weather.h"
#include "util.h"
#include "suggests_maker.h"

#include <alice/bass/forms/urls_builder.h>

using NAlice::TDateTime;
using NAlice::TDateTimeList;

namespace NBASS::NWeather {

namespace {

std::pair<int, int> GetMinMaxTemps(TContext& ctx, const TForecast& forecast) {
    auto userTime = forecast.UserTime;
    auto dtlVariant = GetDateTimeList(ctx, userTime);
    if (auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtlVariant)) {
        if (dateTimeList->TotalDays() == 1) {
            const TDateTime& dt = *dateTimeList->cbegin();
            const auto& requestedDay = forecast.FindDay(dt);
            if (requestedDay) {
                return GetMinMaxTempsFromDayParts(requestedDay->Parts);
            }
        }
    }

    return GetMinMaxTempsFromDayParts(forecast.Today().Parts);
}

} // namespace

bool IsDayHoursWeatherScenario(TContext& ctx, const TForecast& forecast) {
    auto userTime = forecast.UserTime;
    auto dtlVariant = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TError>(&dtlVariant)) {
        return false;
    }

    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtlVariant);
    if (dateTimeList->TotalDays() != 1) {
        return false;
    }

    const TDateTime* dt = dateTimeList->cbegin();
    if (dt == dateTimeList->cend()) {
        return false;
    }

    ssize_t daysDiff = dt->OffsetWidth(userTime);
    return daysDiff < 2;
}

TMaybe<THour> FigureOutHour(const TForecast& forecast, const TDateTime& dt) {
    const auto& day = forecast.FindDay(dt);
    if (!day) {
        return TMaybe<const THour>();
    }

    switch (dt.DayPart()) {
        case TDateTime::EDayPart::Night:
            return day->Hours[0];
        case TDateTime::EDayPart::Morning:
            return day->Hours[6];
        case TDateTime::EDayPart::Day:
            return day->Hours[12];
        case TDateTime::EDayPart::Evening:
            return day->Hours[18];
        default:
            // Если часть дня не запросили и это не сегодня - показываем с 8 часов
            if (forecast.UserTime.ToString("%F") != dt.SplitTime().ToString("%F")) {
                return day->Hours[8];
            }

            const auto& userHour = forecast.FindHour(TDateTime(forecast.UserTime));
            if (userHour && userHour->Next) {
                return *userHour->Next;
            }

            return userHour;
    }
}

TResultValue PrepareDayHoursForecastSlots(TContext& ctx, const TForecast& forecast) {
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
    auto forecastLocationSlot = std::get<NSc::TValue>(forecastLocationSlotVariant);

    auto dt = *dateTimeList->cbegin();
    const auto& hour = FigureOutHour(forecast, dt);

    if (!hour) {
        return TError(TError::EType::NOWEATHER, "No weather found for the given time");
    }

    auto [tempMin, tempMax] = GetMinMaxTemps(ctx, forecast);
    const NSc::TValue tzName(userTime.TimeZone().name());

    const auto& part = *hour->DayPart;

    auto urlVariant = GetWeatherUrl(ctx, part.Day->Date.MDay());
    if (auto err = std::get_if<TError>(&urlVariant)) {
        return *err;
    }

    if (ctx.ClientFeatures().SupportsDivCards()) {
        NSc::TValue card;
        card["day"]["date"] = part.Day->Date.ToString("%F");
        card["day"]["tz"] = tzName;
        card["day"]["background"] = part.BackgroundStyleClass();
        card["day"]["temp"]["avg"] = part.TempAvg;
        card["day"]["icon"] = part.IconUrl();
        card["day"]["condition"]["title"] = forecast.L10n[part.Condition];
        card["day"]["condition"]["feels_like"] = part.FeelsLike;
        card["day"]["hours"] = GetHours(*hour);
        ctx.AddDivCardBlock("weather__1day_v2", card);
    }

    AddForecastLocationSlot(ctx, forecastLocationSlot);

    NSc::TValue temperatures;
    auto& temperaturesArray = temperatures.GetArrayMutable();
    temperaturesArray.push_back(tempMin);
    temperaturesArray.push_back(tempMax);

    NSc::TValue forecastSlot;
    forecastSlot["condition"] = forecast.L10n[part.Condition];
    forecastSlot["date"] = part.Day->Date.ToString("%F");
    forecastSlot["temperature"] = temperatures;
    forecastSlot["type"] = "weather_for_date";
    forecastSlot["tz"] = tzName;
    forecastSlot["uri"] = std::get<TString>(urlVariant);

    ctx.CreateSlot("weather_forecast", "forecast", true, forecastSlot);

    TVector<ESuggestType> suggests;
    const auto diff = dateTimeList->begin()->OffsetWidth(userTime);
    if (diff == 0) {
        suggests.push_back(ESuggestType::Tomorrow);
    }
    if (diff <= 1) {
        suggests.push_back(ESuggestType::AfterTomorrow);
    }
    suggests.push_back(ESuggestType::Weekend);
    if (forecast.Fact.PrecStrength > 0) {
        suggests.push_back(ESuggestType::NowcastWhenEnds);
    } else {
        suggests.push_back(ESuggestType::NowcastWhenStarts);
    }
    suggests.push_back(ESuggestType::SearchFallback);
    WriteSuggests(ctx, suggests);

    if (ctx.ClientFeatures().SupportsLedDisplay()) {
        AddLedDirective(ctx, MakeWeatherGifUris(ctx, part.TempAvg, part.PrecType, part.PrecStrength, part.Cloudness));
    }

    return ResultSuccess();
}

} // namespace NBASS::NWeather

#include "today_weather.h"
#include "current_weather.h"
#include "consts.h"
#include "util.h"
#include "suggests_maker.h"

#include <alice/bass/libs/logging_v2/logger.h>

using NAlice::TDateTime;
using NAlice::TDateTimeList;

namespace NBASS::NWeather {

bool IsTodayWeatherScenario(TContext& ctx, const TForecast& forecast) {
    if (ctx.HasExpFlag(NExperiment::DISABLE_NEW_NLG) && !ctx.HasExpFlag(NExperiment::NEW_NLG_COMPARE)) {
        return false;
    }
    if (!IsSlotEmpty(ctx.GetSlot("day_part"))) { // "Погода сегодня вечером" or "Погода вечером"
        return false;
    }
    if (IsSlotEmpty(ctx.GetSlot("when"))) { // "Погода"
        return true;
    }
    auto userTime = forecast.UserTime;
    auto dtlVariant = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TError>(&dtlVariant)) {
        return false;
    }

    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtlVariant);
    if (dateTimeList->TotalDays() != 1) {
        return false;
    }

    const auto diff = dateTimeList->cbegin()->OffsetWidth(userTime);
    return diff == 0;
}

bool HasNowcast(const TNowcast& nowcast) {
    return nowcast.Code == HttpCodes::HTTP_OK;
}

std::variant<bool, TError> IsNowcastForNowCase(TContext& ctx, const TForecast& forecast, const TNowcast& nowcast) {
    if (!HasNowcast(nowcast)) {
        return false;
    }

    const auto userTime = forecast.UserTime;
    auto dtl = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TError>(&dtl)) {
        return *err;
    }

    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtl);
    if (!dateTimeList->IsNow()) {
        return false;
    }

    const TStringBuf state = nowcast.State;
    return state != "nodata" && state != "noprec" && state != "still" && state != "norule";
}

bool IsDayPartForecastCase(TContext& ctx) {
    return !IsSlotEmpty(ctx.GetSlot("day_part"));
}

enum TemperatureType {
    MinMax, Avg
};

NSc::TValue PrepareForecastByDayPart(const TDayPart& dayPart, TemperatureType temperatureType, const TForecast& forecast) {
    NSc::TValue slot;

    slot["day_part"] = TDateTime::DayPartAsString(dayPart.Type);

    if (temperatureType == TemperatureType::MinMax) {
        auto& tempArray = slot["temperature"].GetArrayMutable();
        tempArray.push_back(dayPart.TempMin);
        tempArray.push_back(dayPart.TempMax);
    } else {
        slot["temperature"] = dayPart.TempAvg;
    }

    slot["condition"] = forecast.L10n[dayPart.Condition];
    slot["precipitation_type"] = dayPart.PrecType;
    slot["precipitation_current"] = dayPart.IsPrecipitation();

    return slot;
}

TResultValue PrepareTodayForecastsSlots(TContext& ctx, const TForecast& forecast, const TMaybe<TNowcast> nowcastMaybe) {
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

    const auto currentDayPartType = TDateTime::TimeToDayPart(forecast.UserTime);

    AddForecastLocationSlot(ctx, forecastLocationSlot);

    NSc::TValue forecastSlot;
    forecastSlot["condition"] = forecast.L10n[fact.Condition];
    forecastSlot["date"] = userTime.ToString("%F");
    forecastSlot["temperature"] = fact.Temp;
    forecastSlot["type"] = "weather_today";
    forecastSlot["tz"] = tzName;
    forecastSlot["uri"] = std::get<TString>(urlVariant);
    forecastSlot["day_part"] = TDateTime::DayPartAsString(currentDayPartType);

    ctx.CreateSlot("weather_forecast", "forecast", true, forecastSlot);

    if (currentDayPartType == TDateTime::EDayPart::Morning && ctx.HasExpFlag(NExperiment::NEW_NLG_COMPARE)) {
        NSc::TValue yesterdaySlot;
        yesterdaySlot["temperature"] = forecast.Yesterday.Temp;
        ctx.CreateSlot("yesterday_forecast", "forecast", true, yesterdaySlot);
    }

    bool isCurrentPrecipitation = forecast.Fact.IsPrecipitation();

    ctx.GetOrCreateSlot("precipitation_current", "num")->Value.SetIntNumber(isCurrentPrecipitation);
    TSlot* precTypeSlot = ctx.GetOrCreateSlot("precipitation_type", "num");
    precTypeSlot->Value.SetIntNumber(forecast.Fact.PrecType);

    auto dateSlot = ctx.GetOrCreateSlot("date", "string");
    auto tzSlot = ctx.GetOrCreateSlot("tz", "string");
    auto precDayPartSlot = ctx.GetOrCreateSlot("precipitation_day_part", "string");
    auto precChangeHoursSlot = ctx.GetOrCreateSlot("precipitation_change_hours", "num");
    auto precNextDayPartSlot = ctx.GetOrCreateSlot("precipitation_next_day_part", "string");
    auto precNextTypeSlot = ctx.GetOrCreateSlot("precipitation_next_type", "num");
    auto precNextChangeHoursSlot = ctx.GetOrCreateSlot("precipitation_next_change_hours", "num");
    auto nowcastSlot = ctx.GetOrCreateSlot("weather_nowcast_alert", "string");

    // Remove previous values
    precDayPartSlot->Value.SetNull();
    precNextDayPartSlot->Value.SetNull();
    precNextTypeSlot->Value.SetNull();
    precNextChangeHoursSlot->Value.SetNull();
    nowcastSlot->Value.SetNull();

    precChangeHoursSlot->Value.SetNumber(0);

    if (forecast.Days.size() > 0) {
        int foundChanges = 0;
        const TInstant time = TInstant::Seconds(forecast.Now);
        const TInstant timeNextDay = time + TDuration::Days(1);
        const TMaybe<THour> firstHour = forecast.FindHour(TDateTime(forecast.UserTime));
        for (
            const THour* hour = firstHour.Get();
            hour && hour->HourTS <= timeNextDay;
            hour = hour->Next
        ) {
            // Первое изменение осадков
            if (isCurrentPrecipitation != hour->IsPrecipitation() && foundChanges == 0) {
                if (hour->DayPart) {
                    precDayPartSlot->Value.SetString(TDateTime::DayPartAsString(hour->DayPart->Type));
                }
                precChangeHoursSlot->Value.SetIntNumber((hour->HourTS - time).Hours() + 1);

                if (!isCurrentPrecipitation) {
                    precTypeSlot->Value.SetIntNumber(hour->PrecType);
                }

                ++foundChanges;
                isCurrentPrecipitation = hour->IsPrecipitation();
                continue;
            }

            if (ctx.HasExpFlag("weather_precipitation_starts_ends")) {
                dateSlot->Value.SetString(forecast.UserTime.ToString("%F-%T"));
                tzSlot->Value.SetString(forecast.UserTime.TimeZone().name());

                // ASSISTANT-3085: Поддержать закончится-начнется со стороны BASS
                precNextChangeHoursSlot->Value.SetNumber(0);

                if (isCurrentPrecipitation != hour->IsPrecipitation() && foundChanges == 1) {
                    if (hour->DayPart) {
                        precNextDayPartSlot->Value.SetString(TDateTime::DayPartAsString(hour->DayPart->Type));
                    }
                    precNextChangeHoursSlot->Value.SetIntNumber((hour->HourTS - time).Hours() + 1);

                    if (!isCurrentPrecipitation) {
                        precNextTypeSlot->Value.SetIntNumber(hour->PrecType);
                    }

                    break;
                }
            }
        }
    }

    switch (currentDayPartType) {
        case TDateTime::EDayPart::Night:
            ctx.CreateSlot("forecast_next", "forecast", true, PrepareForecastByDayPart(forecast.Tomorrow().Parts.Day, TemperatureType::MinMax, forecast));
            break;
        case TDateTime::EDayPart::Morning:
            ctx.CreateSlot("forecast_next", "forecast", true, PrepareForecastByDayPart(forecast.Today().Parts.Day, TemperatureType::MinMax, forecast));
            ctx.CreateSlot("forecast_next_next", "forecast", true, PrepareForecastByDayPart(forecast.Today().Parts.Evening, TemperatureType::Avg, forecast));
            break;
        case TDateTime::EDayPart::Day:
            ctx.CreateSlot("forecast_next", "forecast", true, PrepareForecastByDayPart(forecast.Today().Parts.Evening, TemperatureType::Avg, forecast));
            ctx.CreateSlot("forecast_next_next", "forecast", true, PrepareForecastByDayPart(forecast.Tomorrow().Parts.Night, TemperatureType::Avg, forecast));
            break;
        case TDateTime::EDayPart::Evening:
            ctx.CreateSlot("forecast_next", "forecast", true, PrepareForecastByDayPart(forecast.Tomorrow().Parts.Night, TemperatureType::Avg, forecast));
            break;
        default:
            LOG(ERR) << "Failed to generate forecast_next* because current day part is unexpected (" << currentDayPartType << ")." << Endl;
            break;
    }

    if (nowcastMaybe) {
        const TNowcast& nowcast = *nowcastMaybe;
        auto isNowcastForNowCase = IsNowcastForNowCase(ctx, forecast, nowcast);
        if (auto err = std::get_if<TError>(&isNowcastForNowCase)) {
            LOG(ERR) << "Error checking IsNowcastForNowCase: " << *err << Endl;
        } else if (std::get<bool>(isNowcastForNowCase)) {
            nowcastSlot->Value.SetString((*nowcastMaybe).Text);
        }
    }

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


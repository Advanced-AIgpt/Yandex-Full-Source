#include "pressure_cases.h"

#include <alice/hollywood/library/scenarios/weather/util/util.h>

namespace NAlice::NHollywood::NWeather {

[[nodiscard]] TWeatherStatus PrepareCurrentPressureForecastSlots(TWeatherContext& ctx) {
    auto urlVariant = GetWeatherUrl(ctx);
    if (auto err = std::get_if<TWeatherError>(&urlVariant)) {
        return *err;
    }

    const auto& forecast = *ctx.Forecast();
    auto userTime = forecast.UserTime;
    const auto& fact = forecast.Fact;
    const NJson::TJsonValue tzName(userTime.TimeZone().name());

    NJson::TJsonValue forecastSlot;
    forecastSlot["date"] = userTime.ToString("%F");
    forecastSlot["type"] = "pressure_current";
    forecastSlot["tz"] = tzName;
    forecastSlot["pressure"] = fact.PressureMM;

    ctx.AddSlot("weather_forecast", "forecast", forecastSlot);

    ctx.Renderer().AddTextCard(NNlgTemplateNames::GET_WEATHER_PRESSURE, "render_pressure_current");

    TVector<ESuggestType> suggests{
        ESuggestType::Feedback,
        ESuggestType::Tomorrow,
        ESuggestType::Weekend,
        fact.PrecStrength > 0 ? ESuggestType::NowcastWhenEnds : ESuggestType::NowcastWhenStarts,
        ESuggestType::SearchFallback,
        ESuggestType::Onboarding
    };
    ctx.Renderer().AddSuggests(suggests);

    return EWeatherOkCode::RESPONSE_ALREADY_RENDERED;
}

TWeatherStatus PrepareDayPartPressureForecastSlots(TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();

    const auto& userTime = forecast.UserTime;
    const auto dtl = GetDateTimeList(ctx, userTime);
    if (const auto err = std::get_if<TWeatherError>(&dtl)) {
        return *err;
    }

    const auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtl);
    const auto& dayPartMaybe = forecast.FindDayPart(*dateTimeList->cbegin());
    if (!dayPartMaybe) {
        return TWeatherError(EWeatherErrorCode::NOPRESSURE) << "No pressure found for the given time";
    }
    const auto& dayPart = *dayPartMaybe;

    const auto urlVariant = GetWeatherUrl(ctx, dateTimeList->cbegin()->SplitTime().MDay());
    if (const auto err = std::get_if<TWeatherError>(&urlVariant)) {
        return *err;
    }

    const NJson::TJsonValue tzName(userTime.TimeZone().name());

    NJson::TJsonValue forecastSlot;
    forecastSlot["date"] = dayPart.Day->Date.ToString("%F");
    forecastSlot["type"] = "pressure_for_date";
    forecastSlot["tz"] = tzName;
    forecastSlot["uri"] = std::get<TString>(urlVariant);

    forecastSlot["pressure"] = dayPart.PressureMM;

    ctx.AddSlot("weather_forecast", "forecast", forecastSlot);

    ctx.Renderer().AddTextCard(NNlgTemplateNames::GET_WEATHER_PRESSURE, "render_pressure_for_date");

    TVector<ESuggestType> suggests{
        ESuggestType::Feedback,
        dayPart.Day->Date.ToString("%F") != userTime.ToString("%F") ? ESuggestType::Today : ESuggestType::Tomorrow,
        forecast.Fact.PrecStrength > 0 ? ESuggestType::NowcastWhenEnds : ESuggestType::NowcastWhenStarts,
        ESuggestType::SearchFallback,
        ESuggestType::Onboarding
    };
    ctx.Renderer().AddSuggests(std::move(suggests));

    return EWeatherOkCode::RESPONSE_ALREADY_RENDERED;
}

[[nodiscard]] TWeatherStatus PrepareDayPressureForecastSlots(TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();
    auto userTime = forecast.UserTime;
    auto dtl = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TWeatherError>(&dtl)) {
        return *err;
    }
    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtl);

    const NJson::TJsonValue tzName(userTime.TimeZone().name());

    NJson::TJsonValue forecastSlot;
    const auto* dayPtr = forecast.FindDay(*dateTimeList->begin());
    if (!dayPtr) {
        return TWeatherError{EWeatherErrorCode::NOPRESSURE} << "Requested date not found";
    }
    const auto& day = *dayPtr;

    auto urlVariant = GetWeatherUrl(ctx, day.Date.MDay());
    if (auto err = std::get_if<TWeatherError>(&urlVariant)) {
        return *err;
    }

    forecastSlot["date"] = day.Date.ToString("%F");
    forecastSlot["tz"] = tzName;
    forecastSlot["type"] = "pressure_for_date";

    forecastSlot["pressure"] = GetPressureMMFromDay(day);

    ctx.AddSlot("weather_forecast", "forecast", forecastSlot);

    ctx.Renderer().AddTextCard(NNlgTemplateNames::GET_WEATHER_PRESSURE, "render_pressure_for_date");

    TVector<ESuggestType> suggests{
        ESuggestType::Feedback,
        ESuggestType::Today,
        dateTimeList->cbegin()->OffsetWidth(userTime) == 1 ? ESuggestType::AfterTomorrow : ESuggestType::Tomorrow,
        forecast.Fact.PrecStrength > 0 ? ESuggestType::NowcastWhenEnds : ESuggestType::NowcastWhenStarts,
        ESuggestType::SearchFallback,
        ESuggestType::Onboarding
    };
    ctx.Renderer().AddSuggests(suggests);

    return EWeatherOkCode::RESPONSE_ALREADY_RENDERED;
}

[[nodiscard]] TWeatherStatus PrepareDaysRangePressureForecastSlots(TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();
    auto userTime = forecast.UserTime;

    FixWhenSlotForNextWeekend(ctx);

    auto dtl = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TWeatherError>(&dtl)) {
        return *err;
    }
    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtl);

    const NJson::TJsonValue tzName(userTime.TimeZone().name());

    NJson::TJsonValue forecastSlot;

    NJson::TJsonValue forecastDaysArray = NJson::TJsonArray();

    for (const auto& dt: *dateTimeList) {
        const auto& dayForecastMaybe = forecast.FindDay(dt);

        if (!dayForecastMaybe || !dayForecastMaybe->Next) {
            ctx.Renderer().AddAttention("incomplete_forecast");
            continue;
        }

        const auto& day = *dayForecastMaybe;

        NJson::TJsonValue slotDay;
        slotDay["date"] = day.Date.ToString("%F");
        slotDay["tz"] = tzName;
        slotDay["pressure"] = GetPressureMMFromDay(day);

        forecastDaysArray.GetArraySafe().push_back(std::move(slotDay));
    }

    if (forecastDaysArray.GetArraySafe().size() == 0) {
        return TWeatherError{EWeatherErrorCode::NOPRESSURE} << "Requested range not found";
    }

    forecastSlot["days"] = std::move(forecastDaysArray);

    forecastSlot["type"] = "pressure_for_range";
    ctx.AddSlot("weather_forecast", "forecast", forecastSlot);

    ctx.Renderer().AddTextCard(NNlgTemplateNames::GET_WEATHER_PRESSURE, "render_pressure_for_range");

    TVector<ESuggestType> suggests{
        ESuggestType::Feedback,
        ESuggestType::Today,
        ESuggestType::Tomorrow,
        forecast.Fact.PrecStrength > 0 ? ESuggestType::NowcastWhenEnds : ESuggestType::NowcastWhenStarts,
        ESuggestType::SearchFallback,
        ESuggestType::Onboarding
    };
    ctx.Renderer().AddSuggests(suggests);

    return EWeatherOkCode::RESPONSE_ALREADY_RENDERED;
}

NJson::TJsonValue PreparePressureForecastByDayPart(const TDayPart& dayPart) {
    NJson::TJsonValue slot;

    slot["day_part"] = TDateTime::DayPartAsString(dayPart.Type);
    slot["pressure"] = dayPart.PressureMM;

    return slot;
}

TWeatherStatus PrepareTodayPressureForecastSlots(TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();

    auto userTime = forecast.UserTime;
    const auto& fact = forecast.Fact;
    const NJson::TJsonValue tzName(userTime.TimeZone().name());

    const auto currentDayPartType = TDateTime::TimeToDayPart(forecast.UserTime);

    NJson::TJsonValue forecastSlot;
    forecastSlot["date"] = userTime.ToString("%F");
    forecastSlot["type"] = "pressure_today";
    forecastSlot["tz"] = tzName;
    forecastSlot["day_part"] = TDateTime::DayPartAsString(currentDayPartType);

    forecastSlot["pressure"] = fact.PressureMM;

    ctx.AddSlot("weather_forecast", "forecast", forecastSlot);

    switch (currentDayPartType) {
        case TDateTime::EDayPart::Night:
            ctx.AddSlot("forecast_next", "forecast", PreparePressureForecastByDayPart(forecast.Tomorrow().Parts.Day));
            ctx.RemoveSlot("forecast_next_next"); // removing slot that could been left from previous query
            break;
        case TDateTime::EDayPart::Morning:
            ctx.AddSlot("forecast_next", "forecast", PreparePressureForecastByDayPart(forecast.Today().Parts.Day));
            ctx.AddSlot("forecast_next_next", "forecast", PreparePressureForecastByDayPart(forecast.Today().Parts.Evening));
            break;
        case TDateTime::EDayPart::Day:
            ctx.AddSlot("forecast_next", "forecast", PreparePressureForecastByDayPart(forecast.Today().Parts.Evening));
            ctx.AddSlot("forecast_next_next", "forecast", PreparePressureForecastByDayPart(forecast.Tomorrow().Parts.Night));
            break;
        case TDateTime::EDayPart::Evening:
            ctx.AddSlot("forecast_next", "forecast", PreparePressureForecastByDayPart(forecast.Tomorrow().Parts.Night));
            ctx.RemoveSlot("forecast_next_next"); // removing slot that could been left from previous query
            break;
        default:
            LOG_ERROR(ctx.Logger()) << "Failed to generate forecast_next* because current day part is unexpected (" << currentDayPartType << ").";
            break;
    }

    ctx.Renderer().AddTextCard(NNlgTemplateNames::GET_WEATHER_PRESSURE, "render_pressure_current");

    TVector<ESuggestType> suggests{
        ESuggestType::Feedback,
        ESuggestType::Tomorrow,
        ESuggestType::Weekend,
        fact.PrecStrength > 0 ? ESuggestType::NowcastWhenEnds : ESuggestType::NowcastWhenStarts,
        ESuggestType::SearchFallback,
        ESuggestType::Onboarding
    };
    ctx.Renderer().AddSuggests(suggests);

    return EWeatherOkCode::RESPONSE_ALREADY_RENDERED;
}

} // namespace NAlice::NHollywood::NWeather

#include "today_wind.h"
#include <alice/hollywood/library/scenarios/weather/util/util.h>

namespace NAlice::NHollywood::NWeather {

NJson::TJsonValue PrepareWindForecastByDayPart(const TDayPart& dayPart, const ELanguage language) {
    NJson::TJsonValue slot;

    slot["day_part"] = TDateTime::DayPartAsString(dayPart.Type);

    slot["wind_speed"] = dayPart.WindSpeed;
    slot["wind_gust"] = dayPart.WindGust;
    slot["wind_dir"] = TranslateWindDirection(dayPart.WindDir, language);

    return slot;
}

TWeatherStatus PrepareTodayWindForecastSlots(TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();

    auto userTime = forecast.UserTime;
    const auto& fact = forecast.Fact;
    const NJson::TJsonValue tzName(userTime.TimeZone().name());

    const auto currentDayPartType = TDateTime::TimeToDayPart(forecast.UserTime);

    NJson::TJsonValue forecastSlot;
    forecastSlot["date"] = userTime.ToString("%F");
    forecastSlot["type"] = "wind_today";
    forecastSlot["tz"] = tzName;
    forecastSlot["day_part"] = TDateTime::DayPartAsString(currentDayPartType);

    forecastSlot["wind_speed"] = fact.WindSpeed;
    forecastSlot["wind_gust"] = fact.WindGust;
    forecastSlot["wind_dir"] = TranslateWindDirection(fact.WindDir, ctx.Ctx().UserLang);

    ctx.AddSlot("weather_forecast", "forecast", forecastSlot);

    switch (currentDayPartType) {
        case TDateTime::EDayPart::Night:
            ctx.AddSlot("forecast_next", "forecast", PrepareWindForecastByDayPart(forecast.Tomorrow().Parts.Day, ctx.Ctx().UserLang));
            ctx.RemoveSlot("forecast_next_next"); // removing slot that could been left from previous query
            break;
        case TDateTime::EDayPart::Morning:
            ctx.AddSlot("forecast_next", "forecast", PrepareWindForecastByDayPart(forecast.Today().Parts.Day, ctx.Ctx().UserLang));
            ctx.AddSlot("forecast_next_next", "forecast", PrepareWindForecastByDayPart(forecast.Today().Parts.Evening, ctx.Ctx().UserLang));
            break;
        case TDateTime::EDayPart::Day:
            ctx.AddSlot("forecast_next", "forecast", PrepareWindForecastByDayPart(forecast.Today().Parts.Evening, ctx.Ctx().UserLang));
            ctx.AddSlot("forecast_next_next", "forecast", PrepareWindForecastByDayPart(forecast.Tomorrow().Parts.Night, ctx.Ctx().UserLang));
            break;
        case TDateTime::EDayPart::Evening:
            ctx.AddSlot("forecast_next", "forecast", PrepareWindForecastByDayPart(forecast.Tomorrow().Parts.Night, ctx.Ctx().UserLang));
            ctx.RemoveSlot("forecast_next_next"); // removing slot that could been left from previous query
            break;
        default:
            LOG_ERROR(ctx.Logger()) << "Failed to generate forecast_next* because current day part is unexpected (" << currentDayPartType << ").";
            break;
    }

    ctx.Renderer().AddTextCard(NNlgTemplateNames::GET_WEATHER_WIND, "render_wind_today");

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

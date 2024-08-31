#include "day_wind.h"

#include <alice/hollywood/library/scenarios/weather/util/util.h>

namespace NAlice::NHollywood::NWeather {

[[nodiscard]] TWeatherStatus PrepareDayWindForecastSlots(TWeatherContext& ctx) {
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
        return TWeatherError{EWeatherErrorCode::NOWIND} << "Requested date not found";
    }
    const auto& day = *dayPtr;

    auto urlVariant = GetWeatherWindUrl(ctx, day.Date.MDay());
    if (auto err = std::get_if<TWeatherError>(&urlVariant)) {
        return *err;
    }

    forecastSlot["date"] = day.Date.ToString("%F");
    forecastSlot["tz"] = tzName;
    forecastSlot["type"] = "wind_for_date";

    forecastSlot["wind_speed"] = GetWindSpeedFromDay(day);
    forecastSlot["wind_gust"] = GetWindGustFromDay(day);
    forecastSlot["wind_dir"] = GetTranslatedWindDirectionFromDay(day, ctx.Ctx().UserLang);

    ctx.AddSlot("weather_forecast", "forecast", forecastSlot);

    ctx.Renderer().AddTextCard(NNlgTemplateNames::GET_WEATHER_WIND, "render_wind_for_date");

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

} // namespace NAlice::NHollywood::NWeather

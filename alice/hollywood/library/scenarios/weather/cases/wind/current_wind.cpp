#include "current_wind.h"

#include <alice/hollywood/library/scenarios/weather/util/util.h>

namespace NAlice::NHollywood::NWeather {

[[nodiscard]] TWeatherStatus PrepareCurrentWindForecastSlots(TWeatherContext& ctx) {
    auto urlVariant = GetWeatherWindUrl(ctx);
    if (auto err = std::get_if<TWeatherError>(&urlVariant)) {
        return *err;
    }

    const auto& forecast = *ctx.Forecast();
    auto userTime = forecast.UserTime;
    const auto& fact = forecast.Fact;
    const NJson::TJsonValue tzName(userTime.TimeZone().name());

    NJson::TJsonValue forecastSlot;
    forecastSlot["date"] = userTime.ToString("%F");
    forecastSlot["type"] = "wind_current";
    forecastSlot["tz"] = tzName;

    forecastSlot["wind_speed"] = fact.WindSpeed;
    forecastSlot["wind_gust"] = fact.WindGust;
    if ((forecastSlot["wind_dir"] = TranslateWindDirection(fact.WindDir, ctx.Ctx().UserLang)) == "") {
        LOG_ERROR(ctx.Logger()) << "Recieved unknown wind direction";
    }

    ctx.AddSlot("weather_forecast", "forecast", forecastSlot);

    ctx.Renderer().AddTextCard(NNlgTemplateNames::GET_WEATHER_WIND, "render_wind_current");

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

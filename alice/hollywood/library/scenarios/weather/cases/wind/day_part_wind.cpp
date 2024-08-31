#include "day_part_wind.h"

#include <alice/hollywood/library/scenarios/weather/util/util.h>

namespace NAlice::NHollywood::NWeather {

TWeatherStatus PrepareDayPartWindForecastSlots(TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();

    const auto& userTime = forecast.UserTime;
    const auto dtl = GetDateTimeList(ctx, userTime);
    if (const auto err = std::get_if<TWeatherError>(&dtl)) {
        return *err;
    }

    const auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtl);
    const auto& dayPartMaybe = forecast.FindDayPart(*dateTimeList->cbegin());
    if (!dayPartMaybe) {
        return TWeatherError(EWeatherErrorCode::NOWIND) << "No wind found for the given time";
    }
    const auto& dayPart = *dayPartMaybe;

    const auto urlVariant = GetWeatherWindUrl(ctx, dateTimeList->cbegin()->SplitTime().MDay());
    if (const auto err = std::get_if<TWeatherError>(&urlVariant)) {
        return *err;
    }

    const NJson::TJsonValue tzName(userTime.TimeZone().name());

    NJson::TJsonValue forecastSlot;
    forecastSlot["date"] = dayPart.Day->Date.ToString("%F");
    forecastSlot["type"] = "wind_for_date";
    forecastSlot["tz"] = tzName;
    forecastSlot["uri"] = std::get<TString>(urlVariant);

    forecastSlot["wind_speed"] = dayPart.WindSpeed;
    forecastSlot["wind_gust"] = dayPart.WindGust;

    if ((forecastSlot["wind_dir"] = TranslateWindDirection(dayPart.WindDir, ctx.Ctx().UserLang)) == "") {
        LOG_ERROR(ctx.Logger()) << "Recieved unknown wind direction";
    }

    ctx.AddSlot("weather_forecast", "forecast", forecastSlot);

    ctx.Renderer().AddTextCard(NNlgTemplateNames::GET_WEATHER_WIND, "render_wind_for_date");

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

} // namespace NAlice::NHollywood::NWeather

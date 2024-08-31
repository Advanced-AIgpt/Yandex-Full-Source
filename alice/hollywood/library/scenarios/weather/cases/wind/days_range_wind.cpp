#include "days_range_wind.h"

#include <alice/hollywood/library/scenarios/weather/util/util.h>

namespace NAlice::NHollywood::NWeather {

[[nodiscard]] TWeatherStatus PrepareDaysRangeWindForecastSlots(TWeatherContext& ctx) {
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

        slotDay["wind_speed"] = GetWindSpeedFromDay(day);
        slotDay["wind_gust"] = GetWindGustFromDay(day);
        slotDay["wind_dir"] = GetTranslatedWindDirectionFromDay(day, ctx.Ctx().UserLang);

        forecastDaysArray.GetArraySafe().push_back(std::move(slotDay));
    }

    if (forecastDaysArray.GetArraySafe().size() == 0) {
        return TWeatherError{EWeatherErrorCode::NOWIND} << "Requested range not found";
    }

    forecastSlot["days"] = std::move(forecastDaysArray);

    forecastSlot["type"] = "wind_for_range";
    ctx.AddSlot("weather_forecast", "forecast", forecastSlot);

    ctx.Renderer().AddTextCard(NNlgTemplateNames::GET_WEATHER_WIND, "render_wind_for_range");

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

} // namespace NAlice::NHollywood::NWeather

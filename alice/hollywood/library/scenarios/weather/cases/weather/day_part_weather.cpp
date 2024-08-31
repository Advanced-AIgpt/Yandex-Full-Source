#include "day_part_weather.h"

#include <alice/hollywood/library/scenarios/weather/context/api.h>
#include <alice/hollywood/library/scenarios/weather/util/util.h>

namespace NAlice::NHollywood::NWeather {

bool IsDayPartWeatherScenario(const TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();

    const auto& userTime = forecast.UserTime;
    const auto dtlVariant = GetDateTimeList(ctx, userTime);
    if (const auto err = std::get_if<TWeatherError>(&dtlVariant)) {
        return false;
    }

    const auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtlVariant);
    return dateTimeList->TotalDays() == 1 && !ctx.IsSlotEmpty("day_part");
}

const NRenderer::TDivRenderData GenerateRenderData(const TWeatherContext& ctx, const TDayPart& part) {
    const auto& forecast = *ctx.Forecast();
    auto userTime = forecast.UserTime;
    const auto& tzName = userTime.TimeZone().name();

    LOG_INFO(ctx.Logger()) << "Adding ShowView Directive";
    NRenderer::TDivRenderData renderData;
    renderData.SetCardId("weather.scenario.div.card");
    auto& weatherScenarioData = *renderData.MutableScenarioData()->MutableWeatherDayPartData();

    weatherScenarioData.SetDate(part.Day->Date.ToString("%F"));
    weatherScenarioData.SetDayPartType(TDateTime::DayPartAsString(part.Type).data());
    weatherScenarioData.SetTz(tzName.c_str());
    weatherScenarioData.SetUserDate(userTime.ToString("%F"));
    weatherScenarioData.SetUserTime(userTime.ToString("%H:%M"));
    weatherScenarioData.SetSunrise(part.Day->Sunrise);
    weatherScenarioData.SetSunset(part.Day->Sunset);
    weatherScenarioData.SetTemperature(part.TempAvg);
    weatherScenarioData.SetIcon(part.IconUrl());
    weatherScenarioData.SetIconType(part.Icon);
    weatherScenarioData.MutableCondition()->SetTitle(forecast.L10n.Translate(part.Condition));
    weatherScenarioData.MutableCondition()->SetFeelsLike(part.FeelsLike);
    weatherScenarioData.MutableCondition()->SetCloudness(part.Cloudness);
    weatherScenarioData.MutableCondition()->SetPrecStrength(part.PrecStrength);
    weatherScenarioData.MutableCondition()->SetPrecType(part.PrecType);
    weatherScenarioData.MutableTodayDaylight()->SetSunrise(forecast.Today().Sunrise);
    weatherScenarioData.MutableTodayDaylight()->SetSunset(forecast.Today().Sunset);

    FillShowViewGeoLocation(*weatherScenarioData.MutableGeoLocation(), ctx);

    return renderData;
}

TWeatherStatus PrepareDayPartForecastSlots(TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();

    const auto& userTime = forecast.UserTime;
    const auto dtl = GetDateTimeList(ctx, userTime);
    if (const auto err = std::get_if<TWeatherError>(&dtl)) {
        return *err;
    }

    const auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtl);
    const auto& dayPartMaybe = forecast.FindDayPart(*dateTimeList->cbegin());
    if (!dayPartMaybe) {
        return TWeatherError(EWeatherErrorCode::NOWEATHER) << "No weather found for the given time";
    }
    const auto& dayPart = *dayPartMaybe;

    const auto urlVariant = GetWeatherUrl(ctx, dateTimeList->cbegin()->SplitTime().MDay());
    if (const auto err = std::get_if<TWeatherError>(&urlVariant)) {
        return *err;
    }

    const NJson::TJsonValue tzName(userTime.TimeZone().name());

    NJson::TJsonValue temperatures = NJson::TJsonArray();
    temperatures.GetArraySafe().push_back(dayPart.TempMin);
    temperatures.GetArraySafe().push_back(dayPart.TempMax);

    NJson::TJsonValue forecastSlot;
    forecastSlot["condition"] = forecast.L10n.Translate(dayPart.Condition);
    forecastSlot["date"] = dayPart.Day->Date.ToString("%F");
    forecastSlot["temperature"] = temperatures;
    forecastSlot["type"] = "weather_for_date";
    forecastSlot["tz"] = tzName;
    forecastSlot["uri"] = std::get<TString>(urlVariant);

    ctx.AddSlot("weather_forecast", "forecast", forecastSlot);

    TVector<ESuggestType> suggests{
        ESuggestType::Feedback,
        dayPart.Day->Date.ToString("%F") != userTime.ToString("%F") ? ESuggestType::Today : ESuggestType::Tomorrow,
        forecast.Fact.PrecStrength > 0 ? ESuggestType::NowcastWhenEnds : ESuggestType::NowcastWhenStarts,
        ESuggestType::SearchFallback,
        ESuggestType::Onboarding
    };
    ctx.Renderer().AddSuggests(std::move(suggests));

    ctx.Renderer().AddAnimationDirectives(TDefaultConditionProvider{dayPart});

    if (ctx.SupportsShowView()) {
        auto renderData = GenerateRenderData(ctx, dayPart);
        ctx.Renderer().Builder().GetResponseBodyBuilder()->AddShowViewDirective(std::move(renderData), NScenarios::TShowViewDirective_EInactivityTimeout_Medium);
        ctx.Renderer().Builder().GetResponseBodyBuilder()->AddClientActionDirective("tts_play_placeholder", {});
    }
    else if (ctx.CanRenderDivCards()) {
        NJson::TJsonValue dayJson;
        dayJson["date"] = dayPart.Day->Date.ToString("%F");
        dayJson["tz"] = tzName;
        dayJson["background"] = dayPart.BackgroundStyleClass();
        dayJson["temp"]["avg"] = dayPart.TempAvg;
        dayJson["icon"] = dayPart.IconUrl();
        dayJson["condition"]["title"] = forecast.L10n.Translate(dayPart.Condition);
        dayJson["condition"]["feels_like"] = dayPart.FeelsLike;

        NJson::TJsonValue card;
        card["day"] = dayJson;

        ctx.Renderer().AddDivCard(NNlgTemplateNames::GET_WEATHER, "weather__1day_v2", card);
        ctx.Renderer().AddVoiceCard(NNlgTemplateNames::GET_WEATHER, "render_weather_for_date");

        return EWeatherOkCode::RESPONSE_ALREADY_RENDERED;
    }

    ctx.Renderer().AddTextCard(NNlgTemplateNames::GET_WEATHER, "render_weather_for_date");

    return EWeatherOkCode::RESPONSE_ALREADY_RENDERED;
}

} // namespace NAlice::NHollywood::NWeather

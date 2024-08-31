#include "day_part_weather.h"

#include <alice/hollywood/library/scenarios/weather/context/api.h>
#include <alice/hollywood/library/scenarios/weather/util/util.h>

namespace NAlice::NHollywood::NWeather {

namespace {

NJson::TJsonValue MakePart(TString type, const TDayPart& part, const TForecast& forecast) {
    NJson::TJsonValue card_part;
    card_part["type"] = type;
    card_part["icon"] = part.IconUrl(48);
    card_part["temp"]["avg"] = part.TempAvg;
    card_part["condition"]["title"] = forecast.L10n.Translate(part.Condition);
    card_part["condition"]["code"] = part.Condition;
    return card_part;
}

} // namespace

bool IsDayWeatherScenario(const TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();
    auto userTime = forecast.UserTime;
    auto dtlVariant = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TWeatherError>(&dtlVariant)) {
        return false;
    }
    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtlVariant);
    return dateTimeList->TotalDays() == 1;
}

void FillDayPartItem(::NAlice::NData::TWeatherDayPartItem& partItem, const TDayPart& part, const TForecast& forecast) {
    partItem.SetDayPartType(TDateTime::DayPartAsString(part.Type).data());
    partItem.SetTemperature(part.TempAvg);
    partItem.SetIcon(part.IconUrl());
    partItem.SetIconType(part.Icon);
    partItem.MutableCondition()->SetTitle(forecast.L10n.Translate(part.Condition));
    partItem.MutableCondition()->SetFeelsLike(part.FeelsLike);
    partItem.MutableCondition()->SetCloudness(part.Cloudness);
    partItem.MutableCondition()->SetPrecStrength(part.PrecStrength);
    partItem.MutableCondition()->SetPrecType(part.PrecType);
}

const NRenderer::TDivRenderData GenerateRenderData(const TWeatherContext& ctx, const TDay& day) {

    const auto& forecast = *ctx.Forecast();
    auto userTime = forecast.UserTime;
    const auto& tzName = userTime.TimeZone().name();

    LOG_INFO(ctx.Logger()) << "Adding ShowView Directive";
    NRenderer::TDivRenderData renderData;
    renderData.SetCardId("weather.scenario.div.card");
    auto& weatherScenarioData = *renderData.MutableScenarioData()->MutableWeatherDayData();

    weatherScenarioData.SetDate(day.Date.ToString("%F"));
    weatherScenarioData.SetTz(tzName.c_str());
    weatherScenarioData.SetUserDate(userTime.ToString("%F"));
    weatherScenarioData.SetUserTime(userTime.ToString("%H:%M"));
    weatherScenarioData.SetSunrise(day.Sunrise);
    weatherScenarioData.SetSunset(day.Sunset);
    weatherScenarioData.SetTemperature(day.Parts.Day.TempAvg);
    weatherScenarioData.SetIcon(day.Parts.Day.IconUrl());
    weatherScenarioData.SetIconType(day.Parts.Day.Icon);
    weatherScenarioData.MutableCondition()->SetTitle(forecast.L10n.Translate(day.Parts.Day.Condition));
    weatherScenarioData.MutableCondition()->SetFeelsLike(day.Parts.Day.FeelsLike);
    weatherScenarioData.MutableCondition()->SetCloudness(day.Parts.Day.Cloudness);
    weatherScenarioData.MutableCondition()->SetPrecStrength(day.Parts.Day.PrecStrength);
    weatherScenarioData.MutableCondition()->SetPrecType(day.Parts.Day.PrecType);
    weatherScenarioData.MutableTodayDaylight()->SetSunrise(forecast.Today().Sunrise);
    weatherScenarioData.MutableTodayDaylight()->SetSunset(forecast.Today().Sunset);

    FillShowViewGeoLocation(*weatherScenarioData.MutableGeoLocation(), ctx);

    FillDayPartItem(*weatherScenarioData.AddDayPartItems(), day.Parts.Morning, forecast);
    FillDayPartItem(*weatherScenarioData.AddDayPartItems(), day.Parts.Day, forecast);
    FillDayPartItem(*weatherScenarioData.AddDayPartItems(), day.Parts.Evening, forecast);
    if (day.Next) {
        FillDayPartItem(*weatherScenarioData.AddDayPartItems(), day.Next->Parts.Night, forecast);
    }

    return renderData;
}

[[nodiscard]] TWeatherStatus PrepareDayForecastSlots(TWeatherContext& ctx) {
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
        return TWeatherError{EWeatherErrorCode::NOWEATHER} << "Requested date not found";
    }
    const auto& day = *dayPtr;

    auto urlVariant = GetWeatherUrl(ctx, day.Date.MDay());
    if (auto err = std::get_if<TWeatherError>(&urlVariant)) {
        return *err;
    }

    auto dayShortTemp = day.Parts.DayShort.Temp;
    auto nightShortTemp = day.Parts.NightShort.Temp;

    NJson::TJsonValue temperatures = NJson::TJsonArray();
    temperatures.GetArraySafe().push_back(Min(nightShortTemp, dayShortTemp));
    temperatures.GetArraySafe().push_back(Max(nightShortTemp, dayShortTemp));
    forecastSlot["temperature"] = std::move(temperatures);

    forecastSlot["date"] = day.Date.ToString("%F");
    forecastSlot["tz"] = tzName;
    forecastSlot["condition"] = forecast.L10n.Translate(day.Parts.DayShort.Condition);
    forecastSlot["type"] = "weather_for_date";
    forecastSlot["uri"] = std::get<TString>(urlVariant);

    ctx.AddSlot("weather_forecast", "forecast", forecastSlot);

    TVector<ESuggestType> suggests{
        ESuggestType::Feedback,
        ESuggestType::Today,
        ESuggestType::Tomorrow,
        forecast.Fact.PrecStrength > 0 ? ESuggestType::NowcastWhenEnds : ESuggestType::NowcastWhenStarts,
        ESuggestType::SearchFallback,
        ESuggestType::Onboarding
    };
    ctx.Renderer().AddSuggests(suggests);

    ctx.Renderer().AddAnimationDirectives(TDefaultConditionProvider{day.Parts.Day});

    if (ctx.SupportsShowView()) {
        auto renderData = GenerateRenderData(ctx, day);
        ctx.Renderer().Builder().GetResponseBodyBuilder()->AddShowViewDirective(std::move(renderData), NScenarios::TShowViewDirective_EInactivityTimeout_Medium);
        ctx.Renderer().Builder().GetResponseBodyBuilder()->AddClientActionDirective("tts_play_placeholder", {});
    }
    else if (ctx.CanRenderDivCards()) {
        NJson::TJsonValue dayJson;
        NJson::TJsonArray parts;

        dayJson["tz"] = tzName;
        dayJson["background"] = day.Parts.DayShort.BackgroundStyleClass();
        dayJson["date"] = day.Date.ToString("%F");
        dayJson["temp"]["avg"] = day.Parts.Day.TempAvg;
        dayJson["icon"] = day.Parts.Day.IconUrl(48);
        dayJson["condition"]["title"] = forecast.L10n.Translate(day.Parts.Day.Condition);
        dayJson["condition"]["code"] = day.Parts.Day.Condition;
        dayJson["condition"]["feels_like"] = day.Parts.Day.FeelsLike;

        parts.GetArraySafe().push_back(MakePart("morning", day.Parts.Morning, forecast));
        parts.GetArraySafe().push_back(MakePart("day", day.Parts.Day, forecast));
        parts.GetArraySafe().push_back(MakePart("evening", day.Parts.Evening, forecast));
        if (day.Next) {
            parts.GetArraySafe().push_back(MakePart("night", day.Next->Parts.Night, forecast));
        }

        dayJson["parts"] = parts;
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

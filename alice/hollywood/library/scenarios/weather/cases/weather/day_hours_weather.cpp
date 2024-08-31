#include "day_hours_weather.h"

#include <alice/hollywood/library/scenarios/weather/util/util.h>

using NAlice::TDateTime;
using NAlice::TDateTimeList;

namespace NAlice::NHollywood::NWeather {

namespace {

std::pair<int, int> GetMinMaxTemps(const TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();

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

bool IsDayHoursWeatherScenario(const TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();

    auto userTime = forecast.UserTime;
    auto dtlVariant = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TWeatherError>(&dtlVariant)) {
        return false;
    }
    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtlVariant);
    if (dateTimeList->TotalDays() != 1) {
        return false;
    }

    ssize_t daysDiff = dateTimeList->cbegin()->OffsetWidth(userTime);
    return daysDiff < 2;
}

bool IsTomorrowWeatherScenario(const TWeatherContext& ctx) {
    if (!ctx.IsSlotEmpty("day_part")) {
        return false;
    }

    const auto& forecast = *ctx.Forecast();

    auto userTime = forecast.UserTime;
    auto dtlVariant = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TWeatherError>(&dtlVariant)) {
        return false;
    }
    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtlVariant);
    if (dateTimeList->TotalDays() != 1) {
        return false;
    }

    ssize_t daysDiff = dateTimeList->cbegin()->OffsetWidth(userTime);
    return daysDiff == 1;
}

const THour* FigureOutHour(const TForecast& forecast, const TDateTime& dt) {
    const auto* day = forecast.FindDay(dt);
    if (!day) {
        return nullptr;
    }
    switch (dt.DayPart()) {
        case TDateTime::EDayPart::Night:
            return &day->Hours.at(0);
        case TDateTime::EDayPart::Morning:
            return &day->Hours.at(6);
        case TDateTime::EDayPart::Day:
            return &day->Hours.at(12);
        case TDateTime::EDayPart::Evening:
            return &day->Hours.at(18);
        default:
            // Если часть дня не запросили и это не сегодня - показываем с 8 часов
            if (forecast.UserTime.ToString("%F") != dt.SplitTime().ToString("%F")) {
                return &day->Hours.at(8);
            }
            const auto& userHour = forecast.FindHour(TDateTime(forecast.UserTime));
            if (userHour && userHour->Next) {
                return userHour->Next;
            }
            return userHour;
    }
}

const NRenderer::TDivRenderData GenerateRenderData(const TWeatherContext& ctx, const THour& hour) {
    const auto& forecast = *ctx.Forecast();
    auto userTime = forecast.UserTime;
    const auto& tzName = userTime.TimeZone().name();

    const auto& part = hour.DayPart;

    LOG_INFO(ctx.Logger()) << "Adding ShowView Directive";
    NRenderer::TDivRenderData renderData;
    renderData.SetCardId("weather.scenario.div.card");
    auto& weatherScenarioData = *renderData.MutableScenarioData()->MutableWeatherDayHoursData();

    weatherScenarioData.SetDate(part->Day->Date.ToString("%F"));
    weatherScenarioData.SetTz(tzName.c_str());
    weatherScenarioData.SetUserDate(userTime.ToString("%F"));
    weatherScenarioData.SetUserTime(userTime.ToString("%H:%M"));
    weatherScenarioData.SetSunrise(part->Day->Sunrise);
    weatherScenarioData.SetSunset(part->Day->Sunset);
    weatherScenarioData.SetTemperature(part->TempAvg);
    weatherScenarioData.SetIcon(part->IconUrl());
    weatherScenarioData.SetIconType(part->Icon);
    weatherScenarioData.MutableCondition()->SetTitle(forecast.L10n.Translate(part->Condition));
    weatherScenarioData.MutableCondition()->SetFeelsLike(part->FeelsLike);
    weatherScenarioData.MutableCondition()->SetCloudness(part->Cloudness);
    weatherScenarioData.MutableCondition()->SetPrecStrength(part->PrecStrength);
    weatherScenarioData.MutableCondition()->SetPrecType(part->PrecType);
    weatherScenarioData.MutableTodayDaylight()->SetSunrise(forecast.Today().Sunrise);
    weatherScenarioData.MutableTodayDaylight()->SetSunset(forecast.Today().Sunset);

    if (!ctx.IsSlotEmpty("day_part")) {
        weatherScenarioData.SetDayPartType(ctx.FindSlot("day_part")->Value.AsString());
    }

    FillShowViewGeoLocation(*weatherScenarioData.MutableGeoLocation(), ctx);

    for (const auto* currentHour = &hour; currentHour && weatherScenarioData.HourItemsSize() < 24; currentHour = currentHour->Next) {
        FillShowViewHourItem(*weatherScenarioData.AddHourItems(), *currentHour);
    }

    return renderData;
}

void RenderPhraseWeatherForDate(TRenderer& renderer, const bool voiceOnly, const bool useWeatherTomorrowExp) {
    if (voiceOnly) {
        if (useWeatherTomorrowExp) {
            renderer.AddVoiceCard(NNlgTemplateNames::GET_WEATHER, "render_weather_tomorrow_exp");
        } else {
            renderer.AddVoiceCard(NNlgTemplateNames::GET_WEATHER, "render_weather_for_date");
        }
    } else {
        if (useWeatherTomorrowExp) {
            renderer.AddTextCard(NNlgTemplateNames::GET_WEATHER, "render_weather_tomorrow_exp");
        } else {
            renderer.AddTextCard(NNlgTemplateNames::GET_WEATHER, "render_weather_for_date");
        }
    }
}

TWeatherStatus PrepareDayHoursForecastSlots(TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();

    auto userTime = forecast.UserTime;
    auto dtl = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TWeatherError>(&dtl)) {
        return *err;
    }
    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtl);

    auto dt = *dateTimeList->cbegin();
    const auto* hour = FigureOutHour(forecast, dt);

    if (!hour) {
        return TWeatherError(EWeatherErrorCode::NOWEATHER) << "No weather found for the given time";
    }

    auto [tempMin, tempMax] = GetMinMaxTemps(ctx);
    const NJson::TJsonValue tzName(userTime.TimeZone().name());

    const auto& part = *hour->DayPart;
    auto urlVariant = GetWeatherUrl(ctx, part.Day->Date.MDay());
    if (auto err = std::get_if<TWeatherError>(&urlVariant)) {
        return *err;
    }

    NJson::TJsonValue temperatures = NJson::TJsonArray();
    temperatures.GetArraySafe().push_back(tempMin);
    temperatures.GetArraySafe().push_back(tempMax);

    NJson::TJsonValue forecastSlot;
    forecastSlot["condition"] = forecast.L10n.Translate(part.Condition);
    forecastSlot["date"] = part.Day->Date.ToString("%F");
    forecastSlot["temperature"] = temperatures;
    forecastSlot["type"] = "weather_for_date";
    forecastSlot["tz"] = tzName;
    forecastSlot["uri"] = std::get<TString>(urlVariant);

    // Voice warning about tomorrow's weather
    bool useWeatherTomorrowExp = false;
    if (!forecast.Warnings.empty() && IsTomorrowWeatherScenario(ctx)) {
        LOG_INFO(ctx.Logger()) << "Attempt to find GroupedTomorrow warning and change voice-over";
        TMaybe<TString> tomorrowWarningMessage;

        // Trying to get GroupedTomorrow warning
        for (const auto& warning : forecast.Warnings) {
            if (warning.GetCode() == TWeatherProtos::TWarning::GroupedTomorrow) {
                tomorrowWarningMessage = warning.GetMessage();
                break;
            }
        }

        // Building voice answer
        if(tomorrowWarningMessage.Defined()) {
            useWeatherTomorrowExp = true;
            forecastSlot["type"] = "weather_tomorrow_exp";
            forecastSlot["tomorrow_warning_message"] = *tomorrowWarningMessage;
            LOG_INFO(ctx.Logger()) << "GroupedTomorrow warning found (ﾉ^_^)ﾉ: " << tomorrowWarningMessage;
        } else {
            LOG_ERROR(ctx.Logger()) << "Attention: GroupedTomorrow warning not found ¯\\_(ツ)_/¯";
        }
    }

    ctx.AddSlot("weather_forecast", "forecast", forecastSlot);

    TVector<ESuggestType> suggests;
    suggests.push_back(ESuggestType::Feedback);
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
    suggests.push_back(ESuggestType::Onboarding);
    ctx.Renderer().AddSuggests(suggests);

    bool voiceOnly = false;
    if (ctx.SupportsShowView()) {
        auto renderData = GenerateRenderData(ctx, *hour);
        ctx.Renderer().Builder().GetResponseBodyBuilder()->AddShowViewDirective(std::move(renderData), NScenarios::TShowViewDirective_EInactivityTimeout_Medium);
        ctx.Renderer().Builder().GetResponseBodyBuilder()->AddClientActionDirective("tts_play_placeholder", {});
    }
    else if (ctx.CanRenderDivCards()) {
        NJson::TJsonValue dayJson;
        dayJson["date"] = part.Day->Date.ToString("%F");
        dayJson["tz"] = tzName;
        dayJson["background"] = part.BackgroundStyleClass();
        dayJson["temp"]["avg"] = part.TempAvg;
        dayJson["icon"] = part.IconUrl();
        dayJson["condition"]["title"] = forecast.L10n.Translate(part.Condition);
        dayJson["condition"]["feels_like"] = part.FeelsLike;
        dayJson["hours"] = GetHours(*hour);

        NJson::TJsonValue card;
        card["day"] = dayJson;

        ctx.Renderer().AddDivCard(NNlgTemplateNames::GET_WEATHER, "weather__1day_v2", card);
        voiceOnly = true;
    }

    ctx.Renderer().AddAnimationDirectives(TDefaultConditionProvider{part});

    RenderPhraseWeatherForDate(ctx.Renderer(), voiceOnly, useWeatherTomorrowExp);

    return EWeatherOkCode::RESPONSE_ALREADY_RENDERED;
}

} // NAlice::NHollywood::NWeather

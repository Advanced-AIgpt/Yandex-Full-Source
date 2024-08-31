#include "current_weather.h"

#include <alice/hollywood/library/scenarios/weather/util/util.h>
#include <alice/protos/data/scenario/data.pb.h>
#include <library/cpp/timezone_conversion/convert.h>
#include <range/v3/all.hpp>

using namespace ranges;

namespace NAlice::NHollywood::NWeather {

namespace {

TString GetBackgroundType(const TForecast& forecast) {
    auto userTime = forecast.UserTime.ToString("%H:%M");

    const auto& sunrise = forecast.Today().Sunrise;
    const auto& sunset = forecast.Today().Sunset;

    const auto style = CloudinessPrecCssStyle(forecast.Fact.Cloudness, forecast.Fact.PrecStrength);
    if (userTime > sunrise && userTime < sunset) {
        return style + "_day";
    }
    return style + "_night";
}

TString GetDayPart(const TForecast& forecast) {
    auto userTime = forecast.UserTime.ToString("%H:%M");

    const auto& sunrise = forecast.Today().Sunrise;
    const auto& sunset = forecast.Today().Sunset;

    if (userTime > sunrise && userTime < sunset) {
        return "day";
    }
    return "night";
}

TString GetConditionsType(const TForecast& forecast) {
    return CloudinessPrecCssStyle(forecast.Fact.Cloudness, forecast.Fact.PrecStrength);
}

} // namespace

bool IsCurrentWeatherScenario(const TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();
    auto userTime = forecast.UserTime;
    auto dtlVariant = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TWeatherError>(&dtlVariant)) {
        return false;
    }
    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtlVariant);
    return dateTimeList->IsNow();
}

TMaybe<TWeatherError> PrepareCurrentForecastDivCard(TWeatherContext& ctx) {
    const auto& place = *ctx.WeatherPlace();
    const auto& expFlags = ctx.RunRequest().ExpFlags();
    const auto& forecast = *ctx.Forecast();

    const auto userTime = forecast.UserTime;

    const auto* hour = forecast.FindHour(TDateTime(userTime));
    if (!hour) {
        return TWeatherError{EWeatherErrorCode::NOWEATHER} << "No weather found for the given time";
    }

    if (hour->Next) {
        hour = hour->Next;
    }

    const auto& fact = forecast.Fact;
    const NJson::TJsonValue tzName(userTime.TimeZone().name());

    { // Add current day card
        NJson::TJsonValue card;
        NJson::TJsonValue dayJson;

        dayJson["date"] = hour->DayPart->Day->Date.ToString("%F");
        dayJson["tz"] = tzName;
        dayJson["background"] = GetBackgroundType(forecast);
        dayJson["day_part"] = GetDayPart(forecast);
        dayJson["temp"]["avg"] = fact.Temp;
        dayJson["icon"] = fact.IconUrl();
        dayJson["condition"]["title"] = forecast.L10n.Translate(fact.Condition);
        dayJson["condition"]["feels_like"] = fact.FeelsLike;
        dayJson["condition"]["type"] = GetConditionsType(forecast);
        dayJson["hours"] = GetHours(*hour);
        card["day"] = std::move(dayJson);

        ctx.Renderer().AddDivCard(NNlgTemplateNames::GET_WEATHER, "weather__curday_v2", card);
    }

    // Add 10 days card, when user asks about a place where he is not
    auto canShowDays = (
        (expFlags.contains(NExperiment::WEATHER_ALWAYS_DAYS_IN_CURRENT_WEATHER) || place.GetNonUserGeo()) &&
        !expFlags.contains(NExperiment::WEATHER_DISABLE_DAYS_IN_CURRENT_WEATHER)
    );

    if (canShowDays) {
        NJson::TJsonValue card;
        NJson::TJsonValue cardDaysArray = NJson::TJsonArray();

        auto daysCount = std::min(forecast.Days.size(), DAYS_COUNT_IN_CURRENT_WEATHER);

        for (size_t i = 0; i < daysCount; ++i) {
            const auto& day = forecast.Days.at(i);

            if (!day.Next) {
                break;
            }

            const auto& dayShortTemp = day.Parts.DayShort.Temp;
            const auto& nightShortTemp = day.Next->Parts.NightShort.Temp;

            NJson::TJsonValue cardDay;
            cardDay["date"] = day.Date.ToString("%F");

            const auto& week_day = day.Date.WDay();
            cardDay["is_red"] = static_cast<bool>(week_day == 0 || week_day == 6);

            cardDay["tz"] = tzName;
            cardDay["temp"]["min"] = Min(nightShortTemp, dayShortTemp);
            cardDay["temp"]["max"] = Max(nightShortTemp, dayShortTemp);
            cardDay["icon"] = day.Parts.DayShort.IconUrl(48, "dark");
            cardDay["condition"]["title"] = forecast.L10n.Translate(day.Parts.DayShort.Condition);
            cardDay["condition"]["code"] = day.Parts.DayShort.Condition;

            auto urlVariant = GetWeatherUrl(ctx, day.Date.MDay());
            if (auto uri = std::get_if<TString>(&urlVariant)) {
                cardDay["uri"] = *uri;
            }
            cardDaysArray.GetArraySafe().push_back(std::move(cardDay));
        }

        if (!cardDaysArray.GetArraySafe().empty()) {
            card["days"] = std::move(cardDaysArray);

            ctx.Renderer().AddDivCard(NNlgTemplateNames::GET_WEATHER, "weather__days_list_v2", card);
        }
    }

    return Nothing();
}

NRenderer::TDivRenderData GenerateCurrentWeatherRenderData(const TWeatherContext& ctx, const THour& hour) {
    const auto& forecast = *ctx.Forecast();
    const auto userTime = forecast.UserTime;

    LOG_INFO(ctx.Logger()) << "Adding ShowView Directive";
    NRenderer::TDivRenderData renderData;
    renderData.SetCardId("weather.scenario.div.card");
    auto& weatherScenarioData = *renderData.MutableScenarioData()->MutableWeatherDayHoursData();

    const auto& fact = forecast.Fact;
    const auto& tzName = userTime.TimeZone().name();

    weatherScenarioData.SetDate(hour.DayPart->Day->Date.ToString("%F"));
    weatherScenarioData.SetTz(tzName.c_str());
    weatherScenarioData.SetUserDate(userTime.ToString("%F"));
    weatherScenarioData.SetUserTime(userTime.ToString("%H:%M"));
    weatherScenarioData.SetSunrise(forecast.Today().Sunrise);
    weatherScenarioData.SetSunset(forecast.Today().Sunset);
    weatherScenarioData.SetTemperature(fact.Temp);
    weatherScenarioData.SetIcon(fact.IconUrl());
    weatherScenarioData.SetIconType(fact.Icon);
    weatherScenarioData.MutableCondition()->SetTitle(forecast.L10n.Translate(fact.Condition));
    weatherScenarioData.MutableCondition()->SetFeelsLike(fact.FeelsLike);
    weatherScenarioData.MutableCondition()->SetCloudness(fact.Cloudness);
    weatherScenarioData.MutableCondition()->SetPrecStrength(fact.PrecStrength);
    weatherScenarioData.MutableCondition()->SetPrecType(fact.PrecType);
    weatherScenarioData.MutableTodayDaylight()->SetSunrise(forecast.Today().Sunrise);
    weatherScenarioData.MutableTodayDaylight()->SetSunset(forecast.Today().Sunrise);

    FillShowViewGeoLocation(*weatherScenarioData.MutableGeoLocation(), ctx);

    for (const auto* currentHour = &hour; currentHour && weatherScenarioData.HourItemsSize() < 24; currentHour = currentHour->Next) {
        FillShowViewHourItem(*weatherScenarioData.AddHourItems(), *currentHour);
    }

    return renderData;
}

void ExtractPrecWarning(const TWeatherContext& ctx, NJson::TJsonValue& forecastSlot, TVector<TWeatherProtos::TWarning>& warnings) {
    auto precFilter = [](const TWeatherProtos::TWarning& w) { return TodayPrecipitationCodes.contains(w.GetCode()); };
    auto notPrecFilter = [](const TWeatherProtos::TWarning& w) { return !TodayPrecipitationCodes.contains(w.GetCode()); };
    auto precWarnings = warnings | views::filter(precFilter) | to<TVector<TWeatherProtos::TWarning>>();

    if (precWarnings.empty()){
        return;
    }
    auto warning = precWarnings.at(0);

    NDatetime::TCivilSecond begin, now;

    switch (warning.GetCode()) {
    case TWeatherProtos::TWarning::TodayNoPrecipitation:
        return;
    case TWeatherProtos::TWarning::TodayPrecipitationWillEnd:
    case TWeatherProtos::TWarning::TodayPrecipitationWontEnd:
        forecastSlot["condition"] = warning.GetMessage();
        LOG_INFO(ctx.Logger()) << "EXP: WEATHER_NOW_FORECAST_WARNING - new condition from warnings: " << warning.GetMessage();

        warnings = warnings | views::filter(notPrecFilter) | to<TVector<TWeatherProtos::TWarning>>();
        return;
    case TWeatherProtos::TWarning::TodayPrecipitationWillBegin:
        now = ConvertTCivilTime(warning.GetTodayPrecipitationWillBeginContext().GetNow());
        begin = ConvertTCivilTime(warning.GetTodayPrecipitationWillBeginContext().GetPrec().GetBegins());
        break;
    case TWeatherProtos::TWarning::TodayPrecipitationWillBeginAndEnd:
        now = ConvertTCivilTime(warning.GetTodayPrecipitationWillBeginAndEndContext().GetNow());
        begin = ConvertTCivilTime(warning.GetTodayPrecipitationWillBeginAndEndContext().GetPrec().GetBegins());
        break;
    default:
        LOG_ERROR(ctx.Logger()) << "EXP: WEATHER_NOW_FORECAST_WARNING - unexpected precipitation warning";
        return;
    }

    if (!ctx.RunRequest().HasExpFlag(NExperiment::WEATHER_NOW_ONLY_SIGNIFICANT) ||NDatetime::AddHours(now, 2) >= begin){
        forecastSlot["prec_soon_info"] = warning.GetMessage();
        LOG_INFO(ctx.Logger()) << "EXP: WEATHER_NOW_FORECAST_WARNING - extra info about soon precipitation: " << warning.GetMessage();

        warnings = warnings | views::filter(notPrecFilter) | to<TVector<TWeatherProtos::TWarning>>();
    }
}

void PackWarningsInfo(NJson::TJsonValue& slot, TVector<TWeatherProtos::TWarning>& warnings, TString name) {
    slot[name].SetType(NJson::JSON_ARRAY).GetArraySafe() =
        warnings | views::transform([](const TWeatherProtos::TWarning& w) {
            NJson::TJsonValue slot;
            slot["message"] = w.GetMessage();
            return slot;
        }) | to<NJson::TJsonValue::TArray>();
}

void RenderPhraseWeatherCurrent(TRenderer& renderer, const bool voiceOnly, const bool useWeatherCurrentExp) {
    if (voiceOnly) {
        if (useWeatherCurrentExp) {
            renderer.AddVoiceCard(NNlgTemplateNames::GET_WEATHER, "render_weather_current_exp");
        } else {
            renderer.AddVoiceCard(NNlgTemplateNames::GET_WEATHER, "render_weather_current");
        }
    } else {
        if (useWeatherCurrentExp) {
            renderer.AddTextCard(NNlgTemplateNames::GET_WEATHER, "render_weather_current_exp");
        } else {
            renderer.AddTextCard(NNlgTemplateNames::GET_WEATHER, "render_weather_current");
        }
    }
}

[[nodiscard]] TWeatherStatus PrepareCurrentForecastSlots(TWeatherContext& ctx) {
    auto urlVariant = GetWeatherUrl(ctx);
    if (auto err = std::get_if<TWeatherError>(&urlVariant)) {
        return *err;
    }

    const auto& forecast = *ctx.Forecast();
    auto userTime = forecast.UserTime;
    const auto& fact = forecast.Fact;
    const NJson::TJsonValue tzName(userTime.TimeZone().name());

    NJson::TJsonValue forecastSlot;
    forecastSlot["condition"] = forecast.L10n.Translate(fact.Condition);
    forecastSlot["date"] = userTime.ToString("%F");
    forecastSlot["temperature"] = fact.Temp;
    forecastSlot["type"] = "weather_current";
    forecastSlot["tz"] = tzName;
    forecastSlot["uri"] = std::get<TString>(urlVariant);

    // fill info for background sounds
    FillBackgroundSoundsFromFact(ctx.Renderer().BackgroundSounds(), fact);

    bool useWeatherCurrentExp = false;
    if (ctx.RunRequest().HasExpFlag(NExperiment::WEATHER_NOW_FORECAST_WARNING) && !forecast.Warnings.empty()) {
        auto todayFilter = [](const TWeatherProtos::TWarning& w) { return TodayWarningsCodes.contains(w.GetCode()); };

        double threshold = ctx.RunRequest().LoadValueFromExpPrefix<double>(NExperiment::WEATHER_NOW_SIGNIFICANCE_THRESHOLD, 0);
        LOG_INFO(ctx.Logger()) << "EXP: WEATHER_NOW_FORECAST_WARNING - Setting up threshold for warning: " << threshold;
        auto significanceFilter = [threshold](const TWeatherProtos::TWarning& w) { return w.GetSignificance() >= threshold; };

        auto warnings = forecast.Warnings | views::filter(todayFilter) | to<TVector<TWeatherProtos::TWarning>>();
        if (ctx.RunRequest().HasExpFlag(NExperiment::WEATHER_NOW_ONLY_SIGNIFICANT)) {
            LOG_INFO(ctx.Logger()) << "EXP: WEATHER_NOW_FORECAST_WARNING - preparing only significant warnings";
            warnings = warnings | views::filter(significanceFilter) | to<TVector<TWeatherProtos::TWarning>>();
        }

        if (!warnings.empty()) {
            forecastSlot["type"] = "weather_current_exp";
            useWeatherCurrentExp = true;
        }

        ExtractPrecWarning(ctx, forecastSlot, warnings);

        // warnings can become empty after extracting precipitation warning
        if (!warnings.empty()) {
            if (ctx.RunRequest().HasExpFlag(NExperiment::WEATHER_NOW_ONLY_SIGNIFICANT)) {
                PackWarningsInfo(forecastSlot, warnings, "significant_info");
                LOG_INFO(ctx.Logger()) << "EXP: WEATHER_NOW_FORECAST_WARNING - significant today info added";
            } else {
                PackWarningsInfo(forecastSlot, warnings, "extra_info");
                LOG_INFO(ctx.Logger()) << "EXP: WEATHER_NOW_FORECAST_WARNING - extra today info added";
            }
        } else {
            LOG_INFO(ctx.Logger()) << "EXP: WEATHER_NOW_FORECAST_WARNING - no significant warnings are found ¯\\_(ツ)_/¯";
        }
    }

    ctx.AddSlot("weather_forecast", "forecast", forecastSlot);

    TVector<ESuggestType> suggests{
        ESuggestType::Feedback,
        ESuggestType::Tomorrow,
        ESuggestType::Weekend,
        fact.PrecStrength > 0 ? ESuggestType::NowcastWhenEnds : ESuggestType::NowcastWhenStarts,
        ESuggestType::SearchFallback,
        ESuggestType::Onboarding
    };
    ctx.Renderer().AddSuggests(suggests);

    const auto* hour = forecast.FindHour(TDateTime(userTime));
    if (hour && hour->Next) {
        hour = hour->Next;
    }

    bool voiceOnly = false;
    if (ctx.SupportsShowView() && hour) {
        auto renderData = GenerateCurrentWeatherRenderData(ctx, *hour);
        ctx.Renderer().Builder().GetResponseBodyBuilder()->AddShowViewDirective(std::move(renderData), NScenarios::TShowViewDirective_EInactivityTimeout_Medium);
        ctx.Renderer().Builder().GetResponseBodyBuilder()->AddClientActionDirective("tts_play_placeholder", {});
    }
    else if (ctx.CanRenderDivCards()) {
        if (const auto error = PrepareCurrentForecastDivCard(ctx)) {
            return *error;
        }
        voiceOnly = true;
    }

    ctx.Renderer().AddAnimationDirectives(TDefaultConditionProvider{fact});

    RenderPhraseWeatherCurrent(ctx.Renderer(), voiceOnly, useWeatherCurrentExp);

    return EWeatherOkCode::RESPONSE_ALREADY_RENDERED;
}

} // namespace NAlice::NHollywood::NWeather

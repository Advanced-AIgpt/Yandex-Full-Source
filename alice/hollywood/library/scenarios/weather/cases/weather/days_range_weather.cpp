#include "day_part_weather.h"

#include <alice/hollywood/library/scenarios/weather/context/api.h>
#include <alice/hollywood/library/scenarios/weather/util/util.h>

#include <util/generic/utility.h>

namespace NAlice::NHollywood::NWeather {

namespace {

bool IsThisWeekendWeatherScenario(const TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();

    const auto userTime = forecast.UserTime;
    const auto dtlVariant = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TWeatherError>(&dtlVariant)) {
        return false;
    }
    const auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtlVariant);
    if (dateTimeList->TotalDays() != 2) {
        return false;
    }

    THashSet<int> wDays;
    for (auto it = dateTimeList->cbegin(); it < dateTimeList->cend(); it++) {
        const size_t daysDiff = it->OffsetWidth(userTime);
        // Check that all days are not further than a week from now
        if (daysDiff > 6) {
            return false;
        }
        wDays.insert(it->SplitTime().WDay());
    }
    // 0 - Sunday, 6 - saturday
    return wDays.contains(0) && wDays.contains(6);
}

bool IsThisWeekWeatherScenario(const TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();

    const auto userTime = forecast.UserTime;
    const auto dtlVariant = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TWeatherError>(&dtlVariant)) {
        return false;
    }
    const auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtlVariant);
    if (dateTimeList->TotalDays() != 7) {
        return false;
    }

    THashSet<int> daysSinceNow(7);
    for (auto it = dateTimeList->cbegin(); it < dateTimeList->cend(); it++) {
        daysSinceNow.insert(
            it->OffsetWidth(userTime)
        );
    }
    // Check that forecast range is exactly 7 days from now
    for (int i = 0; i < 7; i++) {
        if (!daysSinceNow.contains(i)) {
            return false;
        }
    }
    return true;
}

bool IsNextWeekWeatherScenario(const TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();

    const auto userTime = forecast.UserTime;
    const auto dtlVariant = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TWeatherError>(&dtlVariant)) {
        return false;
    }
    const auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtlVariant);
    if (dateTimeList->TotalDays() > 7) {
        return false;
    }

    THashSet<int> wDays(7);
    size_t minDaysDiff = 1000;
    size_t maxDaysDiff = 0;
    for (auto it = dateTimeList->cbegin(); it < dateTimeList->cend(); it++) {
        const size_t daysDiff = it->OffsetWidth(userTime);
        minDaysDiff = Min(minDaysDiff, daysDiff);
        maxDaysDiff = Max(maxDaysDiff, daysDiff);
        wDays.insert(
            it->SplitTime().WDay()
        );
    }
    // Check that the forecast range is not farther than a week from now
    if (minDaysDiff > 7 || maxDaysDiff - minDaysDiff != dateTimeList->TotalDays() - 1) {
        return false;
    }
    // Check that the forecast range matches the beginning of a week
    for (size_t i = 0; i < dateTimeList->TotalDays(); i++) {
        const int wDay = (i + 1) % 7;
        if (!wDays.contains(wDay)) {
            return false;
        }
    }
    return true;
}

TMaybe<TWeatherProtos::TWarning::ECode> GetWarningType(const TWeatherContext& ctx) {
    if (IsThisWeekendWeatherScenario(ctx)) {
        return TWeatherProtos::TWarning::GroupedWeekend;
    }
    if (IsThisWeekWeatherScenario(ctx)) {
        return TWeatherProtos::TWarning::GroupedThisWeek;
    }
    if (IsNextWeekWeatherScenario(ctx)) {
        return TWeatherProtos::TWarning::GroupedNextWeek;
    }
    return Nothing();
}

const NRenderer::TDivRenderData GenerateRenderData(const TWeatherContext& ctx, const std::unique_ptr<TDateTimeList>& dateTimeList) {

    const auto& forecast = *ctx.Forecast();
    auto userTime = forecast.UserTime;
    const auto& tzName = userTime.TimeZone().name();

    LOG_INFO(ctx.Logger()) << "Adding ShowView Directive";
    NRenderer::TDivRenderData renderData;
    renderData.SetCardId("weather.scenario.div.card");
    auto& weatherScenarioData = *renderData.MutableScenarioData()->MutableWeatherDaysRangeData();

    for (const auto& dt: *dateTimeList) {
        const auto& dayForecastMaybe = forecast.FindDay(dt);

        if (!dayForecastMaybe || !dayForecastMaybe->Next) {
            continue;
        }

        const auto& day = *dayForecastMaybe;
        auto* dayItem = weatherScenarioData.AddDayItems();
        dayItem->SetDate(day.Date.ToString("%F"));
        dayItem->SetTz(tzName.c_str());
        dayItem->SetWeekDay(day.Date.WDay());
        dayItem->SetDayTemp(day.Parts.DayShort.Temp);
        dayItem->SetNightTemp(day.Next->Parts.NightShort.Temp);
        dayItem->SetIcon(day.Parts.DayShort.IconUrl());
        dayItem->SetIconType(day.Parts.DayShort.Icon);
        dayItem->MutableCondition()->SetTitle(forecast.L10n.Translate(day.Parts.DayShort.Condition));
        dayItem->MutableCondition()->SetFeelsLike(day.Parts.DayShort.FeelsLike);
        dayItem->MutableCondition()->SetCloudness(day.Parts.DayShort.Cloudness);
        dayItem->MutableCondition()->SetPrecStrength(day.Parts.DayShort.PrecStrength);

        auto urlVariant = GetWeatherUrl(ctx, day.Date.MDay());
        if (auto uri = std::get_if<TString>(&urlVariant)) {
             dayItem->SetUrl(*uri);
        }
    }

    weatherScenarioData.SetTz(tzName.c_str());
    weatherScenarioData.SetUserDate(userTime.ToString("%F"));
    weatherScenarioData.SetUserTime(userTime.ToString("%H:%M"));
    weatherScenarioData.MutableTodayDaylight()->SetSunrise(forecast.Today().Sunrise);
    weatherScenarioData.MutableTodayDaylight()->SetSunset(forecast.Today().Sunset);
    FillShowViewGeoLocation(*weatherScenarioData.MutableGeoLocation(), ctx);

    return renderData;
}

void RenderPhrase(TRenderer& renderer, const bool voiceOnly, const bool useWeatherDayRangeExp) {
    if (voiceOnly) {
        if (useWeatherDayRangeExp) {
            renderer.AddVoiceCard(NNlgTemplateNames::GET_WEATHER, "render_weather_for_range_exp");
        } else {
            renderer.AddVoiceCard(NNlgTemplateNames::GET_WEATHER, "render_weather_for_range");
        }
    } else {
        if (useWeatherDayRangeExp) {
            renderer.AddTextCard(NNlgTemplateNames::GET_WEATHER, "render_weather_for_range_exp");
        } else {
            renderer.AddTextCard(NNlgTemplateNames::GET_WEATHER, "render_weather_for_range");
        }
    }
}

} // namespace

bool IsDaysRangeWeatherScenario(const TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();
    auto userTime = forecast.UserTime;
    auto dtlVariant = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TWeatherError>(&dtlVariant)) {
        return false;
    }
    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtlVariant);
    return dateTimeList->TotalDays() > 1;
}

[[nodiscard]] TWeatherStatus PrepareDaysRangeForecastSlots(TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();
    auto userTime = forecast.UserTime;

    FixWhenSlotForNextWeekend(ctx);

    auto dtl = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TWeatherError>(&dtl)) {
        return *err;
    }
    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtl);

    auto urlVariant = GetWeatherMonthUrl(ctx);
    if (auto err = std::get_if<TWeatherError>(&urlVariant)) {
        return *err;
    }
    const NJson::TJsonValue tzName(userTime.TimeZone().name());

    NJson::TJsonValue card;
    NJson::TJsonValue forecastSlot;

    NJson::TJsonValue cardDaysArray = NJson::TJsonArray();
    NJson::TJsonValue forecastDaysArray = NJson::TJsonArray();

    for (const auto& dt: *dateTimeList) {
        const auto& dayForecastMaybe = forecast.FindDay(dt);

        if (!dayForecastMaybe || !dayForecastMaybe->Next) {
            ctx.Renderer().AddAttention("incomplete_forecast");
            continue;
        }

        const auto& day = *dayForecastMaybe;
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

        NJson::TJsonValue slotDay;
        slotDay["date"] = day.Date.ToString("%F");
        slotDay["tz"] = tzName;
        slotDay["condition"] = forecast.L10n.Translate(day.Parts.DayShort.Condition);

        NJson::TJsonValue temperatures = NJson::TJsonArray();
        temperatures.GetArraySafe().push_back(Min(nightShortTemp, dayShortTemp));
        temperatures.GetArraySafe().push_back(Max(nightShortTemp, dayShortTemp));
        slotDay["temperature"] = std::move(temperatures);

        forecastDaysArray.GetArraySafe().push_back(std::move(slotDay));
    }

    if (forecastDaysArray.GetArraySafe().size() == 0) {
        return TWeatherError{EWeatherErrorCode::NOWEATHER} << "Requested range not found";
    }

    card["days"] = std::move(cardDaysArray);
    forecastSlot["days"] = std::move(forecastDaysArray);

    forecastSlot["type"] = "weather_for_range";
    forecastSlot["uri"] = std::get<TString>(urlVariant);

    bool useWeatherWarningExp = false;
    if (ctx.RunRequest().HasExpFlag(NExperiment::WEATHER_FOR_RANGE_FORECAST_WARNING) && !forecast.Warnings.empty()) {
        LOG_INFO(ctx.Logger()) << "EXP: WEATHER_DAY_RANGE_FORECAST_WARNING - Attempt to find warning and change voice-over";

        const auto warningType = GetWarningType(ctx);
        TMaybe<TString> warningMessage;

        if (warningType.Defined()) {
            // Trying to get warning message
            for (const auto& warning : forecast.Warnings) {
                if (warning.GetCode() == *warningType) {
                    warningMessage = warning.GetMessage();
                    break;
                }
            }
        }

        // Building voice answer
        if(warningMessage.Defined()) {
            useWeatherWarningExp = true;
            forecastSlot["type"] = "weather_day_range_exp";
            forecastSlot["day_range_warning_message"] = *warningMessage;
            LOG_INFO(ctx.Logger()) << "EXP: WEATHER_DAY_RANGE_FORECAST_WARNING - warning found (ﾉ^_^)ﾉ: " << warningMessage;
        } else {
            LOG_ERROR(ctx.Logger()) << "EXP: WEATHER_DAY_RANGE_FORECAST_WARNING - Attention: warning not found ¯\\_(ツ)_/¯";
        }
    }

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

    bool voiceOnly = false;
    if (ctx.SupportsShowView()) {
        auto renderData = GenerateRenderData(ctx, dateTimeList);
        ctx.Renderer().Builder().GetResponseBodyBuilder()->AddShowViewDirective(std::move(renderData), NScenarios::TShowViewDirective_EInactivityTimeout_Medium);
        ctx.Renderer().Builder().GetResponseBodyBuilder()->AddClientActionDirective("tts_play_placeholder", {});
    }
    else if (ctx.CanRenderDivCards()) {
        ctx.Renderer().AddDivCard(NNlgTemplateNames::GET_WEATHER, "weather__days_list_v2", card);
        voiceOnly = true;
    }

    RenderPhrase(ctx.Renderer(), voiceOnly, useWeatherWarningExp);

    return EWeatherOkCode::RESPONSE_ALREADY_RENDERED;
}

} // namespace NAlice::NHollywood::NWeather

#include "today_weather.h"
#include "alice/protos/data/scenario/centaur/teasers/teaser_settings.pb.h"
#include "current_weather.h"

#include <alice/hollywood/library/scenarios/weather/cases/nowcast/nowcast_day_part_weather.h>
#include <alice/hollywood/library/scenarios/weather/cases/nowcast/nowcast_for_now_weather.h>
#include <alice/hollywood/library/scenarios/weather/util/util.h>

#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/common/frame_request_params.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/protos/data/scenario/centaur/main_screen.pb.h>
#include <alice/protos/data/scenario/data.pb.h>

namespace NAlice::NHollywood::NWeather {

bool IsTodayWeatherScenario(const TWeatherContext& ctx) {
    if (ctx.RunRequest().HasExpFlag(NExperiment::DISABLE_NEW_NLG) && !ctx.RunRequest().HasExpFlag(NExperiment::NEW_NLG_COMPARE)) {
        return false;
    }
    if (!ctx.IsSlotEmpty("day_part")) { // "Погода сегодня вечером" or "Погода вечером"
        return false;
    }
    if (ctx.IsSlotEmpty("when")) { // "Погода"
        return true;
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
    return daysDiff == 0;
}

enum TemperatureType {
    MinMax, Avg
};

void FillWeatherTeaserData(NData::TWeatherTeaserData& weatherScenarioData, const THour& hour, const TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();
    const auto userTime = forecast.UserTime;

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

    FillShowViewGeoLocation(*weatherScenarioData.MutableGeoLocation(), ctx);

    for (const auto* currentHour = &hour; currentHour && weatherScenarioData.HourItemsSize() < 24; currentHour = currentHour->Next) {
        FillShowViewHourItem(*weatherScenarioData.AddHourItems(), *currentHour);
    }
}

const NRenderer::TDivRenderData GenerateWeatherTeaserRenderData(const TWeatherContext& ctx, const THour& hour) {
    LOG_INFO(ctx.Logger()) << "Adding AddCard Directive";
    NRenderer::TDivRenderData renderData;
    renderData.SetCardId("weather.teaser.div.card");
    auto& weatherScenarioData = *renderData.MutableScenarioData()->MutableWeatherTeaserData();

    FillWeatherTeaserData(weatherScenarioData, hour, ctx);
    
    return renderData;
}

const NData::TScenarioData GenerateTeasersPreviewData(const TWeatherContext& ctx, const THour& hour) {
    LOG_INFO(ctx.Logger()) << "Generating Teaser Preview Data";

    NData::TScenarioData scenarioData;
    auto& previewData = *scenarioData.MutableTeasersPreviewData();
    auto& teaserPreview = *previewData.AddTeaserPreviews();

    teaserPreview.SetTeaserName("Погода");
    auto& teaserConfig = *teaserPreview.MutableTeaserConfigData();
    teaserConfig.SetTeaserType("Weather");
    auto& weatherScenarioData = *teaserPreview.MutableTeaserPreviewScenarioData()->MutableWeatherTeaserData();

    FillWeatherTeaserData(weatherScenarioData, hour, ctx);
    
    return scenarioData;
}

const NRenderer::TDivRenderData GenerateWeatherMainScreenRenderData(const TWeatherContext& ctx, const THour& hour) {
    const auto& forecast = *ctx.Forecast();
    const auto userTime = forecast.UserTime;

    LOG_INFO(ctx.Logger()) << "Adding WeatherMainScreenData";
    NRenderer::TDivRenderData renderData;
    renderData.SetCardId("weather.main_screen.div.card");
    auto& weatherMainScreenData = *renderData.MutableScenarioData()->MutableWeatherMainScreenData();

    const auto& fact = forecast.Fact;
    const auto& tzName = userTime.TimeZone().name();

    weatherMainScreenData.SetDate(hour.DayPart->Day->Date.ToString("%F"));
    weatherMainScreenData.SetTz(tzName.c_str());
    weatherMainScreenData.SetUserDate(userTime.ToString("%F"));
    weatherMainScreenData.SetUserTime(userTime.ToString("%H:%M"));
    weatherMainScreenData.SetSunrise(forecast.Today().Sunrise);
    weatherMainScreenData.SetSunset(forecast.Today().Sunset);
    weatherMainScreenData.SetTemperature(fact.Temp);
    weatherMainScreenData.SetIcon(fact.IconUrl());
    weatherMainScreenData.SetIconType(fact.Icon);
    weatherMainScreenData.MutableCondition()->SetTitle(forecast.L10n.Translate(fact.Condition));
    weatherMainScreenData.MutableCondition()->SetFeelsLike(fact.FeelsLike);
    weatherMainScreenData.MutableCondition()->SetCloudness(fact.Cloudness);
    weatherMainScreenData.MutableCondition()->SetPrecStrength(fact.PrecStrength);
    weatherMainScreenData.MutableCondition()->SetPrecType(fact.PrecType);

    FillShowViewGeoLocation(*weatherMainScreenData.MutableGeoLocation(), ctx);

    return renderData;
}

const NData::TScenarioData GenerateWeatherWidgetScenarioData(TWeatherContext& ctx) {
    LOG_INFO(ctx.Logger()) << "Adding CentaurWidgetCardItem - WeatherCardData";

    NData::TScenarioData scenarioData;
    auto& widgetData = *scenarioData.MutableCentaurScenarioWidgetData();
    widgetData.SetWidgetType("weather");
    auto& cardData = *widgetData.AddWidgetCards();
    auto& weatherCardData = *cardData.MutableWeatherCardData();

    const auto& forecast = *ctx.Forecast();
    const auto userTime = forecast.UserTime;
    const auto& fact = forecast.Fact;

    auto locationSlot = ctx.FindSlot("forecast_location");
    if (!IsSlotEmpty(locationSlot)) {
        NJson::TJsonValue locationJson;
        NJson::ReadJsonFastTree(locationSlot->Value.AsString(), &locationJson);
        weatherCardData.SetCity(locationJson["city"].GetString());
    }
    weatherCardData.SetTemperature(fact.Temp);
    weatherCardData.SetImage(fact.IconUrl());
    weatherCardData.SetComment(forecast.L10n.Translate(fact.Condition));
    weatherCardData.SetSunrise(forecast.Today().Sunrise);
    weatherCardData.SetSunset(forecast.Today().Sunset);
    weatherCardData.SetUserTime(userTime.ToString("%H:%M"));
    weatherCardData.MutableCondition()->SetCloudness(fact.Cloudness);
    weatherCardData.MutableCondition()->SetPrecStrength(fact.PrecStrength);

    if (ctx.RunRequest().HasExpFlag(NExperiment::CENTAUR_TYPED_ACTION_EXP_FLAG_NAME)) {
        TTypedSemanticFrame tsf;
        tsf.MutableWeatherSemanticFrame();
        google::protobuf::Any typedAction;
        typedAction.PackFrom(std::move(tsf));
        cardData.MutableTypedAction()->CopyFrom(typedAction);
    } else {
        NScenarios::TFrameAction onClickAction;
        auto* parsedUtterance = onClickAction.MutableParsedUtterance();
        parsedUtterance->MutableTypedSemanticFrame()->MutableWeatherSemanticFrame();
        parsedUtterance->MutableParams()->SetDisableOutputSpeech(true);
        parsedUtterance->MutableParams()->SetDisableShouldListen(true);

        auto* analytics = parsedUtterance->MutableAnalytics();
        analytics->SetProductScenario("CentaurMainScreen");
        analytics->SetPurpose("open_weather");
        analytics->SetOrigin(TAnalyticsTrackingModule_EOrigin_SmartSpeaker);

        const TString frameActionId = "OnClickMainScreenWeatherCard";
        cardData.SetAction("@@mm_deeplink#" + frameActionId);
        ctx.Renderer().Builder().GetResponseBodyBuilder()->AddAction(frameActionId, std::move(onClickAction));
    }

    return scenarioData;
}

NJson::TJsonValue PrepareForecastByDayPart(const TDayPart& dayPart, const TemperatureType temperatureType, const TForecast& forecast) {
    NJson::TJsonValue slot;

    slot["day_part"] = TDateTime::DayPartAsString(dayPart.Type);

    if (temperatureType == TemperatureType::MinMax) {
        NJson::TJsonValue temperature = NJson::TJsonArray();
        temperature.GetArraySafe().push_back(dayPart.TempMin);
        temperature.GetArraySafe().push_back(dayPart.TempMax);
        slot["temperature"] = temperature;
    } else {
        slot["temperature"] = dayPart.TempAvg;
    }

    slot["condition"] = forecast.L10n.Translate(dayPart.Condition);
    slot["precipitation_type"] = dayPart.PrecType;
    slot["precipitation_current"] = dayPart.IsPrecipitation();

    return slot;
}

void RenderPhraseWeatherToday(TRenderer& renderer, const bool voiceOnly, const bool useWeatherTodayExp) {
    if (voiceOnly) {
        if (useWeatherTodayExp) {
            renderer.AddVoiceCard(NNlgTemplateNames::GET_WEATHER, "render_weather_today_exp");
        } else {
            renderer.AddVoiceCard(NNlgTemplateNames::GET_WEATHER, "render_weather_today");
        }
    } else {
        if (useWeatherTodayExp) {
            renderer.AddTextCard(NNlgTemplateNames::GET_WEATHER, "render_weather_today_exp");
        } else {
            renderer.AddTextCard(NNlgTemplateNames::GET_WEATHER, "render_weather_today");
        }
    }
}

TWeatherStatus PrepareTodayForecastSlots(TWeatherContext& ctx) {
    auto urlVariant = GetWeatherUrl(ctx);
    if (auto err = std::get_if<TWeatherError>(&urlVariant)) {
        return *err;
    }

    const auto& forecast = *ctx.Forecast();

    auto userTime = forecast.UserTime;
    const auto& fact = forecast.Fact;
    const NJson::TJsonValue tzName(userTime.TimeZone().name());

    const auto currentDayPartType = TDateTime::TimeToDayPart(forecast.UserTime);

    NJson::TJsonValue forecastSlot;
    forecastSlot["condition"] = forecast.L10n.Translate(fact.Condition);
    forecastSlot["date"] = userTime.ToString("%F");
    forecastSlot["temperature"] = fact.Temp;
    forecastSlot["type"] = "weather_today";
    forecastSlot["tz"] = tzName;
    forecastSlot["uri"] = std::get<TString>(urlVariant);
    forecastSlot["day_part"] = TDateTime::DayPartAsString(currentDayPartType);

    // fill info for background sounds
    FillBackgroundSoundsFromFact(ctx.Renderer().BackgroundSounds(), fact);

    bool useWeatherTodayExp = false;
    // Voice warning about today's weather
    if (ctx.RunRequest().HasExpFlag(NExperiment::WEATHER_TODAY_FORECAST_WARNING) && !forecast.Warnings.empty()) {
        LOG_INFO(ctx.Logger()) << "EXP: WEATHER_TODAY_FORECAST_WARNING - Attempt to find GroupedToday warning and change voice-over";
        TMaybe<TString> todayWarningMessage;

        // Trying to get GroupedToday warning
        for (const auto& warning : forecast.Warnings) {
            if (warning.GetCode() == TWeatherProtos::TWarning::GroupedToday) {
                todayWarningMessage = warning.GetMessage();
                break;
            }
        }

        // Building voice answer
        if(todayWarningMessage.Defined()) {
            useWeatherTodayExp = true;
            forecastSlot["type"] = "weather_today_exp";
            forecastSlot["today_warning_message"] = *todayWarningMessage;
            LOG_INFO(ctx.Logger()) << "EXP: WEATHER_TODAY_FORECAST_WARNING - GroupedToday warning found (ﾉ^_^)ﾉ: " << todayWarningMessage;
        } else {
            LOG_ERROR(ctx.Logger()) << "EXP: WEATHER_TODAY_FORECAST_WARNING - Attention: GroupedToday warning not found ¯\\_(ツ)_/¯";
        }
    }

    ctx.AddSlot("weather_forecast", "forecast", forecastSlot);

    if (currentDayPartType == TDateTime::EDayPart::Morning && ctx.RunRequest().HasExpFlag(NExperiment::NEW_NLG_COMPARE)) {
        NJson::TJsonValue yesterdaySlot;
        yesterdaySlot["temperature"] = forecast.Yesterday.Temp;
        ctx.AddSlot("yesterday_forecast", "forecast", yesterdaySlot);
    }

    bool isCurrentPrecipitation = forecast.Fact.IsPrecipitation();

    ctx.AddSlot("precipitation_current", "num", ToString(static_cast<int>(isCurrentPrecipitation)));

    TPtrWrapper<TSlot> precTypeSlot = ctx.FindOrAddSlot("precipitation_type", "num");
    const_cast<TSlot*>(precTypeSlot.Get())->Value = TSlot::TValue{ToString(forecast.Fact.PrecType)};

    auto dateSlot = ctx.FindOrAddSlot("date", "string");
    auto tzSlot = ctx.FindOrAddSlot("tz", "string");
    auto precDayPartSlot = ctx.FindOrAddSlot("precipitation_day_part", "string");
    auto precChangeHoursSlot = ctx.FindOrAddSlot("precipitation_change_hours", "num");
    auto precNextDayPartSlot = ctx.FindOrAddSlot("precipitation_next_day_part", "string");
    auto precNextTypeSlot = ctx.FindOrAddSlot("precipitation_next_type", "num");
    auto precNextChangeHoursSlot = ctx.FindOrAddSlot("precipitation_next_change_hours", "num");
    auto nowcastSlot = ctx.FindOrAddSlot("weather_nowcast_alert", "string");

    // Remove previous values
    const_cast<TSlot*>(precDayPartSlot.Get())->Value = TSlot::TValue{"null"};
    const_cast<TSlot*>(precNextDayPartSlot.Get())->Value = TSlot::TValue{"null"};
    const_cast<TSlot*>(precNextTypeSlot.Get())->Value = TSlot::TValue{"null"};
    const_cast<TSlot*>(precNextChangeHoursSlot.Get())->Value = TSlot::TValue{"null"};
    const_cast<TSlot*>(nowcastSlot.Get())->Value = TSlot::TValue{"null"};

    const_cast<TSlot*>(precChangeHoursSlot.Get())->Value = TSlot::TValue{"0"};

    if (forecast.Days.size() > 0) {
        int foundChanges = 0;
        const TInstant time = TInstant::Seconds(forecast.Now);
        const TInstant timeNextDay = time + TDuration::Days(1);

        for (
            const THour* hour = forecast.FindHour(TDateTime(forecast.UserTime));
            hour && hour->HourTS <= timeNextDay;
            hour = hour->Next
        ) {
            // Первое изменение осадков
            if (isCurrentPrecipitation != hour->IsPrecipitation() && foundChanges == 0) {
                if (hour->DayPart) {
                    const_cast<TSlot*>(precDayPartSlot.Get())->Value = TSlot::TValue{TString{TDateTime::DayPartAsString(hour->DayPart->Type)}};
                }
                const_cast<TSlot*>(precChangeHoursSlot.Get())->Value = TSlot::TValue{ToString((hour->HourTS - time).Hours() + 1)};
                if (!isCurrentPrecipitation) {
                    const_cast<TSlot*>(precTypeSlot.Get())->Value = TSlot::TValue{ToString(hour->PrecType)};
                }
                ++foundChanges;
                isCurrentPrecipitation = hour->IsPrecipitation();
                continue;
            }

            const_cast<TSlot*>(dateSlot.Get())->Value = TSlot::TValue{forecast.UserTime.ToString("%F-%T")};
            const_cast<TSlot*>(tzSlot.Get())->Value = TSlot::TValue{TString{forecast.UserTime.TimeZone().name()}};
            // ASSISTANT-3085: Поддержать закончится-начнется со стороны BASS
            const_cast<TSlot*>(precNextChangeHoursSlot.Get())->Value = TSlot::TValue{"0"};
            if (isCurrentPrecipitation != hour->IsPrecipitation() && foundChanges == 1) {
                if (hour->DayPart) {
                    const_cast<TSlot*>(precNextDayPartSlot.Get())->Value = TSlot::TValue{TString{TDateTime::DayPartAsString(hour->DayPart->Type)}};
                }
                const_cast<TSlot*>(precNextChangeHoursSlot.Get())->Value = TSlot::TValue{ToString((hour->HourTS - time).Hours() + 1)};
                if (!isCurrentPrecipitation) {
                    const_cast<TSlot*>(precNextTypeSlot.Get())->Value = TSlot::TValue{ToString(hour->PrecType)};
                }
                break;
            }
        }
    }

    switch (currentDayPartType) {
        case TDateTime::EDayPart::Night:
            ctx.AddSlot("forecast_next", "forecast", PrepareForecastByDayPart(forecast.Tomorrow().Parts.Day, TemperatureType::MinMax, forecast));
            ctx.RemoveSlot("forecast_next_next"); // removing slot that could been left from previous query
            break;
        case TDateTime::EDayPart::Morning:
            ctx.AddSlot("forecast_next", "forecast", PrepareForecastByDayPart(forecast.Today().Parts.Day, TemperatureType::MinMax, forecast));
            ctx.AddSlot("forecast_next_next", "forecast", PrepareForecastByDayPart(forecast.Today().Parts.Evening, TemperatureType::Avg, forecast));
            break;
        case TDateTime::EDayPart::Day:
            ctx.AddSlot("forecast_next", "forecast", PrepareForecastByDayPart(forecast.Today().Parts.Evening, TemperatureType::Avg, forecast));
            ctx.AddSlot("forecast_next_next", "forecast", PrepareForecastByDayPart(forecast.Tomorrow().Parts.Night, TemperatureType::Avg, forecast));
            break;
        case TDateTime::EDayPart::Evening:
            ctx.AddSlot("forecast_next", "forecast", PrepareForecastByDayPart(forecast.Tomorrow().Parts.Night, TemperatureType::Avg, forecast));
            ctx.RemoveSlot("forecast_next_next"); // removing slot that could been left from previous query
            break;
        default:
            LOG_ERROR(ctx.Logger()) << "Failed to generate forecast_next* because current day part is unexpected (" << currentDayPartType << ").";
            break;
    }

    if (ctx.Nowcast()) {
        const auto& nowcast = *ctx.Nowcast();
        auto isNowcastForNowCase = IsNowcastForNowCase(ctx);
        if (auto err = std::get_if<TWeatherError>(&isNowcastForNowCase)) {
            LOG_ERROR(ctx.Logger()) << "Error checking IsNowcastForNowCase: " << err->Message();
        } else if (std::get<bool>(isNowcastForNowCase)) {
            const_cast<TSlot*>(nowcastSlot.Get())->Value = TSlot::TValue{nowcast.Text};
        }
    }

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
    if (ctx.IsCollectCardRequest() && hour) {
        auto renderData = GenerateWeatherTeaserRenderData(ctx, *hour);
        ctx.Renderer().Builder().GetResponseBodyBuilder()->AddScenarioData(renderData.GetScenarioData());
        if(ctx.RunRequest().HasExpFlag("teaser_settings")) {
            ctx.Renderer().Builder().GetResponseBodyBuilder()->AddCardDirectiveWithTeaserTypeAndId(std::move(renderData), NScenarios::TAddCardDirective_EChromeLayerType::TAddCardDirective_EChromeLayerType_None, "Weather");
        } else {
            ctx.Renderer().Builder().GetResponseBodyBuilder()->AddCardDirective(std::move(renderData), NScenarios::TAddCardDirective_EChromeLayerType::TAddCardDirective_EChromeLayerType_None);
        }
    }
    else if (ctx.IsCollectTeasersPreviewRequest() && hour){
        auto scenarioData = GenerateTeasersPreviewData(ctx, *hour);
        ctx.Renderer().Builder().GetResponseBodyBuilder()->AddScenarioData(scenarioData);
    }
    else if ((ctx.IsCollectMainScreenRequest() || ctx.IsCollectWidgetGalleryRequest()) && hour) {
        if (ctx.RunRequest().HasExpFlag(NExperiment::SCENARIO_WIDGET_MECHANICS_EXP_FLAG_NAME)) {
            ctx.Renderer().Builder().GetResponseBodyBuilder()->AddScenarioData(GenerateWeatherWidgetScenarioData(ctx));
        } else {
            auto renderData = GenerateWeatherMainScreenRenderData(ctx, *hour);
            ctx.Renderer().Builder().GetResponseBodyBuilder()->AddScenarioData(renderData.GetScenarioData());
        }
    }
    else if (ctx.SupportsShowView() && hour) {
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

    if (!ctx.IsCollectCardRequest() && !ctx.IsCollectMainScreenRequest()) {
        ctx.Renderer().AddAnimationDirectives(TDefaultConditionProvider{fact});
    }

    RenderPhraseWeatherToday(ctx.Renderer(), voiceOnly, useWeatherTodayExp);

    return EWeatherOkCode::RESPONSE_ALREADY_RENDERED;
}

} // namespace NAlice::NHollywood::NWeather

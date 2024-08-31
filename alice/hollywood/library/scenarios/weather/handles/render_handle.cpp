#include "render_handle.h"

#include <alice/hollywood/library/scenarios/weather/cases/change.h>
#include <alice/hollywood/library/scenarios/weather/cases/nowcast/nowcast_by_hours_weather.h>
#include <alice/hollywood/library/scenarios/weather/cases/nowcast/nowcast_day_part_weather.h>
#include <alice/hollywood/library/scenarios/weather/cases/nowcast/nowcast_default_weather.h>
#include <alice/hollywood/library/scenarios/weather/cases/nowcast/nowcast_for_now_weather.h>
#include <alice/hollywood/library/scenarios/weather/cases/nowcast/switch_weather.h>
#include <alice/hollywood/library/scenarios/weather/cases/nowcast/nowcast_util.h>
#include <alice/hollywood/library/scenarios/weather/cases/pressure_cases.h>
#include <alice/hollywood/library/scenarios/weather/cases/weather/current_weather.h>
#include <alice/hollywood/library/scenarios/weather/cases/weather/day_hours_weather.h>
#include <alice/hollywood/library/scenarios/weather/cases/weather/day_part_weather.h>
#include <alice/hollywood/library/scenarios/weather/cases/weather/day_weather.h>
#include <alice/hollywood/library/scenarios/weather/cases/weather/days_range_weather.h>
#include <alice/hollywood/library/scenarios/weather/cases/weather/today_weather.h>
#include <alice/hollywood/library/scenarios/weather/cases/wind/today_wind.h>
#include <alice/hollywood/library/scenarios/weather/cases/wind/current_wind.h>
#include <alice/hollywood/library/scenarios/weather/cases/wind/day_part_wind.h>
#include <alice/hollywood/library/scenarios/weather/cases/wind/day_wind.h>
#include <alice/hollywood/library/scenarios/weather/cases/wind/days_range_wind.h>
#include <alice/hollywood/library/scenarios/weather/context/context.h>
#include <alice/hollywood/library/scenarios/weather/util/util.h>

#include <alice/hollywood/library/resources/geobase.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/weather/proto/weather.pb.h>

#include <util/string/join.h>
#include <util/string/printf.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NWeather {

namespace {

TWeatherStatus WeatherNowcastRenderDoImpl(TWeatherContext& ctx) {
    if (ctx.IsCollectCardRequest() || ctx.IsCollectTeasersPreviewRequest()) {
        return TWeatherError{EWeatherErrorCode::NOWEATHER} << "Not supported for collect card scenario case";
    }

    // fix when slot
    auto when = ctx.FindSlot("when");
    if (!IsSlotEmpty(when)) {
        NJson::TJsonValue whenJson;
        NJson::ReadJsonFastTree(when->Value.AsString(), &whenJson);

        if (whenJson["days"].GetInteger() == 0 && whenJson["days_relative"].GetBoolean() == true) {
            const_cast<TSlot*>(when.Get())->Value = TSlot::TValue{"null"};
        }
    }

    // OLD TODO: Remove unused slots
    TPtrWrapper<TSlot> nowCastSlot = ctx.FindOrAddSlot("weather_nowcast_alert", "string");
    TPtrWrapper<TSlot> precDayPartSlot = ctx.FindOrAddSlot("precipitation_day_part", "string");
    TPtrWrapper<TSlot> precForDayPartSlot = ctx.FindOrAddSlot("precipitation_for_day_part", "num");
    TPtrWrapper<TSlot> currentPrecSlot = ctx.FindOrAddSlot("precipitation_current", "num");
    TPtrWrapper<TSlot> precChangeHoursSlot = ctx.FindOrAddSlot("precipitation_change_hours", "num");
    TPtrWrapper<TSlot> phraseNumber = ctx.FindOrAddSlot("set_number", "num");
    TPtrWrapper<TSlot> precTypeSlot = ctx.FindOrAddSlot("precipitation_type", "num");

    // clean old values
    const_cast<TSlot*>(precTypeSlot.Get())->Value = TSlot::TValue{"null"};
    const_cast<TSlot*>(nowCastSlot.Get())->Value = TSlot::TValue{"null"};
    const_cast<TSlot*>(currentPrecSlot.Get())->Value = TSlot::TValue{"null"};
    const_cast<TSlot*>(precDayPartSlot.Get())->Value = TSlot::TValue{"null"};
    const_cast<TSlot*>(precForDayPartSlot.Get())->Value = TSlot::TValue{"null"};

    const_cast<TSlot*>(precChangeHoursSlot.Get())->Value = TSlot::TValue{"0"};

    const auto& forecast = *ctx.Forecast();
    const size_t now = forecast.Now;
    constexpr size_t RANDOM_CAP = 100;
    if (IsSlotEmpty(phraseNumber)) {
        const_cast<TSlot*>(phraseNumber.Get())->Value = TSlot::TValue{ToString(now % RANDOM_CAP)};
    } else {
        size_t newPhraseNumber = (*phraseNumber->Value.As<size_t>() + 1) % RANDOM_CAP;
        const_cast<TSlot*>(phraseNumber.Get())->Value = TSlot::TValue{ToString(newPhraseNumber)};
    }

    const auto isGetWeatherScenario = IsGetWeatherScenario(ctx);
    if (const auto err = std::get_if<TWeatherError>(&isGetWeatherScenario)) {
        return *err;
    }
    if (std::get<bool>(isGetWeatherScenario)) {
        LOG_INFO(ctx.Logger()) << "GetWeatherScenario";
        return EWeatherSkipBranchCode::SKIP_CURRENT_SCENARIO_BRANCH;
    }

    auto isNowcastForNowCase = IsNowcastForNowCase(ctx);
    if (auto err = std::get_if<TWeatherError>(&isNowcastForNowCase)) {
        return *err;
    }
    if (std::get<bool>(isNowcastForNowCase)) {
        LOG_INFO(ctx.Logger()) << "PrepareNowcastForNow";
        return PrepareNowcastForNow(ctx);
    }

    // find fact value of precipitation
    bool isCurrentPrecipitation = forecast.Fact.PrecStrength > 0.0;
    const_cast<TSlot*>(currentPrecSlot.Get())->Value = TSlot::TValue{ToString(static_cast<int>(isCurrentPrecipitation))};

    // set current precipitation type in slot
    // it won't changed if precipitation ended in future
    const_cast<TSlot*>(precTypeSlot.Get())->Value = TSlot::TValue{ToString(forecast.Fact.PrecType)};

    if (IsDayPartForecastCase(ctx)) {
        LOG_INFO(ctx.Logger()) << "PrepareNowcastDayPartForecastSlot";
        return PrepareNowcastDayPartForecastSlot(ctx);
    }

    if (IsByHoursForecastCase(ctx)) {
        LOG_INFO(ctx.Logger()) << "PrepareNowcastByHoursForecastSlot";
        return PrepareNowcastByHoursForecastSlot(ctx);
    }

    LOG_INFO(ctx.Logger()) << "PrepareNowcastDefaultSlot";
    return PrepareNowcastDefaultSlot(ctx);
}

TWeatherStatus WeatherPrecMapRenderDoImpl(TWeatherContext& ctx) {
    if (ctx.CanOpenLink()) {
        const TString precMapUrl = MakeNowcastNowUrl(ctx);
    	ctx.Renderer().AddOpenUriDirective(precMapUrl);
        auto button = ctx.Renderer().CreateOpenUriButton("weather_nowcast", precMapUrl);
        ctx.Renderer().AddTextWithButtons(NNlgTemplateNames::GET_WEATHER_NOWCAST, "render_prec_map", {button});
    } else if (ctx.CanRenderDivCards()) {
        auto divCard = MakeDivCard(ctx);
        AddNowcastDivCardBlock(ctx, divCard);
        SuggestNowcastNowUrlSlot(ctx);
        ctx.Renderer().AddTextCard(NNlgTemplateNames::GET_WEATHER_NOWCAST, "render_prec_map");
    } else {
        ctx.Renderer().AddAttention("can_not_display_prec_map");
        return EWeatherSkipBranchCode::SKIP_CURRENT_SCENARIO_BRANCH;
    }

    TVector<ESuggestType> suggests;
    suggests.push_back(ESuggestType::Feedback);
    suggests.push_back(ESuggestType::SearchFallback);
    suggests.push_back(ESuggestType::Onboarding);
    ctx.Renderer().AddSuggests(suggests);

    return EWeatherOkCode::RESPONSE_ALREADY_RENDERED;
}

TWeatherStatus WeatherRenderDoImpl(TWeatherContext& ctx) {
    if (IsTodayWeatherScenario(ctx) || ctx.IsCollectCardRequest() || ctx.IsCollectTeasersPreviewRequest() && ctx.RunRequest().HasExpFlag("teaser_settings")) {
        LOG_INFO(ctx.Logger()) << "PrepareTodayForecastsSlots";
        return PrepareTodayForecastSlots(ctx);
    }

    if (IsCurrentWeatherScenario(ctx)) {
        LOG_INFO(ctx.Logger()) << "PrepareCurrentForecastSlots";
        return PrepareCurrentForecastSlots(ctx);
    }

    if (IsDayHoursWeatherScenario(ctx)) {
        LOG_INFO(ctx.Logger()) << "PrepareDayHoursForecastSlots";
        return PrepareDayHoursForecastSlots(ctx);
    }

    if (IsDayPartWeatherScenario(ctx)) {
        LOG_INFO(ctx.Logger()) << "PrepareDayPartForecastSlots";
        return PrepareDayPartForecastSlots(ctx);
    }

    if (IsDayWeatherScenario(ctx)) {
        LOG_INFO(ctx.Logger()) << "PrepareDayForecastSlots";
        return PrepareDayForecastSlots(ctx);
    }

    if (IsDaysRangeWeatherScenario(ctx)) {
        LOG_INFO(ctx.Logger()) << "PrepareDaysRangeForecastSlots";
        return PrepareDaysRangeForecastSlots(ctx);
    }

    return TWeatherError{EWeatherErrorCode::NOWEATHER} << "Failed to match case for request";
}

TWeatherStatus WeatherPressureRenderDoImpl(TWeatherContext& ctx) {
    if (ctx.IsCollectCardRequest() || ctx.IsCollectTeasersPreviewRequest()) {
        return TWeatherError{EWeatherErrorCode::NOPRESSURE} << "Not supported for collect card scenario case";
    }

    if (IsTodayWeatherScenario(ctx)) {
        LOG_INFO(ctx.Logger()) << "PrepareTodayPressureForecastsSlots";
        return PrepareTodayPressureForecastSlots(ctx);
    }

    if (IsCurrentWeatherScenario(ctx)) {
        LOG_INFO(ctx.Logger()) << "PrepareCurrentPressureForecastSlots";
        return PrepareCurrentPressureForecastSlots(ctx);
    }

    if (IsDayPartWeatherScenario(ctx)) {
        LOG_INFO(ctx.Logger()) << "PrepareDayPartPressureForecastSlots";
        return PrepareDayPartPressureForecastSlots(ctx);
    }

    if (IsDayWeatherScenario(ctx)) {
        LOG_INFO(ctx.Logger()) << "PrepareDayPressureForecastSlots";
        return PrepareDayPressureForecastSlots(ctx);
    }

    if (IsDaysRangeWeatherScenario(ctx)) {
        LOG_INFO(ctx.Logger()) << "PrepareDaysRangePressureForecastSlots";
        return PrepareDaysRangePressureForecastSlots(ctx);
    }

    return TWeatherError{EWeatherErrorCode::NOPRESSURE} << "Failed to match case for request";
}

TWeatherStatus WeatherWindRenderDoImpl(TWeatherContext& ctx) {
    if (ctx.IsCollectCardRequest() || ctx.IsCollectTeasersPreviewRequest() ) {
        return TWeatherError{EWeatherErrorCode::NOWIND} << "Not supported for collect card scenario case";
    }

    if (IsTodayWeatherScenario(ctx)) {
        LOG_INFO(ctx.Logger()) << "PrepareTodayWindForecastsSlots";
        return PrepareTodayWindForecastSlots(ctx);
    }

    if (IsCurrentWeatherScenario(ctx)) {
        LOG_INFO(ctx.Logger()) << "PrepareCurrentWindForecastSlots";
        return PrepareCurrentWindForecastSlots(ctx);
    }

    if (IsDayPartWeatherScenario(ctx)) {
        LOG_INFO(ctx.Logger()) << "PrepareDayPartWindForecastSlots";
        return PrepareDayPartWindForecastSlots(ctx);
    }

    if (IsDayWeatherScenario(ctx)) {
        LOG_INFO(ctx.Logger()) << "PrepareDayWindForecastSlots";
        return PrepareDayWindForecastSlots(ctx);
    }

    if (IsDaysRangeWeatherScenario(ctx)) {
        LOG_INFO(ctx.Logger()) << "PrepareDaysRangeWindForecastSlots";
        return PrepareDaysRangeWindForecastSlots(ctx);
    }

    return TWeatherError{EWeatherErrorCode::NOWIND} << "Failed to match case for request";
}

TWeatherStatus WeatherChangeRenderDoImpl(TWeatherContext& ctx) {
    if (ctx.IsCollectCardRequest()) {
        return TWeatherError{EWeatherErrorCode::NOCHANGE} << "Not supported for collect card scenario case";
    }

    LOG_INFO(ctx.Logger()) << "PreparePrecipitationChangeSlots";
    return PreparePrecipitationChangeSlots(ctx);
}

void WriteError(TWeatherContext& ctx, const TWeatherError& error) {
    auto& renderer = ctx.Renderer();

    LOG_ERROR(ctx.Logger()) << "Got an error: " << error.Message().Quote();
    renderer.AddError(error.Code());

    TVector<ESuggestType> suggests;
    suggests.push_back(ESuggestType::Feedback);
    if (error.Code() != EWeatherErrorCode::NOGEOFOUND) {
        suggests.push_back(ESuggestType::Today);
        suggests.push_back(ESuggestType::Tomorrow);
    }
    suggests.push_back(ESuggestType::SearchFallback);
    renderer.AddSuggests(suggests);
}

void RenderDoImpl(TWeatherContext& ctx) {
    auto& renderer = ctx.Renderer();

    const TMaybe<TWeatherError> prepareCityError = ctx.GetError("prepare_city_error");
    if (prepareCityError) {
        WriteError(ctx, *prepareCityError);
        return;
    }

    LOG_INFO(ctx.Logger()) << ctx.Frame()->Name();

    // try to print nowcast error info if present
    if (const auto alertError = ctx.Protos().AlertError()) {
        LOG_INFO(ctx.Logger()) << "Got alert error: (" << alertError->GetStatusCode() << ") " << alertError->GetMessage();
    }

    if (!ctx.Forecast()) {
        WriteError(ctx, TWeatherError{EWeatherErrorCode::WEATHERERROR} << "No correct answer from Weather API");
        return;
    }

    if (auto error = AddForecastLocationSlots(ctx)) {
        LOG_ERROR(ctx.Logger()) << "Failed to add forecast location slots: " << error->Message().Quote();
        WriteError(ctx, *error);
        return;
    }

    if (IsPressureWeatherScenario(ctx.Frame()->Name())) {
        const auto weatherStatus = WeatherPressureRenderDoImpl(ctx);
        if (const auto* error = std::get_if<TWeatherError>(&weatherStatus)) {
            LOG_ERROR(ctx.Logger()) << "Got an error: " << error->Message().Quote();
            renderer.AddError(error->Code());
        }
        return;
    }

    if (IsWindWeatherScenario(ctx.Frame()->Name())) {
        const auto weatherStatus = WeatherWindRenderDoImpl(ctx);
        if (const auto* error = std::get_if<TWeatherError>(&weatherStatus)) {
            LOG_ERROR(ctx.Logger()) << "Got an error: " << error->Message().Quote();
            renderer.AddError(error->Code());
        }
        return;
    }

    if (IsPrecMapWeatherScenario(ctx.Frame()->Name())) {
        const auto weatherStatus = WeatherPrecMapRenderDoImpl(ctx);
        if (std::holds_alternative<EWeatherSkipBranchCode>(weatherStatus)) {
            LOG_INFO(ctx.Logger()) << "Switch to Nowcast scenario from Precipitation Map scenario";
            ctx.Frame()->SetName(TString{NFrameNames::GET_WEATHER_NOWCAST});
        } else {
            if (const auto* error = std::get_if<TWeatherError>(&weatherStatus)) {
                LOG_ERROR(ctx.Logger()) << "Got an error: " << error->Message().Quote();
                renderer.AddError(error->Code());
            }
            return;
        }
    }

    if (IsWeatherChangeScenario(ctx.Frame()->Name())) {
        const auto weatherStatus = WeatherChangeRenderDoImpl(ctx);
        if (const auto* error = std::get_if<TWeatherError>(&weatherStatus)) {
            LOG_ERROR(ctx.Logger()) << "Got an error: " << error->Message().Quote();
            renderer.AddError(error->Code());
        }
        return;
    }

    if (IsNowcastWeatherScenario(ctx.Frame()->Name()) && ctx.Nowcast()) {
        const auto weatherStatus = WeatherNowcastRenderDoImpl(ctx);
        if (std::holds_alternative<EWeatherSkipBranchCode>(weatherStatus)) {
            LOG_INFO(ctx.Logger()) << "Switch to Weather scenario from Nowcast scenario";
            ctx.Frame()->SetName(TString{NFrameNames::GET_WEATHER});
        } else if (const auto* error = std::get_if<TWeatherError>(&weatherStatus)) {
            LOG_ERROR(ctx.Logger()) << "Got an error: " << error->Message().Quote();
            renderer.AddError(error->Code());
            return;
        } else {
            const auto okCode = std::get<EWeatherOkCode>(weatherStatus);
            if (okCode == EWeatherOkCode::NEED_RENDER_RESPONSE) {
                renderer.AddTextCard(NNlgTemplateNames::GET_WEATHER_NOWCAST, "render_result");
            }
            return;
        }
    }

    {
        const auto weatherStatus = WeatherRenderDoImpl(ctx);
        if (const auto* error = std::get_if<TWeatherError>(&weatherStatus)) {
            WriteError(ctx, *error);
        }
    }
}

} // namespace

void TWeatherRenderHandle::Do(TScenarioHandleContext& ctx) const {
    TWeatherContext weatherCtx{ctx, /* forecastReady = */ true};

    RenderDoImpl(weatherCtx);

    auto& renderer = weatherCtx.Renderer();
    renderer.SetProductScenarioName("weather");
    renderer.SetIntentName(weatherCtx.Frame()->Name());

    renderer.Render();
    auto response = *std::move(renderer.Builder()).BuildResponse();

    // Save Weather state
    TSemanticFrame semanticFrame = weatherCtx.Frame()->ToProto();
    TWeatherState weatherState;
    weatherState.MutableSemanticFrame()->MergeFrom(semanticFrame);
    weatherState.SetClientTimeMs(weatherCtx.ClientTime().MilliSeconds());

    auto* body = response.MutableResponseBody();
    body->MutableState()->PackFrom(weatherState);
    body->MutableSemanticFrame()->MergeFrom(semanticFrame);

    // Write answer
    weatherCtx->ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);

    if (auto responseBodyBuilder = renderer.Builder().GetResponseBodyBuilder()) {
        for (auto const& [cardId, cardData] : responseBodyBuilder->GetRenderData()) {
            LOG_INFO(ctx.Ctx.Logger()) << "Adding render_data to context";
            ctx.ServiceCtx.AddProtobufItem(cardData, RENDER_DATA_ITEM);
        }
    }
}

}  // namespace NAlice::NHollywood::NWeather

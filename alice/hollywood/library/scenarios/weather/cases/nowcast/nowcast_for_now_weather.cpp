#include "nowcast_for_now_weather.h"
#include "nowcast_by_hours_weather.h"
#include "nowcast_util.h"

namespace NAlice::NHollywood::NWeather {

TWeatherErrorOr<bool> IsNowcastForNowCase(const TWeatherContext& ctx) {
    if (!ctx.Nowcast()) {
        return false;
    }

    const auto& forecast = *ctx.Forecast();
    auto userTime = forecast.UserTime;
    auto dtl = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TWeatherError>(&dtl)) {
        return *err;
    }

    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtl);
    if (!dateTimeList->IsNow()) {
        return false;
    }

    const auto& nowcast = *ctx.Nowcast();
    const TStringBuf state = nowcast.State;
    return state != "nodata" && state != "norule";
}

TWeatherStatus PrepareNowcastForNow(TWeatherContext& ctx) {
    // getting forecast and nowcast
    const auto& forecast = *ctx.Forecast();
    const auto& nowcast = *ctx.Nowcast();

    bool isCurrentPrecipitation = forecast.Fact.PrecStrength > 0.0;

    if (ctx.RunRequest().HasExpFlag(NExperiment::WEATHER_SEARCH_PRECS_IN_HOURS) && isCurrentPrecipitation == 0.0) {
        return PrepareNowcastByHoursForecastSlot(ctx);
    }

    ctx.AddSlot("precipitation_current", "num", ToString(static_cast<int>(isCurrentPrecipitation)));

    TPtrWrapper<TSlot> nowCastSlot = ctx.FindOrAddSlot("weather_nowcast_alert", "string");
    const_cast<TSlot*>(nowCastSlot.Get())->Value = TSlot::TValue{nowcast.Text};

    TPtrWrapper<TSlot> precTypeSlot = ctx.FindOrAddSlot("precipitation_type", "num");
    const_cast<TSlot*>(precTypeSlot.Get())->Value = TSlot::TValue{ToString(forecast.Fact.PrecType)};

    TVector<ESuggestType> suggests;
    suggests.push_back(ESuggestType::Feedback);
    suggests.push_back(ESuggestType::SearchFallback);
    suggests.push_back(ESuggestType::Onboarding);
    ctx.Renderer().AddSuggests(suggests);

    const TString nowcastUrl = MakeNowcastNowUrl(ctx);

    if (!ctx.CanOpenLink()) {
        return EWeatherOkCode::NEED_RENDER_RESPONSE;
    }

    // opening nowcast via suggest button (test)
    if (ctx.CanOpenLink() && ctx.RunRequest().HasExpFlag(NExperiment::WEATHER_OPEN_NOWCAST_IN_SUGGEST)) {
        SuggestNowcastNowUrlSlot(ctx);
        ctx.Renderer().AddSuggests({ESuggestType::OpenUri});
        return EWeatherOkCode::NEED_RENDER_RESPONSE;
    }

    // opening nowcast directly in a few seconds (button can be pushed before)
    ctx.Renderer().AddOpenUriDirective(nowcastUrl);

    // opening nowcast directly in a few seconds (test)
    if (ctx.CanOpenLink() && ctx.RunRequest().HasExpFlag(NExperiment::WEATHER_OPEN_NOWCAST_IN_BROWSER)) {
        return EWeatherOkCode::NEED_RENDER_RESPONSE;
    }

    // adding button to open nowcast immediately after click
    auto button = ctx.Renderer().CreateOpenUriButton("weather_nowcast", nowcastUrl);
    ctx.Renderer().AddTextWithButtons(NNlgTemplateNames::GET_WEATHER_NOWCAST, "render_result", {button});

    return EWeatherOkCode::RESPONSE_ALREADY_RENDERED;
}

} // namespace NAlice::NHollywood::NWeather

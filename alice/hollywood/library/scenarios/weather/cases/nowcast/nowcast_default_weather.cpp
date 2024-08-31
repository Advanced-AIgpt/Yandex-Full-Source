#include "nowcast_day_part_weather.h"
#include "nowcast_util.h"

namespace NAlice::NHollywood::NWeather {

[[nodiscard]] TWeatherStatus PrepareNowcastDefaultSlot(TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();

    TVector<ESuggestType> suggests;
    suggests.push_back(ESuggestType::Feedback);
    suggests.push_back(ESuggestType::SearchFallback);
    suggests.push_back(ESuggestType::Onboarding);
    ctx.Renderer().AddSuggests(suggests);

    if (ctx.CanRenderDivCards()) {
        auto divCard = MakeDivCard(ctx);
        // DIALOG-3578: Отображать карточку с картой для случая, когда осадки в процессе
        if (forecast.Fact.PrecStrength > 0.0 && ctx.Nowcast()) {
            AddNowcastDivCardBlock(ctx, divCard);
            SuggestNowcastNowUrlSlot(ctx);
            return EWeatherOkCode::NEED_RENDER_RESPONSE;
        } else {
            ctx.Renderer().AddDivCard(NNlgTemplateNames::GET_WEATHER_NOWCAST, "weather__precipitation", divCard);
        }
    }

    SuggestHomeUrlSlot(ctx);
    return EWeatherOkCode::NEED_RENDER_RESPONSE;
}

} // namespace NAlice::NHollywood::NWeather

#include "nowcast_day_part_weather.h"
#include "nowcast_util.h"

namespace NAlice::NHollywood::NWeather {

namespace {

TString MakeDetailsUrl(TWeatherContext& ctx) {
    TString startUrl("https://yandex.ru/pogoda/details/today?");

    const auto [lat, lon] = ctx.GetLatLon();

    TCgiParameters frontendCgi;
    frontendCgi.InsertUnescaped("lat", ToString(lat));
    frontendCgi.InsertUnescaped("lon", ToString(lon));
    frontendCgi.InsertUnescaped("from", "alice_raincard");

    if (!ctx.RunRequest().ClientInfo().IsDesktop()) {
        frontendCgi.InsertUnescaped("appsearch_header", "1");
    }

    if (IsTurbo(ctx)) {
        TCgiParameters turboCgi;
        turboCgi.InsertEscaped("text", startUrl + frontendCgi.Print());

        turboCgi.InsertUnescaped("utm_source", "alice");
        turboCgi.InsertUnescaped("utm_campaign", "card");
        turboCgi.InsertUnescaped("utm_medium", "nowcast");
        turboCgi.InsertUnescaped("utm_content", "details_rain");

        return TString("https://yandex.ru/turbo?") + turboCgi.Print();
    }
    frontendCgi.InsertUnescaped("utm_source", "alice");
    frontendCgi.InsertUnescaped("utm_campaign", "card");
    frontendCgi.InsertUnescaped("utm_medium", "nowcast");
    frontendCgi.InsertUnescaped("utm_content", "details_rain");

    return startUrl + frontendCgi.Print();
}

void SuggestDetailsUrlSlot(TWeatherContext &ctx) {
    PrepareWeatherOpenUriSlot(ctx, MakeDetailsUrl(ctx));
}

} // namespace

bool IsDayPartForecastCase(const TWeatherContext& ctx) {
    return !ctx.IsSlotEmpty("day_part");
}

TWeatherStatus PrepareNowcastDayPartForecastSlot(TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();

    auto userTime = forecast.UserTime;
    auto nextUserDay = TDateTime::TSplitTime(userTime.TimeZone(), userTime.AsTimeT() + TDuration::Days(1).Seconds());

    auto userDateString = userTime.ToString("%F");
    auto userNextDayString = nextUserDay.ToString("%F");
    int userHour = userTime.Hour();

    TPtrWrapper<TSlot> precForDayPartSlot = ctx.FindOrAddSlot("precipitation_for_day_part", "num");
    TPtrWrapper<TSlot> precTypeSlot = ctx.FindOrAddSlot("precipitation_type", "num");
    const_cast<TSlot*>(precTypeSlot.Get())->Value = TSlot::TValue{ToString(forecast.Fact.PrecType)};

    const auto dayPart = ctx.FindSlot("day_part");
    auto pt = dayPart->Value.AsString();
    const auto& days = forecast.Days;
    bool showDetails = false;

    for (const auto& day : days) {
        // today and tomorrow
        const auto& dateString = day.Date;
        const auto& parts = day.Parts;

        if (dateString.ToString("%F") != userDateString && dateString.ToString("%F") != userNextDayString) {
            continue;
        }

        const auto func = [&parts](const TStringBuf part) -> TMaybe<const TDayPart*> {
            if (part == "night") return &parts.Night;
            if (part == "morning") return &parts.Morning;
            if (part == "day") return &parts.Day;
            if (part == "evening") return &parts.Evening;
            return Nothing();
        };

        const auto dpMaybe = func(pt);
        if (!dpMaybe) {
            continue;
        }

        const auto& part = *(*dpMaybe);
        // check if day part is in past
        // this is not really necessary because of DateTimeList logic:
        //   past day part interpreted as tomorrow
        // check anyway for stability reasons
        TStringBuf left, right;
        TStringBuf{part.Source}.RSplit(",", left, right);
        int maxHourInPart;
        bool success = TryFromString(right, maxHourInPart);

        if (success && maxHourInPart >= userHour) { // requested day part not in past
            bool hasPrec = part.PrecStrength > 0.0;;
            const_cast<TSlot*>(precForDayPartSlot.Get())->Value = TSlot::TValue{ToString(static_cast<int>(hasPrec))};

            if (hasPrec) {
                const_cast<TSlot*>(precTypeSlot.Get())->Value = TSlot::TValue{ToString(part.PrecType)};
            }

            showDetails = true;
            break;
        }

        return EWeatherSkipBranchCode::SKIP_CURRENT_SCENARIO_BRANCH;
    }

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
            SuggestDetailsUrlSlot(ctx);
            return EWeatherOkCode::NEED_RENDER_RESPONSE;
        } else {
            ctx.Renderer().AddDivCard(NNlgTemplateNames::GET_WEATHER_NOWCAST, "weather__precipitation", divCard);
        }
    }

    if (showDetails) {
        SuggestDetailsUrlSlot(ctx);
    } else {
        SuggestHomeUrlSlot(ctx);
    }
    return EWeatherOkCode::NEED_RENDER_RESPONSE;
}

} // namespace NAlice::NHollywood::NWeather

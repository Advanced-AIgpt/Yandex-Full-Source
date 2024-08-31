#include "nowcast_day_part_weather.h"
#include "nowcast_util.h"

namespace NAlice::NHollywood::NWeather {

namespace {

bool IsPrecipitation(const THour& hour) {
    return hour.PrecStrength > 0.0;
}

TStringBuf DayPart(const THour& hour) {
    if (hour.Hour < 5)
        return "night";
    if (hour.Hour < 12)
        return "morning";
    if (hour.Hour < 18)
        return "day";
    return "evening";
}

} // namespace

bool IsByHoursForecastCase(const TWeatherContext& ctx) {
    return ctx.IsSlotEmpty("day_part");
}

[[nodiscard]] TWeatherStatus PrepareNowcastByHoursForecastSlot(TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();
    auto time = TInstant::Seconds(forecast.Now);
    auto timeNextDay = time + TDuration::Days(1);

    auto isCurrentPrecipitation = forecast.Fact.PrecStrength > 0.0;

    ctx.AddSlot("precipitation_current", "num", ToString(static_cast<int>(isCurrentPrecipitation)));

    TPtrWrapper<TSlot> precDayPartSlot = ctx.FindOrAddSlot("precipitation_day_part", "string");
    TPtrWrapper<TSlot> precTypeSlot = ctx.FindOrAddSlot("precipitation_type", "num");
    TPtrWrapper<TSlot> precChangeHoursSlot = ctx.FindOrAddSlot("precipitation_change_hours", "num");

    const_cast<TSlot*>(precTypeSlot.Get())->Value = TSlot::TValue{ToString(forecast.Fact.PrecType)};

    const auto& days = forecast.Days;
    TVector<const THour*> hours;
    for (const auto& day : days) {
        for (const auto& hour: day.Hours) {
            hours.push_back(&hour);
        }
    }

    int foundChanges = 0;
    for (const auto* hourPtr : hours) {
        const auto& hour = *hourPtr;

        // Ищем изменения осадков в 24-х часовом окне
        if (hour.HourTS <= time) {
            continue;
        }

        if (hour.HourTS > timeNextDay) {
            break;
        }

        // Первое изменение осадков
        if (isCurrentPrecipitation != IsPrecipitation(hour) && foundChanges == 0) {
            const_cast<TSlot*>(precDayPartSlot.Get())->Value = TSlot::TValue{TString{DayPart(hour)}};
            const_cast<TSlot*>(precChangeHoursSlot.Get())->Value = TSlot::TValue{ToString((hour.HourTS - time).Hours() + 1)};

            if (!isCurrentPrecipitation) {
                const_cast<TSlot*>(precTypeSlot.Get())->Value = TSlot::TValue{ToString(hour.PrecType)};
            }

            foundChanges += 1;
            isCurrentPrecipitation = IsPrecipitation(hour);
            continue;
        }

        auto userTime = forecast.UserTime;
        ctx.AddSlot("date", "string", userTime.ToString("%F-%T"));
        ctx.AddSlot("tz", "string", TString{userTime.TimeZone().name()});

        // ASSISTANT-3085: Поддержать закончится-начнется со стороны BASS
        TPtrWrapper<TSlot> precNextDayPartSlot = ctx.FindOrAddSlot("precipitation_next_day_part", "string");
        TPtrWrapper<TSlot> precNextTypeSlot = ctx.FindOrAddSlot("precipitation_next_type", "num");
        TPtrWrapper<TSlot> precNextChangeHoursSlot = ctx.FindOrAddSlot("precipitation_next_change_hours", "num");
        const_cast<TSlot*>(precNextChangeHoursSlot.Get())->Value = TSlot::TValue{"0"};

        if (isCurrentPrecipitation != IsPrecipitation(hour) && foundChanges == 1) {
            const_cast<TSlot*>(precNextDayPartSlot.Get())->Value = TSlot::TValue{TString{DayPart(hour)}};
            const_cast<TSlot*>(precNextChangeHoursSlot.Get())->Value = TSlot::TValue{ToString((hour.HourTS - time).Hours() + 1)};

            if (!isCurrentPrecipitation) {
                const_cast<TSlot*>(precNextTypeSlot.Get())->Value = TSlot::TValue{ToString(hour.PrecType)};
            }

            break;
        }
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

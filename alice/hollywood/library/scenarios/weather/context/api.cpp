#include "api.h"

namespace NAlice::NHollywood::NWeather {

namespace {

constexpr TStringBuf ICONS_BASE_URL = "https://yastatic.net/weather/i/icons/portal/png";

constexpr TStringBuf MOCK_FORECAST_RAIN_STARTS_SOON_EXP = "weather_mock_forecast_rain_starts_soon";
constexpr TStringBuf MOCK_FORECAST_RAIN_ENDS_SOON_EXP = "weather_mock_forecast_rain_ends_soon";
constexpr TStringBuf MOCK_FORECAST_NO_PRECIPITATION = "weather_mock_forecast_no_precipitation";

THashMap<TString, TWeatherProtos::TDayPart> ConstructDayPartsMap(const TWeatherProtos::TPreparedForecastItem& day) {
    THashMap<TString, TWeatherProtos::TDayPart> result;
    for (const auto& part : day.GetParts()) {
        result[part.first] = part.second;
    }
    return result;
}

double UnpackDouble(int value) {
    // unpacking trick - ask dimastark@ for details
    switch (value) {
        case 1:
            return 0.25;
        case 2:
            return 0.50;
        case 3:
            return 0.75;
        case 4:
            return 1.00;
        default:
            return 0.00;
    }
}

void MockForecastFromExps(const TWeatherContext& ctx, TForecast& forecast) {
    auto time = TInstant::Seconds(forecast.Now);

    const auto setRaining = [&](const std::function<bool(const THour&)> condition, bool rainingNow) {
        auto& days = forecast.Days;

        if (rainingNow) {
            forecast.Fact.PrecType = 1;
            forecast.Fact.PrecStrength = 1.0;
        } else {
            forecast.Fact.PrecType = 0;
            forecast.Fact.PrecStrength = 0.0;
        }

        for (auto& day : days) {
            for (auto& hour : day.Hours) {
                if (hour.HourTS <= time) {
                    continue;
                }
                if (condition(hour)) {
                    hour.PrecType = 1;
                    hour.PrecStrength = 1.0;
                } else {
                    hour.PrecType = 0;
                    hour.PrecStrength = 0.0;
                }
            }
        }
    };

    if (ctx.RunRequest().HasExpFlag(MOCK_FORECAST_RAIN_STARTS_SOON_EXP)) {
        setRaining([&](const THour& hour) {
            int hoursSkip = (hour.HourTS - time).Hours() + 1;
            return hoursSkip >= 2;
        }, /* rainingNow = */ false);
    } else if (ctx.RunRequest().HasExpFlag(MOCK_FORECAST_RAIN_ENDS_SOON_EXP)) {
        setRaining([&](const THour& hour) {
            int hoursSkip = (hour.HourTS - time).Hours() + 1;
            return hoursSkip < 2;
        }, /* rainingNow = */ true);
    } else if (ctx.RunRequest().HasExpFlag(MOCK_FORECAST_NO_PRECIPITATION)) {
        setRaining([&](const THour&) { return false; }, /* rainingNow = */ false);
    }
}

void MockNowcastFromExps(const TWeatherContext& ctx, TNowcast& nowcast) {
    if (ctx.RunRequest().HasExpFlag(MOCK_FORECAST_RAIN_STARTS_SOON_EXP)) {
        nowcast.State = "begins";
        nowcast.Text = "Дождь начнется через 2 часа";
    } else if (ctx.RunRequest().HasExpFlag(MOCK_FORECAST_RAIN_ENDS_SOON_EXP)) {
        nowcast.State = "ends";
        nowcast.Text = "Дождь закончится через 2 часа";
    } else if (ctx.RunRequest().HasExpFlag(MOCK_FORECAST_NO_PRECIPITATION)) {
        nowcast.State = "noprec";
        nowcast.Text = "Дождя не будет";
    }
}

} // namespace

TString CloudinessPrecCssStyle(const double cloudiness, const double precStrength) {
    if (cloudiness < 0.01 && precStrength < 0.01) {
        return "clear";
    }

    if (precStrength < 0.01) {
        if (cloudiness > 0.75) {
            return "overcast_light_prec";
        }
        return "cloudy";
    }

    if (precStrength < 0.5) {
        return "overcast_light_prec";
    }

    return "overcast_prec";
};

TString MakeIconUrl(const TString& icon, const size_t size, const TString& theme) {
    return TStringBuilder() << ICONS_BASE_URL << "/" << size << "x" << size << "/" << theme << "/" << icon
                            << ".png";
};

TTzInfo::TTzInfo(const TWeatherProtos::TPreparedForecast& forecast)
    : Name{forecast.GetTzInfo().GetName()}
    , Offset{static_cast<int>(forecast.GetTzInfo().GetOffset())}
{
}

TString TTzInfo::TimeZoneName() const {
    if (!Name.StartsWith("UTC")) {
        return Name;
    }

    return TStringBuilder{}
        << "Etc/GMT"
        << (Offset > 0 ? "-" : "+")
        << (std::abs(Offset) / 60 / 60);
}

TInfo::TInfo(const TWeatherProtos::TPreparedForecast& forecast)
    : TzInfo{forecast}
{
}

TYesterday::TYesterday(const TWeatherProtos::TPreparedForecast& forecast)
    : Temp{static_cast<double>(forecast.GetYesterday().GetTemperature())}
{
}

TFact::TFact(const TWeatherProtos::TPreparedForecast& forecast)
    : Cloudness{forecast.GetFact().GetCloudiness()}
    , PrecStrength{forecast.GetFact().GetPrecipitationsStrength()}
    , Temp{static_cast<double>(forecast.GetFact().GetTemperature())}
    , FeelsLike{static_cast<double>(forecast.GetFact().GetFeelsLike())}
    , PrecType{static_cast<size_t>(forecast.GetFact().GetPrecipitationsType())}
    , WindSpeed{static_cast<double>(forecast.GetFact().GetWindSpeed())}
    , WindGust{static_cast<double>(forecast.GetFact().GetWindGust().value())}
    , WindDir{forecast.GetFact().GetWindDirection()}
    , PressureMM{forecast.GetFact().GetPressureMM()}
    , Icon{forecast.GetFact().GetIcon().value()}
    , Condition{forecast.GetFact().GetCondition().value()}
    , DayTime{forecast.GetFact().GetDayTime()}
    , Season{forecast.GetFact().GetSeason()}
{
}

TString TFact::IconUrl(const size_t size, const TString& theme) const {
    return MakeIconUrl(Icon, size, theme);
}

bool TFact::IsPrecipitation() const {
    return PrecStrength > 0.0;
}

TShortDayPart::TShortDayPart(const TWeatherProtos::TDayPart& dayPart, const NAlice::TDateTime::EDayPart type)
    : Type{type}
    , Temp{static_cast<double>(dayPart.GetTemp().value())}
    , FeelsLike{static_cast<double>(dayPart.GetTemperatureFeels().value())}
    , Cloudness{UnpackDouble(dayPart.GetCloudiness().value())}
    , PrecStrength{UnpackDouble(dayPart.GetPrecStrength().value())}
    , WindSpeed{static_cast<double>(dayPart.GetWindSpeed().value())}
    , WindGust{static_cast<double>(dayPart.GetWindGust().value())}
    , WindDir{dayPart.GetWindDir().value()}
    , PressureMM{dayPart.GetPressureMM().value()}
    , Icon{dayPart.GetIcon().value()}
    , Condition{dayPart.GetCondition().value()} {
}

TString TShortDayPart::IconUrl(const size_t size, const TString& theme) const {
    return MakeIconUrl(Icon, size, theme);
}

TString TShortDayPart::BackgroundStyleClass() const {
    if (Type == NAlice::TDateTime::EDayPart::Morning || Type == NAlice::TDateTime::EDayPart::Day) {
        return CloudinessPrecCssStyle(Cloudness, PrecStrength) + "_day";
    }

    return CloudinessPrecCssStyle(Cloudness, PrecStrength) + "_night";
}

TDayPart::TDayPart(const TWeatherProtos::TDayPart& dayPart, const NAlice::TDateTime::EDayPart type)
    : Type{type}
    , TempMin{static_cast<double>(dayPart.GetTempMin().value())}
    , TempMax{static_cast<double>(dayPart.GetTempMax().value())}
    , TempAvg{static_cast<double>(dayPart.GetTempAvg().value())}
    , FeelsLike{static_cast<double>(dayPart.GetTemperatureFeels().value())}
    , Cloudness{UnpackDouble(dayPart.GetCloudiness().value())}
    , PrecStrength{UnpackDouble(dayPart.GetPrecStrength().value())}
    , PrecType{static_cast<size_t>(dayPart.GetPrecType().value())}
    , WindSpeed{static_cast<double>(dayPart.GetWindSpeed().value())}
    , WindGust{static_cast<double>(dayPart.GetWindGust().value())}
    , WindDir{dayPart.GetWindDir().value()}
    , PressureMM{dayPart.GetPressureMM().value()}
    , Icon{dayPart.GetIcon().value()}
    , Condition{dayPart.GetCondition().value()}
    , Source{dayPart.GetSource().value()} {
}

TString TDayPart::IconUrl(const size_t size, const TString& theme) const {
    return MakeIconUrl(Icon, size, theme);
}

TString TDayPart::BackgroundStyleClass() const {
    if (Type == NAlice::TDateTime::EDayPart::Morning || Type == NAlice::TDateTime::EDayPart::Day) {
        return CloudinessPrecCssStyle(Cloudness, PrecStrength) + "_day";
    }

    return CloudinessPrecCssStyle(Cloudness, PrecStrength) + "_night";
}

bool TDayPart::IsPrecipitation() const {
    return PrecStrength > 0.0;
}

TParts::TParts(THashMap<TString, TWeatherProtos::TDayPart> dayPartsMap)
    : Night{dayPartsMap["night"], NAlice::TDateTime::EDayPart::Night}
    , Morning{dayPartsMap["morning"], NAlice::TDateTime::EDayPart::Morning}
    , Day{dayPartsMap["day"], NAlice::TDateTime::EDayPart::Day}
    , Evening{dayPartsMap["evening"], NAlice::TDateTime::EDayPart::Evening}
    , DayShort{dayPartsMap["day_short"], NAlice::TDateTime::EDayPart::Day}
    , NightShort{dayPartsMap["night_short"], NAlice::TDateTime::EDayPart::Night}
{
    Map["night"] = &Night;
    Map["morning"] = &Morning;
    Map["day"] = &Day;
    Map["evening"] = &Evening;
}

THour::THour(const TWeatherProtos::TPreparedForecastHour& hour)
    : Hour{FromString<size_t>(hour.GetHour())}
    , HourTS{TInstant::Seconds(hour.GetHourTs())}
    , Temp{static_cast<double>(hour.GetTemperature())}
    , Icon{hour.GetIcon()}
    , PrecStrength{hour.GetPrecipitationsStrength()}
    , PrecType{static_cast<size_t>(hour.GetPrecipitationsType())}
    , WindSpeed{static_cast<double>(hour.GetWindSpeed())}
    , WindGust{static_cast<double>(hour.GetWindGust())}
    , WindDir{hour.GetWindDirection()}
    , PressureMM{hour.GetPressureMM()} {
}

TString THour::IconUrl(const size_t size, const TString& theme) const {
    return MakeIconUrl(Icon, size, theme);
}

bool THour::IsPrecipitation() const {
    return PrecStrength > 0.0;
}

TDay::TDay(const TWeatherProtos::TPreparedForecastItem& day, const NDatetime::TTimeZone& tz)
    : Date{NAlice::TDateTime::TSplitTime(tz, day.GetDateTs())}
    , Sunrise{day.GetSunrise().value()}
    , Sunset{day.GetSunset().value()}
    , Parts{ConstructDayPartsMap(day)}
{
    for (const auto& hour : day.GetHours()) {
        Hours.emplace_back(hour);
    }

    for (size_t i = 1; i < Hours.size(); ++i) {
        Hours.at(i - 1).Next = &Hours.at(i);
    }
}

TForecast::TForecast(const TWeatherContext& ctx)
    : PreparedForecast_{*ctx.Protos().PreparedForecast()}
    , Now{PreparedForecast_.GetNow()}
    , NowDt{PreparedForecast_.GetNowDt()}
    , Info{PreparedForecast_}
    , Yesterday{PreparedForecast_}
    , L10n{ctx.Logger(), ctx.Ctx().UserLang}
    , Fact{PreparedForecast_}
    , UserTime(NAlice::TDateTime::TSplitTime(NDatetime::GetTimeZone(Info.TzInfo.TimeZoneName()), Now))
{
    const auto& userTimeZone = NDatetime::GetTimeZone(Info.TzInfo.TimeZoneName());
    for (const auto& day : PreparedForecast_.GetForecasts()) {
        Days.emplace_back(day, userTimeZone);
    }

    for (size_t i = 1; i < Days.size(); ++i) {
        Days.at(i - 1).Next = &Days.at(i);
        if (!Days.at(i - 1).Hours.empty() && !Days.at(i).Hours.empty()) {
            Days.at(i - 1).Hours.at(Days.at(i - 1).Hours.size() - 1).Next = &Days.at(i).Hours.at(0);
        }
    }

    for (auto& day : Days) {
        day.Parts.Day.Day = &day;
        day.Parts.Night.Day = &day;
        day.Parts.Morning.Day = &day;
        day.Parts.Evening.Day = &day;
        day.Parts.DayShort.Day = &day;
        day.Parts.NightShort.Day = &day;

        for (auto& hour : day.Hours) {
            const NAlice::TDateTime::TSplitTime hourST {userTimeZone, 0, 0, 0, (ui32) hour.Hour};
            const auto dayPartEnum = NAlice::TDateTime::TimeToDayPart(hourST);

            switch (dayPartEnum) {
                case NAlice::TDateTime::EDayPart::Night:
                    hour.DayPart = &day.Parts.Night;
                    break;
                case NAlice::TDateTime::EDayPart::Morning:
                    hour.DayPart = &day.Parts.Morning;
                    break;
                case NAlice::TDateTime::EDayPart::Day:
                    hour.DayPart = &day.Parts.Day;
                    break;
                default:
                    hour.DayPart = &day.Parts.Evening;
                    break;
            }
        }
    }

    if (ctx.Protos().Warnings().Defined()) {
        LOG_INFO(ctx.Logger()) << "Taking warnings from response";

        auto warnings = ctx.Protos().Warnings()->GetWarnings();
        Warnings = TVector<TWeatherProtos::TWarning>(warnings.begin(), warnings.end());
    } else {
        LOG_ERROR(ctx.Logger()) << "Attention: TWarnings is empty!";
    }

    MockForecastFromExps(ctx, *this);
}

const TDay& TForecast::Today() const {
    return Days.at(0);
}

const TDay& TForecast::Tomorrow() const {
    return Days.at(1);
}

const TDay* TForecast::FindDay(const NAlice::TDateTime& dt) const {
    for (const auto& day : Days) {
        if (day.Date.ToString("%F") == dt.SplitTime().ToString("%F")) {
            return &day;
        }
    }

    return nullptr;
}

const TDayPart* TForecast::FindDayPart(const NAlice::TDateTime& dt) const {
    const auto* day = FindDay(dt);

    if (!day) {
        return nullptr;
    }

    switch (dt.DayPart()) {
        case NAlice::TDateTime::EDayPart::Morning:
            return &day->Parts.Morning;
        case NAlice::TDateTime::EDayPart::Day:
            return &day->Parts.Day;
        case NAlice::TDateTime::EDayPart::Evening:
            return &day->Parts.Evening;
        default:
            return &day->Parts.Night;
    }
}

const THour* TForecast::FindHour(const NAlice::TDateTime& dt) const {
    const auto* day = FindDay(dt);

    if (day && dt.SplitTime().Hour() >= 0 && static_cast<size_t>(dt.SplitTime().Hour()) < day->Hours.size()) {
        return &day->Hours[dt.SplitTime().Hour()];
    }

    return nullptr;
}

TNowcast::TNowcast(const TWeatherContext& ctx)
    : State{ctx.Protos().Alert()->GetState()}
    , Icon{ctx.Protos().Alert()->GetIcon()}
    , Condition{ctx.Protos().Alert()->GetCondition()}
    , Cloudness{static_cast<size_t>(ctx.Protos().Alert()->GetCloudiness())}
    , PrecType{static_cast<size_t>(ctx.Protos().Alert()->GetPrecType())}
    , PrecStrength{static_cast<size_t>(ctx.Protos().Alert()->GetPrecStrength())}
    , Time{static_cast<size_t>(ctx.Protos().Alert()->GetTime())}
    , Text{ctx.Protos().Alert()->GetText()}
    , Code{200}
{
    MockNowcastFromExps(ctx, *this);
}

} // NAlice::NHollywood::NWeather

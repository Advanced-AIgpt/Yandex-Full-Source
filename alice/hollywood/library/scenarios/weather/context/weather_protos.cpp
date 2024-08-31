#include "weather_protos.h"

namespace NAlice::NHollywood::NWeather {

namespace {

template<typename T>
TMaybe<T> ConstructAnswer(const NAppHost::IServiceContext& ctx, const TStringBuf type) {
    if (ctx.HasProtobufItem(type)) {
        return ctx.GetOnlyProtobufItem<T>(type);
    }
    return Nothing();
}

} // namespace

TWeatherProtos::TWeatherProtos(const NAppHost::IServiceContext& ctx)
    : AccumulatedPrecipitations_{ConstructAnswer<TAccumulatedPrecipitations>(ctx, "TAccumulatedPrecipitations")}
    , Alert_{ConstructAnswer<TAlert>(ctx, "TAlert")}
    , AlertError_{ConstructAnswer<TAlertError>(ctx, "TError")}
    , AlertRequest_{ConstructAnswer<TAlertRequest>(ctx, "TAlertRequest")}
    , DayParts_{ConstructAnswer<TDayParts>(ctx, "TDayParts")}
    , ForecastTimeline_{ConstructAnswer<TForecastTimeline>(ctx, "TForecastTimeline")}
    , YesterdayTimeline_{ConstructAnswer<TForecastTimeline>(ctx, "YESTERDAY_TMeteumTimeline")}
    , GeoLocation_{ConstructAnswer<TGeoLocation>(ctx, "TGeoLocation")}
    , GeoObject_{ConstructAnswer<TGeoObject>(ctx, "TGeoObject")}

    // Using Weather Warnings for TodayForecast scenario experiment — https://st.yandex-team.ru/WEATHER-18123
    , Warnings_{ConstructAnswer<TWarnings>(ctx, "TWarnings")}

    , MagneticFieldTimeline_{ConstructAnswer<TMagneticFieldTimeline>(ctx, "TMagneticFieldTimeline")}
    , NowcastTimeline_{ConstructAnswer<TNowcastTimeline>(ctx, "TNowcastTimeline")}
    , Options_{ConstructAnswer<TOptions>(ctx, "TOptions")}
    , PreparedForecast_{ConstructAnswer<TPreparedForecast>(ctx, "TForecast")}
    , Pressure_{ConstructAnswer<TPressure>(ctx, "TPressure")}
    , TimeSpan_{ConstructAnswer<TTimeSpan>(ctx, "TTimeSpan")}
    , TzInfo_{ConstructAnswer<TTzInfo>(ctx, "TTzInfo")}
{
}

const TMaybe<TWeatherProtos::TGeoObject>& TWeatherProtos::GeoObject() const {
    return GeoObject_;
}

const TMaybe<TWeatherProtos::TMagneticFieldTimeline>& TWeatherProtos::MagneticFieldTimeline() const {
    return MagneticFieldTimeline_;
}

const TMaybe<TWeatherProtos::TAccumulatedPrecipitations>& TWeatherProtos::AccumulatedPrecipitations() const {
    return AccumulatedPrecipitations_;
}

const TMaybe<TWeatherProtos::TPreparedForecast>& TWeatherProtos::PreparedForecast() const {
    return PreparedForecast_;
}

const TMaybe<TWeatherProtos::TPressure>& TWeatherProtos::Pressure() const {
    return Pressure_;
}

const TMaybe<TWeatherProtos::TOptions>& TWeatherProtos::Options() const {
    return Options_;
}

const TMaybe<TWeatherProtos::TAlert>& TWeatherProtos::Alert() const {
    return Alert_;
}

const TMaybe<TWeatherProtos::TAlertError>& TWeatherProtos::AlertError() const {
    return AlertError_;
}

const TMaybe<TWeatherProtos::TAlertRequest>& TWeatherProtos::AlertRequest() const {
    return AlertRequest_;
}

const TMaybe<TWeatherProtos::TWarnings>& TWeatherProtos::Warnings() const {
    return Warnings_;
}

const TMaybe<TWeatherProtos::TDayParts>& TWeatherProtos::DayParts() const {
    return DayParts_;
}

const TMaybe<TWeatherProtos::TForecastTimeline>& TWeatherProtos::ForecastTimeline() const {
    return ForecastTimeline_;
}

const TMaybe<TWeatherProtos::TForecastTimeline>& TWeatherProtos::YesterdayTimeline() const {
    return YesterdayTimeline_;
}

const TMaybe<TWeatherProtos::TGeoLocation>& TWeatherProtos::GeoLocation() const {
    return GeoLocation_;
}

const TMaybe<TWeatherProtos::TNowcastTimeline>& TWeatherProtos::NowcastTimeline() const {
    return NowcastTimeline_;
}

const TMaybe<TWeatherProtos::TTimeSpan>& TWeatherProtos::TimeSpan() const {
    return TimeSpan_;
}

const TMaybe<TWeatherProtos::TTzInfo>& TWeatherProtos::TzInfo() const {
    return TzInfo_;
}

} // namespace NAlice::NHollywood::NWeather

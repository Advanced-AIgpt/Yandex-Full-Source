#pragma once

#include <apphost/api/service/cpp/service_context.h>

#include <weather/app_host/accumulated_precipitations/protos/accumulated_precipitations.pb.h>
#include <weather/app_host/fact/protos/fact.pb.h>
#include <weather/app_host/forecast_postproc/protos/forecast.pb.h>
#include <weather/app_host/forecast_postproc/protos/options.pb.h>
#include <weather/app_host/geo_location/lib/geo_location.h>
#include <weather/app_host/geo_location/protos/geo_location.pb.h>
#include <weather/app_host/geo_location/protos/request.pb.h>
#include <weather/app_host/magnetic_field/protos/magnetic_field.pb.h>
#include <weather/app_host/meteum/protos/meteum.pb.h>
#include <weather/app_host/nowcast/protos/nowcast.pb.h>
#include <weather/app_host/protos/error.pb.h>
#include <weather/app_host/protos/timespan.pb.h>
#include <weather/app_host/v3_nowcast_alert_response/protos/v3_nowcast_alert_response.pb.h>
#include <weather/app_host/warnings/protos/warnings.pb.h>

namespace NAlice::NHollywood::NWeather {

class TWeatherProtos {
public:
    using ELanguage = ::NWeather::ELanguage;

    using TAccumulatedPrecipitations = ::NWeather::NAccumulatedPrecSource::TAccumulatedPrecipitations;
    using TAlert = ::NWeather::NV3NowcastAlertResponseSource::TV3NowcastAlertResponse;
    using TAlertError = ::NWeather::TError;
    using TAlertRequest = ::NWeather::NV3NowcastAlertResponseSource::TAlertRequest;

    // Using Weather Warnings for TodayForecast scenario experiment — https://st.yandex-team.ru/WEATHER-18123
    // Using Weather Warnings for TomorrowForecast scenario experiment — https://st.yandex-team.ru/WEATHER-18197
    // Using Weather Warnings mix in CurrentWeather scenario experiment — https://st.yandex-team.ru/WEATHER-18275
    using TWarningsRequest = ::NWeather::NWarningsSource::TWarningsRequest;
    using TWarnings = ::NWeather::NWarningsSource::TWarnings;
    using TWarning = ::NWeather::NWarningsSource::TWarning;
    using TCivilTime = ::NWeather::NWarningsSource::TCivilTime;
    using TPrecipitationContext = ::NWeather::NWarningsSource::TPrecipitationContext;

    using TDayPart = ::NWeather::NForecastPostProcessSource::TDayPart;
    using TDayParts = ::NWeather::NForecastPostProcessSource::TDayParts;
    using TForecast = ::NWeather::NMeteumSource::TForecast;
    using TForecastFact = ::NWeather::NFactSource::TFact;
    using TForecastTimeline = ::NWeather::NMeteumSource::TMeteumTimeline;
    using TGeoLocation = ::NWeather::NGeoLocationSource::TGeoLocation;
    using TGeoLocationRequest = ::NWeather::NGeoLocationSource::TGeoLocationRequest;
    using TGeoObject = ::NWeather::NGeoLocationSource::TGeoObject;
    using TMagneticFieldTimeline = ::NWeather::NMagneticFieldSource::TMagneticFieldTimeline;
    using TNowcastTimeline = ::NWeather::NNowcastSource::TNowcastTimeline;
    using TOptions = ::NWeather::NForecastPostProcessSource::TOptions;
    using TPressure = ::NWeather::NForecastPostProcessSource::TPressure;
    using TTimeSpan = ::NWeather::TTimeSpan;
    using TTzInfo = ::NWeather::NGeoLocationSource::TTzInfo;

    using TPreparedForecast = ::NWeather::NForecastPostProcessSource::TForecast;
    using TPreparedForecastItem = ::NWeather::NForecastPostProcessSource::TForecastItem;
    using TPreparedForecastHour = ::NWeather::NForecastPostProcessSource::TForecastHour;

public:
    TWeatherProtos(const NAppHost::IServiceContext& ctx);

    const TMaybe<TAccumulatedPrecipitations>& AccumulatedPrecipitations() const;
    const TMaybe<TAlert>& Alert() const;
    const TMaybe<TAlertError>& AlertError() const;
    const TMaybe<TAlertRequest>& AlertRequest() const;

    // For experiment https://st.yandex-team.ru/WEATHER-18123
    const TMaybe<TWarnings>& Warnings() const;

    const TMaybe<TDayParts>& DayParts() const;
    const TMaybe<TForecastTimeline>& ForecastTimeline() const;
    const TMaybe<TForecastTimeline>& YesterdayTimeline() const;
    const TMaybe<TGeoLocation>& GeoLocation() const;
    const TMaybe<TGeoObject>& GeoObject() const;
    const TMaybe<TMagneticFieldTimeline>& MagneticFieldTimeline() const;
    const TMaybe<TNowcastTimeline>& NowcastTimeline() const;
    const TMaybe<TOptions>& Options() const;
    const TMaybe<TPreparedForecast>& PreparedForecast() const;
    const TMaybe<TPressure>& Pressure() const;
    const TMaybe<TTzInfo>& TzInfo() const;
    const TMaybe<TWeatherProtos::TTimeSpan>& TimeSpan() const; // hack because overwrites other TTimeSpan

private:
    const TMaybe<TAccumulatedPrecipitations> AccumulatedPrecipitations_;
    const TMaybe<TAlert> Alert_;
    const TMaybe<TAlertError> AlertError_;
    const TMaybe<TAlertRequest> AlertRequest_;
    const TMaybe<TDayParts> DayParts_;
    const TMaybe<TForecastTimeline> ForecastTimeline_;
    const TMaybe<TForecastTimeline> YesterdayTimeline_;
    const TMaybe<TGeoLocation> GeoLocation_;
    const TMaybe<TGeoObject> GeoObject_;

    // For experiment https://st.yandex-team.ru/WEATHER-18123
    const TMaybe<TWarnings> Warnings_;

    const TMaybe<TMagneticFieldTimeline> MagneticFieldTimeline_;
    const TMaybe<TNowcastTimeline> NowcastTimeline_;
    const TMaybe<TOptions> Options_;
    const TMaybe<TPreparedForecast> PreparedForecast_;
    const TMaybe<TPressure> Pressure_;
    const TMaybe<TTimeSpan> TimeSpan_;
    const TMaybe<TTzInfo> TzInfo_;
};

const THashSet<TWeatherProtos::TWarning::ECode> TodayWarningsCodes = {
    TWeatherProtos::TWarning::TodayTemperatureIsLowerThanYesterday,
    TWeatherProtos::TWarning::TodayTemperatureIsTheSameAsYesterday,
    TWeatherProtos::TWarning::TodayTemperatureIsHigherThanYesterday,
    TWeatherProtos::TWarning::TodayTemperature,
    TWeatherProtos::TWarning::TodayWindAndGust,
    TWeatherProtos::TWarning::TodayPrecipitationWillBegin,
    TWeatherProtos::TWarning::TodayPrecipitationWillBeginAndEnd,
    TWeatherProtos::TWarning::TodayPrecipitationWillEnd,
    TWeatherProtos::TWarning::TodayPrecipitationWontEnd,
    TWeatherProtos::TWarning::TodayNoPrecipitation,
    TWeatherProtos::TWarning::TodayBlizzard,
};

const THashSet<TWeatherProtos::TWarning::ECode> TodayPrecipitationCodes = {
    TWeatherProtos::TWarning::TodayPrecipitationWillBegin,
    TWeatherProtos::TWarning::TodayPrecipitationWillBeginAndEnd,
    TWeatherProtos::TWarning::TodayPrecipitationWillEnd,
    TWeatherProtos::TWarning::TodayPrecipitationWontEnd,
    TWeatherProtos::TWarning::TodayNoPrecipitation,
};

} // namespace NAlice::NHollywood::NWeather

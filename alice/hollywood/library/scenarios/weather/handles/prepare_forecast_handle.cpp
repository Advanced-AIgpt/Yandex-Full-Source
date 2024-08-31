#include "prepare_forecast_handle.h"

#include <alice/hollywood/library/scenarios/weather/context/context.h>
#include <alice/hollywood/library/scenarios/weather/util/error.h>
#include <alice/hollywood/library/scenarios/weather/util/util.h>

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/weather/proto/weather.pb.h>

#include <alice/protos/data/language/language.pb.h>

#include <alice/library/geo_resolver/geo_response_parser.h>

#include <weather/app_host/geo_location/lib/geo_location.h>
#include <weather/app_host/geo_location/protos/geo_location.pb.h>
#include <weather/app_host/protos/timespan.pb.h>

#include <search/session/compression/report.h>
#include <search/idl/meta.pb.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NWeather {

void TWeatherPrepareForecastHandle::Do(TScenarioHandleContext& ctx) const {
    TWeatherContext weatherCtx{ctx};

    if (!weatherCtx.WeatherPlace()) {
        return; // TODO(sparkle): prohibit by graph?
    }

    const auto [lat, lon] = weatherCtx.GetLatLon();

    // Add TTimeSpan
    TWeatherProtos::TTimeSpan timespan;
    timespan.SetFrom(0);
    timespan.SetTo(TDuration::Days(10).Seconds());
    timespan.SetRelativeFromNow(true);
    timespan.SetIncludeWholeLocalDay(true);
    weatherCtx->ServiceCtx.AddProtobufItem(std::move(timespan), TStringBuf("TTimeSpan"));

    // Add TGeoLocationRequest
    const NGeobase::TId geoId = weatherCtx.WeatherPlace()->GetCityGeoId();
    TWeatherProtos::TGeoLocationRequest locationRequest;
    locationRequest.MutableLat()->set_value(lat);
    locationRequest.MutableLon()->set_value(lon);
    locationRequest.MutableGeoID()->set_value(geoId);
    locationRequest.MutableLang()->set_value("ru");
    weatherCtx->ServiceCtx.AddProtobufItem(std::move(locationRequest), TStringBuf("TGeoLocationRequest"));

    if (weatherCtx.RunRequest().HasExpFlag(NExperiment::WEATHER_CHANGE)) {
        // Add TOptions
        TWeatherProtos::TOptions options;
        options.SetIncludeLongtermForecasts(true);
        weatherCtx->ServiceCtx.AddProtobufItem(std::move(options), TStringBuf("TOptions"));
    }

    const bool disableRuOnlyPhrases =
        weatherCtx.RunRequest().BaseRequestProto().GetUserLanguage() != ELang::L_RUS ||
        weatherCtx.RunRequest().HasExpFlag(NExperiment::WEATHER_DISABLE_RU_ONLY_PHRASES);
    if (disableRuOnlyPhrases) {
        LOG_INFO(weatherCtx.Logger()) << "Ru-only phrases are disabled - skip sending TAlertRequest, TWarningsRequest";
        return;
    }

    // Add TAlertRequest + alert Flag
    ctx.ServiceCtx.AddFlag("alert");
    TWeatherProtos::TAlertRequest alertRequest;
    alertRequest.SetLang(TWeatherProtos::ELanguage::ru); // (ask sparkle@): change when other languages come in
    weatherCtx->ServiceCtx.AddProtobufItem(std::move(alertRequest), TStringBuf("TAlertRequest"));

    // Add TWarningsRequest + warnings Flag
    ctx.ServiceCtx.AddFlag("warnings");
    // Add TWarningsRequest
    TWeatherProtos::TWarningsRequest warningsRequest;
    // set WarningsRequest options
    warningsRequest.SetGroupingEnabled(true);
    warningsRequest.SetGroupingTopLimit(9999);
    warningsRequest.SetMessageFormat(TWeatherProtos::TWarningsRequest::alice);
    warningsRequest.SetLanguage(TWeatherProtos::ELanguage::ru);
    warningsRequest.SetKeepNonGrouped(weatherCtx.RunRequest().HasExpFlag(NExperiment::WEATHER_NOW_FORECAST_WARNING));
    if (weatherCtx.RunRequest().HasExpFlag(NExperiment::WEATHER_FOR_RANGE_FORECAST_WARNING)) {
        warningsRequest.SetAliceWeekendAndNextWeekGenerationRule(true);
    }

    weatherCtx->ServiceCtx.AddProtobufItem(std::move(warningsRequest), TStringBuf("TWarningsRequest"));
}

}  // namespace NAlice::NHollywood::NWeather

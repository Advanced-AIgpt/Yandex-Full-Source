#include "api.h"
#include "consts.h"
#include "suggests_maker.h"
#include "util.h"
#include "weather.h"

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/forms/geodb.h>
#include <alice/bass/forms/weather/day_hours_weather.h>
#include <alice/bass/forms/weather/current_weather.h>
#include <alice/bass/forms/weather/day_weather.h>
#include <alice/bass/forms/weather/today_weather.h>
#include <alice/bass/forms/weather/day_part_weather.h>
#include <alice/bass/forms/weather/days_range_weather.h>

#include <library/cpp/scheme/scheme.h>

#include <alice/bass/libs/fetcher/neh.h>

#include <util/string/builder.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS {
namespace NWeather {

namespace {

constexpr TStringBuf GET_WEATHER = "personal_assistant.scenarios.get_weather";
constexpr TStringBuf GET_WEATHER_ELLIPSIS = "personal_assistant.scenarios.get_weather__ellipsis";

NSc::TValue TryDeserializeJsonString(const NSc::TValue& maybeJsonString) {
    NSc::TValue deserializedJson;
    auto jsonString = maybeJsonString.GetString();
    if (!jsonString) {
        return maybeJsonString;
    }
    const bool success = NSc::TValue::FromJson(
        deserializedJson,
        jsonString
    );
    if (!success) {
        return maybeJsonString;
    }
    return deserializedJson;
}

class TWeatherRequestImpl {
public:
    TWeatherRequestImpl(TRequestHandler& r)
        : Ctx(r.Ctx())
    {
    }

    // starts from here!
    TResultValue Do() {
        // Sometimes value of "when" slot is serialized as string (e.g. MEGAMIND-1583)
        TContext::TSlot* whenSlot = Ctx.GetSlot("when");
        if (!IsSlotEmpty(whenSlot)) {
            whenSlot->Value = NWeather::TryDeserializeJsonString(whenSlot->Value);
        }

        // Get forecast beforehand, because datetimelist needs current user time
        auto forecastVariant = NWeather::FetchForecastFromWeatherApi(Ctx);
        if (auto err = std::get_if<TError>(&forecastVariant)) {
            Ctx.AddErrorBlock(*err);
            Ctx.AddSearchSuggest();
            return Nothing();
        }
        auto forecast = std::get<NWeather::TForecast>(forecastVariant);

        if (NWeather::IsTodayWeatherScenario(Ctx, forecast)) {
            const auto nowcastVariant = NWeather::FetchNowcastFromWeatherApi(Ctx);

            if (const auto err = std::get_if<TError>(&nowcastVariant)) {
                Ctx.AddErrorBlock(*err);
                Ctx.AddSearchSuggest();
                return Nothing();
                LOG(ERR) << "Unable to get nowcast data. Error: " << err->Msg << Endl;
            }

            const auto nowcastMaybe = std::get<NWeather::TNowcast>(nowcastVariant);

            LOG(INFO) << "PrepareTodayForecastsSlots" << Endl;
            return NWeather::PrepareTodayForecastsSlots(Ctx, forecast, nowcastMaybe);
        }

        if (NWeather::IsCurrentWeatherScenario(Ctx, forecast)) {
            LOG(INFO) << "PrepareCurrentForecastSlots" << Endl;
            return NWeather::PrepareCurrentForecastSlots(Ctx, forecast);
        }

        if (NWeather::IsDayHoursWeatherScenario(Ctx, forecast)) {
            LOG(INFO) << "PrepareDayHoursForecastSlots" << Endl;
            return NWeather::PrepareDayHoursForecastSlots(Ctx, forecast);
        }

        if (NWeather::IsDayPartScenario(Ctx, forecast)) {
            LOG(INFO) << "PrepareDayPartForecastSlots" << Endl;
            return NWeather::PrepareDayPartForecastSlots(Ctx, forecast);
        }

        if (NWeather::IsDayWeatherScenario(Ctx, forecast)) {
            LOG(INFO) << "PrepareDayForecastSlots" << Endl;
            return NWeather::PrepareDayForecastSlots(Ctx, forecast);
        }

        if (NWeather::IsDaysRangeWeatherScenario(Ctx, forecast)) {
            LOG(INFO) << "PrepareDaysRangeForecastSlots" << Endl;
            return NWeather::PrepareDaysRangeForecastSlots(Ctx, forecast);
        }

        Ctx.AddErrorBlock(TError(TError::EType::NOWEATHERFOUND), "Failed to match case for request");
        Ctx.AddSearchSuggest();
        Ctx.AddOnboardingSuggest();
        return Nothing();
    }

private:
    TContext& Ctx;
};

} // namespace

} // namespace NWeather

// static
TWeatherFormHandler::TErrorStatus TWeatherFormHandler::RequestWeather(TContext& ctx, const TRequestedGeo& requestedCity,
                                                                      NSc::TValue* weatherJson, TMaybe<NGeobase::TRegion>& respondedGeo,
                                                                      bool currentPosition, TMaybe<NSmallGeo::TLatLon> position)
{
    auto geoId = requestedCity.GetId();
    auto geoType = requestedCity.GetGeoType();
    if (!currentPosition &&
        geoType != NGeobase::ERegionType::CITY &&
        geoType != NGeobase::ERegionType::VILLAGE &&
        geoType != NGeobase::ERegionType::AIRPORT &&
        geoType != NGeobase::ERegionType::SETTLEMENT &&
        !position.Defined()
    ) {
        const auto& geobase = ctx.GlobalCtx().GeobaseLookup();
        auto city = requestedCity.GetParentIdByType(NGeobase::ERegionType::CITY);
        if (!NAlice::IsValidId(city)) {
            city = requestedCity.GetRegion().GetCapitalId();
        }
        if (!NAlice::IsValidId(city)) {
            auto region = requestedCity.GetParentIdByType(NGeobase::ERegionType::DISTRICT);
            city = geobase.GetCapitalId(region);
        }

        if (!NAlice::IsValidId(city)) {
            return TErrorWrapper(
                TError::EType::NOGEOFOUND, TStringBuf("Not supported GeoId by Weather: ") + ToString(geoId),
                false
            );
        }

        geoId = city;
        respondedGeo = geobase.GetRegionById(geoId);
    }

    TCgiParameters cgi;

    cgi.InsertUnescaped(TStringBuf("l10n"), "1");
    cgi.InsertUnescaped(TStringBuf("limit"), "1");  // get shorter document in response
    cgi.InsertUnescaped(TStringBuf("nogeo"), "1");  // don't request geo-coder (much more faster)
    cgi.InsertUnescaped(TStringBuf("lang"), ctx.MetaLocale().Lang);
    if (position) {
        cgi.InsertUnescaped(TStringBuf("lat"), ToString(position->Lat));
        cgi.InsertUnescaped(TStringBuf("lon"), ToString(position->Lon));
    } else if (currentPosition) {
        cgi.InsertUnescaped(TStringBuf("lat"), ToString(ctx.Meta().Location().Lat()));
        cgi.InsertUnescaped(TStringBuf("lon"), ToString(ctx.Meta().Location().Lon()));
    } else {
        cgi.InsertUnescaped(TStringBuf("geoid"), ToString(geoId));
    }

    NHttpFetcher::THandle::TRef h = ctx.GetSources().WeatherV3().Request()->AddCgiParams(cgi).Fetch();
    NHttpFetcher::TResponse::TRef resp = h->Wait();
    if (resp->IsError()) {
        return TErrorWrapper(TError::EType::SYSTEM, TStringBuilder() << TStringBuf("Fetching from weather error: ") << resp->GetErrorText());
    }

    NSc::TValue responseJson;
    try {
        responseJson = NSc::TValue::FromJsonThrow(resp.Get()->Data);
    } catch (const NSc::TSchemeParseException& e) {
        return TErrorWrapper(TError::EType::SYSTEM,
                             TStringBuilder() << "Weather API parse error: "
                                              << e.Offset << ", "
                                              << e.Reason << ": "
                                              << resp->Data);
    }

    if (responseJson.IsNull()) {
        return TErrorWrapper(TError::EType::SYSTEM, "no data found in weather document");
    }

    *weatherJson = std::move(responseJson);

    return Nothing();
}

TResultValue TWeatherFormHandler::Do(TRequestHandler& r) {
    NWeather::SetWeatherProductScenario(r.Ctx());

    NWeather::TryParseJsonFromAllSlots(r.Ctx());

    NWeather::TWeatherRequestImpl impl(r);
    auto error = impl.Do();

    if (error) {
        auto& Ctx = r.Ctx();
        Ctx.AddErrorBlock(*error);
        Ctx.AddSuggest(TStringBuf("forecast_today"));
        Ctx.AddSuggest(TStringBuf("forecast_tomorrow"));
        Ctx.AddSearchSuggest();
    }

    return TResultValue();
}

// static
void TWeatherFormHandler::Register(THandlersMap* handlers) {
    auto cbWeatherForm = []() {
        return MakeHolder<TWeatherFormHandler>();
    };
    handlers->emplace(NWeather::GET_WEATHER, cbWeatherForm);
    handlers->emplace(NWeather::GET_WEATHER_ELLIPSIS, cbWeatherForm);
}

} // namespace NBASS

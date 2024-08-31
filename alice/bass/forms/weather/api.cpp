#include "api.h"
#include "util.h"

namespace NBASS::NWeather {

namespace {

constexpr TStringBuf ICONS_BASE_URL = "https://yastatic.net/weather/i/icons/portal/png";

std::variant<NSc::TValue, TError> FetchForecastResponseFromWeatherApi(TContext& ctx) {
    auto latLon = GetLatLon(ctx);
    if (auto err = std::get_if<TError>(&latLon)) {
        return *err;
    }

    auto [lat, lon] = std::get<TLatLon>(latLon);

    TCgiParameters cgi;
    cgi.InsertUnescaped("lat", lat);
    cgi.InsertUnescaped("lon", lon);
    cgi.InsertUnescaped("l10n", "1");
    cgi.InsertUnescaped("nogeo", "1"); // don't request geo-coder
    cgi.InsertUnescaped("lang", ctx.MetaLocale().Lang);
    // cgi.InsertUnescaped("limit", "2"); // two days max in response (2 is minimum)
    cgi.InsertUnescaped("extra", "1"); // for prec_strength in fact
    cgi.InsertUnescaped("_from", "alice_nowcast");

    NHttpFetcher::THandle::TRef weatherRequest =
        ctx.GetSources().WeatherV3().Request()->AddCgiParams(cgi).Fetch();

    NHttpFetcher::TResponse::TRef forecastRespHandle = weatherRequest->Wait();
    if (forecastRespHandle->IsError()) {
        return TError(TError::EType::WEATHERERROR,
                      TStringBuilder() << "Failed to get Weather from API: " << forecastRespHandle->GetErrorText());
    }

    NSc::TValue forecast;

    if (!NSc::TValue::FromJson(forecast, forecastRespHandle->Data)) {
        return TError(TError::EType::WEATHERERROR, "Failed to parse Weather from API");
    }

    return forecast;
}

std::variant<NSc::TValue, TError> FetchNowcastResponseFromWeatherApi(TContext& ctx) {
    auto latLon = GetLatLon(ctx);
    if (auto err = std::get_if<TError>(&latLon)) {
        return *err;
    }

    auto [lat, lon] = std::get<TLatLon>(latLon);

    TCgiParameters cgi;
    cgi.InsertUnescaped("lat", lat);
    cgi.InsertUnescaped("lon", lon);
    cgi.InsertUnescaped("nogeo", "1"); // don't request geo-coder (much more faster)
    cgi.InsertUnescaped("lang", ctx.MetaLocale().Lang);
    cgi.InsertUnescaped("limit", "2"); // two days max in response (2 is minimum)
    cgi.InsertUnescaped("extra", "1"); // for prec_strength in fact
    cgi.InsertUnescaped("_from", "alice_nowcast");

    NHttpFetcher::THandle::TRef nowCastRequest = ctx.GetSources()
                                                     .WeatherNowcastV3()
                                                     .Request()
                                                     ->AddCgiParams(cgi)
                                                     .Fetch();

    NHttpFetcher::TResponse::TRef nowCastRespHandle = nowCastRequest->Wait();

    if (nowCastRespHandle->IsError() && nowCastRespHandle->Code != HttpCodes::HTTP_NOT_FOUND) {
        return TError(TError::EType::WEATHERERROR, TStringBuilder() << "Failed to get Weather nowcast from API: "
                                                                    << nowCastRespHandle->GetErrorText());
    }

    NSc::TValue nowcast;

    if (nowCastRespHandle->Code != HttpCodes::HTTP_NOT_FOUND &&
        !NSc::TValue::FromJson(nowcast, nowCastRespHandle->Data)) {
        return TError(TError::EType::WEATHERERROR, "Failed to parse Weather nowcast from API");
    }

    nowcast["Сode"] = nowCastRespHandle->Code;

    return nowcast;
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

std::variant<TForecast, TError> FetchForecastFromWeatherApi(TContext& ctx) {
    const auto& forecastJsonVariant = FetchForecastResponseFromWeatherApi(ctx);

    if (auto err = std::get_if<TError>(&forecastJsonVariant)) {
        return *err;
    }

    return TForecast(std::get<NSc::TValue>(forecastJsonVariant));
}

std::variant<TNowcast, TError> FetchNowcastFromWeatherApi(TContext& ctx) {
    const auto& nowcastJsonVariant = FetchNowcastResponseFromWeatherApi(ctx);

    if (auto err = std::get_if<TError>(&nowcastJsonVariant)) {
        return *err;
    }

    return TNowcast(std::get<NSc::TValue>(nowcastJsonVariant));
}

} // namespace NBASS::NWeather

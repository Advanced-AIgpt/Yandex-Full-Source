#include "nowcast_util.h"

#include <library/cpp/uri/encode.h>
#include <util/string/printf.h>

namespace NAlice::NHollywood::NWeather {

constexpr TStringBuf NOWCAST_STATIC_API_KEY =
    "AHLd7lsBAAAAendDRwMAXelTRbzn0WRLW45SVz9okkx-2hAAAAAAAAAAAADoTdjtWJt75cH2YqCAvYw8MeZ83g==";

TString MakeTileURL(int64_t genTime, int64_t forDate) {
    TCgiParameters queryParams;
    queryParams.InsertUnescaped("transparent", "1");
    queryParams.InsertUnescaped("nowcast_gen_time", ToString(genTime));
    queryParams.InsertUnescaped("for_date", ToString(forDate));
    queryParams.InsertUnescaped("scale", "2.0");
    return TStringBuilder() << "http://ah.weather.yandex.net/api/v3/nowcast/tile?" << queryParams.Print()
                            << "&x=%x&y=%y&z=%z";
}

TString MakeStaticMapURL(float lat, float lon, int64_t genTime, int64_t forDate, int64_t zoom) {
    TCgiParameters queryParams;
    queryParams.InsertUnescaped("cr", "0");
    queryParams.InsertUnescaped("lg", "0");
    TString latLon = TStringBuilder() << lon << ',' << lat;
    queryParams.InsertUnescaped("ll", latLon);
    queryParams.InsertUnescaped("pt", TStringBuilder() << latLon << ",placemark");
    queryParams.InsertUnescaped("key", NOWCAST_STATIC_API_KEY);
    TStringStream tileEncoded;
    {
        TString tileURL = MakeTileURL(genTime, forDate);
        NUri::TEncoder::EncodeNotAlnum(tileEncoded, tileURL);
    }
    queryParams.InsertUnescaped("l", TStringBuilder() << "map," << tileEncoded.Str());
    queryParams.InsertUnescaped("size", "960,480");
    queryParams.InsertUnescaped("scale", "2.0");
    queryParams.InsertUnescaped("z", ToString(zoom));

    return TStringBuilder() << "https://static-maps.yandex.ru/1.x?" << queryParams.Print();
}

int64_t GetZoom(int64_t startTS, int64_t endTS) {
    auto diff = (endTS - startTS) / 60;
    if (diff <= 30) {
        return 10;
    }
    if (diff <= 60) {
        return 9;
    }
    if (diff <= 90) {
        return 8;
    }
    return 7;
}

NJson::TJsonValue MakeDivCard(const TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();
    const auto& alert = ctx.Protos().Alert();
    int64_t genTime = 0, forDate = 0, zoom = 10;
    if (alert) {
        genTime = alert->GetGenTime();
        forDate = alert->GetTime();
        zoom = GetZoom(TInstant::Now().Seconds(), forDate);
    }
    auto userTime = forecast.UserTime;
    const auto [lat, lon] = ctx.GetLatLon();

    NJson::TJsonValue card;
    auto& current = card["current"].SetType(NJson::JSON_MAP);

    const bool isPrec = forecast.Fact.PrecStrength > 0.0;
    current["precipitation"] = isPrec;

    auto staticMapURL = MakeStaticMapURL(lat, lon, genTime, forDate, zoom);
    current["background_url"] = staticMapURL;
    current["prec_type_url"].SetType(NJson::JSON_NULL);

    auto& hours = card["hours"].SetValue(NJson::JSON_ARRAY).GetArraySafe();

    int currentHour = -1;
    int userHour = userTime.Hour();

    for (int dayIdx : {0, 1}) {
        const TDay& day = forecast.Days.at(dayIdx);
        for (const auto& hour : day.Hours) {
            ++currentHour;

            auto size = hours.size();
            if (size == 24) {
                // all hours collected
                break;
            }
            if (size == 0 && currentHour < userHour) {
                // skip past hours
                continue;
            }

            NJson::TJsonValue hourData;
            if (size == 0) {
                // for first hour we use precipitation from fact
                // and current minutes value in time
                hourData["local_day_time"] = Sprintf("%02d:%02d", static_cast<int>(hour.Hour), userTime.Min());
                hourData["precipitation"] = static_cast<int>(isPrec);
                const TAvatar* avatar = ctx.AvatarsMap().Find(forecast.Fact.Icon);
                hourData["icon"] = avatar->Https;
            } else {
                hourData["local_day_time"] = Sprintf("%02d:%02d", static_cast<int>(hour.Hour), 0);
                hourData["precipitation"] = static_cast<int>(hour.PrecStrength > 0.0);
                const TAvatar* avatar = ctx.AvatarsMap().Find(hour.Icon);
                hourData["icon"] = avatar->Https;
            }

            hours.push_back(hourData);
        }
    }

    return card;
}

void AddNowcastDivCardBlock(TWeatherContext& ctx, NJson::TJsonValue& divCard) {
    const auto& nowcast = *ctx.Nowcast();

    // patch current precipitation
    NJson::TJsonValue& divCardFact = divCard["current"].SetType(NJson::JSON_MAP);
    divCardFact["prec_type_url"].SetType(NJson::JSON_NULL);
    divCardFact["precipitation"] = static_cast<int>(nowcast.PrecStrength > 0.0);
    ctx.Renderer().AddDivCard(NNlgTemplateNames::GET_WEATHER_NOWCAST, "weather__precipitation__nowcast", divCard);
}

void InsertCommonNowcastUtms(TCgiParameters& params) {
    params.InsertUnescaped("utm_source", "alice");
    params.InsertUnescaped("utm_campaign", "card");
    params.InsertUnescaped("utm_medium", "nowcast");
    params.InsertUnescaped("utm_content", "fullscreen");
}

TString ConstructNowcastUrl(const TWeatherContext& ctx, TString startUrl, bool turboInsertEnable) {
    const auto [lat, lon] = ctx.GetLatLon();

    TCgiParameters frontendCgi;
    frontendCgi.InsertUnescaped("lat", ToString(lat));
    frontendCgi.InsertUnescaped("lon", ToString(lon));
    frontendCgi.InsertUnescaped("from", "alice_raincard");

    if (!ctx.RunRequest().ClientInfo().IsDesktop()) {
        frontendCgi.InsertUnescaped("appsearch_header", "1");
    }

    if (turboInsertEnable && IsTurbo(ctx)) {
        TCgiParameters turboCgi;
        turboCgi.InsertEscaped("text", startUrl + frontendCgi.Print());
        InsertCommonNowcastUtms(turboCgi);

        return TString("https://yandex.ru/turbo?") + turboCgi.Print();
    }
    InsertCommonNowcastUtms(frontendCgi);

    return startUrl + frontendCgi.Print();
}

TString MakeNowcastNowUrl(const TWeatherContext& ctx) {
    TStringBuilder url;
    url << YANDEX_POGODA_URL << "/maps/nowcast?";
    return ConstructNowcastUrl(ctx, static_cast<TString>(url), false);
}

void SuggestNowcastNowUrlSlot(TWeatherContext& ctx) {
    PrepareWeatherOpenUriSlot(ctx, MakeNowcastNowUrl(ctx));
}

TString MakeHomeUrl(const TWeatherContext& ctx) {
    TStringBuilder url;
    url << YANDEX_POGODA_URL << "?";
    return ConstructNowcastUrl(ctx, static_cast<TString>(url));
}

void SuggestHomeUrlSlot(TWeatherContext& ctx) {
    PrepareWeatherOpenUriSlot(ctx, MakeHomeUrl(ctx));
}

bool IsTurbo(const TWeatherContext& ctx) {
    auto clientInfo = ctx.RunRequest().ClientInfo();
    return !clientInfo.IsDesktop() && clientInfo.IsAliceKit();
}

void PrepareWeatherOpenUriSlot(TWeatherContext& ctx, const TString& uri) {
    if (!ctx.CanRenderDivCards()) {
        ctx.Renderer().AddSuggests({ESuggestType::OpenUri});
    }

    auto uriSlot = ctx.FindOrAddSlot("uri", "string");
    const_cast<TSlot*>(uriSlot.Get())->Value = TSlot::TValue{uri};
}

} // namespace NAlice::NHollywood::NWeather

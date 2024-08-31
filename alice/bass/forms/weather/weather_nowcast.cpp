#include "api.h"
#include "util.h"
#include "weather_nowcast.h"

#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/forms/geodb.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/smallgeo/region.h>
#include <alice/bass/forms/weather/util.h>

#include <alice/library/datetime/datetime.h>

using NAlice::TDateTime;
using NAlice::TDateTimeList;

namespace NBASS {
namespace {

constexpr TStringBuf GET_WEATHER_NOWCAST = "personal_assistant.scenarios.get_weather_nowcast";
constexpr TStringBuf GET_WEATHER_NOWCAST_ELLIPSIS = "personal_assistant.scenarios.get_weather_nowcast__ellipsis";

using namespace NWeather;

using TLatLon = std::pair<TString, TString>;

constexpr unsigned int RANDOM_CAP = 100;

bool IsPrecipitation(const NSc::TValue& node) {
    const auto& strength = node["prec_strength"];
    return !strength.IsNull() && strength > 0.0;
}

bool IsPrecipitationAlert(const NSc::TValue& node) {
    const auto& strength = node["PrecStrength"];
    return !strength.IsNull() && strength.ForceIntNumber() > 0;
}

TStringBuf GetForegroundType(i64 precType, double precStrength) {
    // rain
    if (precType == 1) {
        if (precStrength <= 0.25)
            return "rain_low";
        if (precStrength >= 0.75)
            return "rain_hvy";

        return "rain_avg";
    }

    // snow
    if (precType == 3) {
        if (precStrength <= 0.25)
            return "snow_low";
        if (precStrength >= 0.75)
            return "snow_hvy";

        return "snow_avg";
    }

    // sleet
    if (precType == 2)
        return "rain_snow";

    return "hail";
}

TStringBuilder GetPrecipitaionTypeImageURL(const NSc::TValue& fact) {
    TStringBuilder precTypeUrl;
    precTypeUrl << "https://yastatic.net/weather/i/pp/prec/1_5x/"
                << GetForegroundType(fact["prec_type"].GetIntNumber(), fact["prec_strength"].GetNumber())
                << ".png";
    return precTypeUrl;
}

TStringBuilder GetPrecipitaionTypeImageURLForAlert(const NSc::TValue& alert) {
    TStringBuilder precTypeUrl;
    precTypeUrl << "https://yastatic.net/weather/i/pp/prec/1_5x/"
                << GetForegroundType(alert["PrecType"].GetIntNumber(), alert["PrecStrength"].GetIntNumber() / 4.)
                << ".png";
    return precTypeUrl;
}

std::variant<std::unique_ptr<TDateTimeList>, TError> GetDateTimeList(TContext &ctx, const TDateTime::TSplitTime &userTime) {
    const TSlot* dayPart = ctx.GetSlot("day_part");
    const TSlot* when = ctx.GetSlot("when");

    try {
        return TDateTimeList::CreateFromSlot(when, dayPart, TDateTime(userTime), {2 /* MAX_WEATHER_DAYS */, true});
    } catch (const yexception& e) {
        return TError(TError::EType::INVALIDPARAM, e.what());
    }
}

TDateTime::TSplitTime GetUserTime(const NSc::TValue& forecast) {
    auto now = forecast["now"].GetIntNumber(); // timestamp from api
    auto info = TInfo(forecast["info"]);
    auto tz = NDatetime::GetTimeZone(info.TzInfo.TimeZoneName());

    return TDateTime::TSplitTime(tz, now);
}

NSc::TValue MakeDivCard(const NSc::TValue& weatherJson, const TDateTime::TSplitTime& userTime, TContext& ctx, const TLatLon& latLon) {
    NSc::TValue card;

    auto& current = card["current"];

    const NSc::TValue& fact = weatherJson["fact"];

    const bool isPrec = IsPrecipitation(fact);
    current["precipitation"] = isPrec;

    {
        TCgiParameters staticMapParameters;
        staticMapParameters.InsertUnescaped("l", "map");
        staticMapParameters.InsertUnescaped("z", "17");
        staticMapParameters.InsertUnescaped("size", "1560,480");

        // Key in Get parameters for static maps :(
        staticMapParameters.InsertUnescaped("key", ctx.GetConfig().Vins().WeatherNowcast().StaticApiKey());

        staticMapParameters.InsertUnescaped("ll", TStringBuilder() << latLon.second << "," << latLon.first);

        current["background_url"] = TString("https://static-maps.yandex.ru/1.x/?") + staticMapParameters.Print();

        TStringBuilder precTypeUrl = GetPrecipitaionTypeImageURL(fact);
        if (isPrec)
            current["prec_type_url"] = precTypeUrl;
        else
            current["prec_type_url"].SetNull();
    }

    auto& hours = card["hours"].GetArrayMutable();

    int currentHour = -1;
    int userHour = userTime.Hour();

    for (int dayIdx : {0, 1}) {
        const NSc::TValue& day = weatherJson["forecasts"][dayIdx];
        for (const auto& hour : day["hours"].GetArray()) {
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

            NSc::TValue hourData;
            if (size == 0) {
                // for first hour we use precipitation from fact
                // and current minutes value in time
                hourData["local_day_time"] = Sprintf(
                        "%02d:%02d",
                        FromString<int>(hour["hour"].GetString()),
                        userTime.Min());
                hourData["precipitation"] = static_cast<int>(isPrec);
                const TAvatar* avatar = ctx.Avatar(TStringBuf("weather"), fact["icon"].GetString());
                hourData["icon"] = avatar->Https;
            } else {
                hourData["local_day_time"] = Sprintf("%02d:%02d", FromString<int>(hour["hour"].GetString()), 0);
                hourData["precipitation"] = static_cast<int>(IsPrecipitation(hour));
                const TAvatar* avatar = ctx.Avatar(TStringBuf("weather"), hour["icon"].GetString());
                hourData["icon"] = avatar->Https;
            }

            hours.push_back(hourData);
        }
    }

    return card;
}

std::variant<NSc::TValue, TError> MakeDivCard(TContext& ctx, const NSc::TValue& forecast) {
    auto userTime = GetUserTime(forecast);
    auto latLon = GetLatLon(ctx);
    if (auto err = std::get_if<TError>(&latLon)) {
        return *err;
    }

    return MakeDivCard(forecast, userTime, ctx, std::get<TLatLon>(latLon));
}

TResultValue RedirectToWeather(TContext& ctx) {
    LOG(INFO) << "RedirectToWeather" << Endl;
    TContext::TPtr newCtx = ctx.SetResponseForm("personal_assistant.scenarios.get_weather", false);
    Y_ENSURE(newCtx);
    newCtx->CopySlotsFrom(ctx, {"where", "when", "day_part"});
    return ctx.RunResponseFormHandler();
}

void PrepareWeatherOpenUriSlot(TContext &ctx, const TString &uri) {
    if (ctx.ClientFeatures().SupportsDivCards()) {
        ctx.AddTextCardBlock(TStringBuf("weather__precipitation__text"));
    } else {
        ctx.AddSuggest(TStringBuf("weather__open_uri"));
    }

    TSlot* uriSlot = ctx.GetOrCreateSlot("uri", "string");
    TStringBuilder newUri;
    newUri << uri << "&utm_source=alice&utm_campaign=card";
    uriSlot->Value.SetString(newUri);
}

std::variant<NSc::TValue, TError> FetchForecastFromWeatherApi(TContext& ctx) {
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

    NHttpFetcher::THandle::TRef weatherRequest = ctx.GetSources().WeatherV3().Request()->AddCgiParams(cgi).Fetch();

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

std::variant<NSc::TValue, TError> FetchNowcastFromWeatherApi(TContext& ctx) {
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

    NHttpFetcher::THandle::TRef nowCastRequest = ctx.GetSources().WeatherNowcastV3().Request()->AddCgiParams(cgi).Fetch();

    NHttpFetcher::TResponse::TRef nowCastRespHandle = nowCastRequest->Wait();

    if (nowCastRespHandle->IsError() && nowCastRespHandle->Code != HttpCodes::HTTP_NOT_FOUND) {
        return TError(TError::EType::WEATHERERROR, "Error from weather nowcast API");
    }

    NSc::TValue nowcast;

    if (nowCastRespHandle->Code != HttpCodes::HTTP_NOT_FOUND && !NSc::TValue::FromJson(nowcast, nowCastRespHandle->Data)) {
        return TError(TError::EType::WEATHERERROR, "Failed to parse Nowcast from API");
    }

    nowcast["Code"] = nowCastRespHandle->Code;

    return nowcast;
}

bool IsTurbo(TContext& ctx) {
    auto clientInfo = ctx.MetaClientInfo();
    return !clientInfo.IsDesktop() && clientInfo.IsAliceKit();
}

TResultValue SuggestHomeUrlSlot(TContext &ctx) {
    auto latLon = GetLatLon(ctx);
    if (auto err = std::get_if<TError>(&latLon)) {
        return *err;
    }

    auto [lat, lon] = std::get<TLatLon>(latLon);

    TCgiParameters frontendCgi, turboCgi;
    frontendCgi.InsertUnescaped("lat", lat);
    frontendCgi.InsertUnescaped("lon", lon);
    frontendCgi.InsertUnescaped("from", "alice_raincard");

    if (IsTurbo(ctx)) {
        frontendCgi.InsertUnescaped("appsearch_header", "1");
        turboCgi.InsertEscaped(
                "text",
                TString("https://yandex.ru/pogoda?") + frontendCgi.Print());

        PrepareWeatherOpenUriSlot(ctx, TString("https://yandex.ru/turbo?") + turboCgi.Print());
    } else {
        PrepareWeatherOpenUriSlot(ctx, TString("https://yandex.ru/pogoda?") + frontendCgi.Print());
    }

    return Nothing();
}

TResultValue SuggestDetailsUrlSlot(TContext &ctx) {
    auto latLon = GetLatLon(ctx);
    if (auto err = std::get_if<TError>(&latLon)) {
        return *err;
    }

    auto [lat, lon] = std::get<TLatLon>(latLon);

    TCgiParameters frontendCgi, turboCgi;
    frontendCgi.InsertUnescaped("lat", lat);
    frontendCgi.InsertUnescaped("lon", lon);
    frontendCgi.InsertUnescaped("from", "alice_raincard");

    if (IsTurbo(ctx)) {
        frontendCgi.InsertUnescaped("appsearch_header", "1");
        turboCgi.InsertEscaped(
                "text",
                TString("https://yandex.ru/pogoda/details/today?") + frontendCgi.Print());

        PrepareWeatherOpenUriSlot(ctx, TString("https://yandex.ru/turbo?") + turboCgi.Print());
    } else {
        PrepareWeatherOpenUriSlot(ctx, TString("https://yandex.ru/pogoda/details/today?") + frontendCgi.Print());
    }

    return Nothing();
}

std::variant<bool, TError> IsGetWeatherScenario(TContext& ctx, const NSc::TValue& forecast) {
    const TSlot* where = ctx.GetSlot("where");
    if (!IsSlotEmpty(where)) {
        auto city = GetCity(ctx);
        if (auto err = std::get_if<TError>(&city)) {
            return *err;
        }

        auto region = std::get<TRequestedGeo>(city).GetRegion();
        if (region.GetEType() < NGeobase::ERegionType::CITY) {
            return true;
        }
    }

    auto userTime = GetUserTime(forecast);
    auto dtl = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TError>(&dtl)) {
        return *err;
    }

    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtl);

    auto userDateString = userTime.ToString("%F");
    if (!dateTimeList->IsNow() && dateTimeList->cbegin()->SplitTime().ToString("%F") != userDateString) { // not now and not today
        return true;
    }

    if (dateTimeList->TotalDays() > 1) { // date range
        return true;
    }

    return false;
}

bool HasNowcast(const NSc::TValue& nowcast) {
    return nowcast["Code"].GetIntNumber() == HttpCodes::HTTP_OK;
}

std::variant<bool, TError> IsNowcastForNowCase(TContext& ctx, const NSc::TValue& forecast, const NSc::TValue& nowcast) {
    if (!HasNowcast(nowcast)) {
        return false;
    }

    auto userTime = GetUserTime(forecast);
    auto dtl = GetDateTimeList(ctx, userTime);
    if (auto err = std::get_if<TError>(&dtl)) {
        return *err;
    }

    auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtl);
    if (!dateTimeList->IsNow()) {
        return false;
    }

    const TStringBuf state = nowcast["State"].GetString();
    return state != "nodata" && state != "noprec" && state != "still" && state != "norule";
}

void AddNowcastDivCardBlock(TContext& ctx, NSc::TValue& divCard, const NSc::TValue& nowcast) {
    // patch current precipitation
    TStringBuilder precTypeUrl = GetPrecipitaionTypeImageURLForAlert(nowcast);
    NSc::TValue& divCardFact = divCard["current"];
    divCardFact["prec_type_url"] = precTypeUrl;
    divCardFact["precipitation"] = IsPrecipitationAlert(nowcast);

    ctx.AddDivCardBlock(TStringBuf("weather__precipitation__nowcast"), divCard);
}

TResultValue SuggestNowcastNowUrlSlot(TContext& ctx) {
    auto latLon = GetLatLon(ctx);
    if (auto err = std::get_if<TError>(&latLon)) {
        return *err;
    }

    auto [lat, lon] = std::get<TLatLon>(latLon);

    TCgiParameters frontendCgi;
    frontendCgi.InsertUnescaped("lat", lat);
    frontendCgi.InsertUnescaped("lon", lon);
    frontendCgi.InsertUnescaped("from", "alice_raincard");

    if (!ctx.MetaClientInfo().IsDesktop()) {
        frontendCgi.InsertUnescaped("appsearch_header", "1");
    }

    PrepareWeatherOpenUriSlot(ctx, TString("https://yandex.ru/pogoda/maps/nowcast?") + frontendCgi.Print());

    return Nothing();
}

TResultValue PrepareNowcastForNow(TContext& ctx, const NSc::TValue& forecast, const NSc::TValue& nowcast) {
    if (ctx.HasExpFlag("weather_precipitation_starts_ends")) {
        const NSc::TValue &fact = forecast["fact"];
        auto isCurrentPrecipitation = IsPrecipitation(fact);
        ctx.GetOrCreateSlot("precipitation_current", "num")->Value.SetIntNumber(isCurrentPrecipitation);
    }

    TSlot* nowCastSlot = ctx.GetOrCreateSlot("weather_nowcast_alert", "string");
    nowCastSlot->Value = nowcast["Text"];

    TSlot* precTypeSlot = ctx.GetOrCreateSlot("precipitation_type", "num");
    precTypeSlot->Value.SetIntNumber(forecast["fact"]["prec_type"].GetIntNumber());

    if (ctx.ClientFeatures().SupportsDivCards()) {
        auto card = MakeDivCard(ctx, forecast);
        if (auto err = std::get_if<TError>(&card)) {
            return *err;
        }

        NSc::TValue divCard = std::get<NSc::TValue>(card);
        AddNowcastDivCardBlock(ctx, divCard, nowcast);
    }

    return SuggestNowcastNowUrlSlot(ctx);
}

bool IsDayPartForecastCase(TContext& ctx) {
    return !IsSlotEmpty(ctx.GetSlot("day_part"));
}

TResultValue PrepareDayPartForecastSlot(TContext& ctx, const NSc::TValue& forecast, const NSc::TValue& nowcast) {
    auto userTime = GetUserTime(forecast);
    auto nextUserDay = TDateTime::TSplitTime(userTime.TimeZone(), userTime.AsTimeT() + TDuration::Days(1).Seconds());

    auto userDateString = userTime.ToString("%F");
    auto userNextDayString = nextUserDay.ToString("%F");
    int userHour = userTime.Hour();

    const NSc::TValue& fact = forecast["fact"];
    TSlot* precForDayPartSlot = ctx.GetOrCreateSlot("precipitation_for_day_part", "num");
    TSlot* precTypeSlot = ctx.GetOrCreateSlot("precipitation_type", "num");
    precTypeSlot->Value.SetIntNumber(fact["prec_type"].GetIntNumber());

    auto dayPart = ctx.GetSlot("day_part");
    auto pt = dayPart->Value.GetString();
    const auto& days = forecast["forecasts"].GetArray();
    bool showDetails = false;
    for (const auto& day : days) {
        // today and tomorrow
        const auto& dateString = day["date"];
        const auto& parts = day["parts"];

        if (dateString != userDateString && dateString != userNextDayString) {
            continue;
        }

        if (!parts.Has(pt)) {
            continue;
        }

        const auto& part = parts[pt];

        // check if day part is in past
        // this is not really necessary because of DateTimeList logic:
        //   past day part interpreted as tomorrow
        // check anyway for stability reasons
        TStringBuf left, right;
        part["_source"].GetString().RSplit(",", left, right);
        int maxHourInPart;
        bool success = TryFromString(right, maxHourInPart);

        if (success && maxHourInPart >= userHour) { // requested day part not in past
            bool hasPrec = part["prec_strength"].GetNumber() > 0.0;
            precForDayPartSlot->Value.SetIntNumber(hasPrec);

            if (hasPrec) {
                precTypeSlot->Value.SetIntNumber(part["prec_type"].GetIntNumber());
            }

            showDetails = true;
            break;
        }

        return RedirectToWeather(ctx);
    }

    if (ctx.ClientFeatures().SupportsDivCards()) {
        auto card = MakeDivCard(ctx, forecast);
        if (auto err = std::get_if<TError>(&card)) {
            return *err;
        }

        NSc::TValue divCard = std::get<NSc::TValue>(card);
        // DIALOG-3578: Отображать карточку с картой для случая, когда осадки в процессе
        if (IsPrecipitation(fact) && HasNowcast(nowcast)) {
            AddNowcastDivCardBlock(ctx, divCard, nowcast);
            return SuggestDetailsUrlSlot(ctx);
        } else {
            ctx.AddDivCardBlock(TStringBuf("weather__precipitation"), divCard);
        }
    }

    if (showDetails) {
        return SuggestDetailsUrlSlot(ctx);
    } else {
        return SuggestHomeUrlSlot(ctx);
    }
}

bool IsByHoursForecastCase(TContext& ctx) {
    return IsSlotEmpty(ctx.GetSlot("day_part"));
}

struct TForecast {
    TInstant Time;
    int Hour;
    i64 PrecType;
    TMaybe<float> PrecStrength;

    bool IsPrecipitation() const {
        return PrecStrength && PrecStrength > 0.0;
    }

    TStringBuf DayPart() const {
        if (Hour < 6)
            return "night";

        if (Hour < 12)
            return "morning";

        if (Hour < 18)
            return "day";

        return "evening";
    }
};

TResultValue PrepareByHoursForecastSlot(TContext& ctx, const NSc::TValue& forecast, const NSc::TValue& nowcast) {
    auto time = TInstant::Seconds(forecast["now"].GetIntNumber());
    auto timeNextDay = time + TDuration::Days(1);

    const NSc::TValue& fact = forecast["fact"];
    auto isCurrentPrecipitation = IsPrecipitation(fact);
    ctx.GetOrCreateSlot("precipitation_current", "num")->Value.SetIntNumber(isCurrentPrecipitation);

    TSlot* precDayPartSlot = ctx.GetOrCreateSlot("precipitation_day_part", "string");
    TSlot* precTypeSlot = ctx.GetOrCreateSlot("precipitation_type", "num");
    TSlot* precChangeHoursSlot = ctx.GetOrCreateSlot("precipitation_change_hours", "num");

    precTypeSlot->Value.SetIntNumber(fact["prec_type"].GetIntNumber());

    const auto& days = forecast["forecasts"].GetArray();
    std::vector<TForecast> hours;
    for (const auto& day : days) {
        for (const auto& hour: day["hours"].GetArray()) {
            TForecast h {
                TInstant::Seconds(hour["hour_ts"].GetIntNumber()),
                FromString(hour["hour"].GetString()),
                hour["prec_type"].GetIntNumber(),
                Nothing()
            };

            if (!hour["prec_strength"].IsNull() && hour["prec_strength"].IsNumber()) {
                h.PrecStrength = hour["prec_strength"].GetNumber();
            }

            hours.push_back(h);
        }
    }

    int foundChanges = 0;
    for (const auto& fore: hours) {
        // Ищем изменения осадков в 24-х часовом окне
        if (fore.Time <= time) {
            continue;
        }

        if (fore.Time > timeNextDay) {
            break;
        }

        // Первое изменение осадков
        if (isCurrentPrecipitation != fore.IsPrecipitation() && foundChanges == 0) {
            precDayPartSlot->Value.SetString(fore.DayPart());
            precChangeHoursSlot->Value.SetIntNumber((fore.Time - time).Hours() + 1);

            if (!isCurrentPrecipitation) {
                precTypeSlot->Value.SetIntNumber(fore.PrecType);
            }

            foundChanges += 1;
            isCurrentPrecipitation = fore.IsPrecipitation();
            continue;
        }

        if (ctx.HasExpFlag("weather_precipitation_starts_ends")) {
            auto userTime = GetUserTime(forecast);
            ctx.GetOrCreateSlot("date", "string")->Value.SetString(userTime.ToString("%F-%T"));
            ctx.GetOrCreateSlot("tz", "string")->Value.SetString(userTime.TimeZone().name());

            // ASSISTANT-3085: Поддержать закончится-начнется со стороны BASS
            TSlot* precNextDayPartSlot = ctx.GetOrCreateSlot("precipitation_next_day_part", "string");
            TSlot* precNextTypeSlot = ctx.GetOrCreateSlot("precipitation_next_type", "num");
            TSlot* precNextChangeHoursSlot = ctx.GetOrCreateSlot("precipitation_next_change_hours", "num");
            precNextChangeHoursSlot->Value.SetNumber(0);

            if (isCurrentPrecipitation != fore.IsPrecipitation() && foundChanges == 1) {
                precNextDayPartSlot->Value.SetString(fore.DayPart());
                precNextChangeHoursSlot->Value.SetIntNumber((fore.Time - time).Hours() + 1);

                if (!isCurrentPrecipitation) {
                    precNextTypeSlot->Value.SetIntNumber(fore.PrecType);
                }

                break;
            }
        }
    }

    if (ctx.ClientFeatures().SupportsDivCards()) {
        auto card = MakeDivCard(ctx, forecast);
        if (auto err = std::get_if<TError>(&card)) {
            return *err;
        }

        NSc::TValue divCard = std::get<NSc::TValue>(card);
        // DIALOG-3578: Отображать карточку с картой для случая, когда осадки в процессе
        if (IsPrecipitation(fact) && HasNowcast(nowcast)) {
            AddNowcastDivCardBlock(ctx, divCard, nowcast);
            return SuggestNowcastNowUrlSlot(ctx);
        } else {
            ctx.AddDivCardBlock(TStringBuf("weather__precipitation"), divCard);
        }
    }

    return SuggestHomeUrlSlot(ctx);
}

TResultValue PrepareDefaultSlot(TContext& ctx, const NSc::TValue& forecast, const NSc::TValue& nowcast) {
    if (ctx.ClientFeatures().SupportsDivCards()) {
        auto card = MakeDivCard(ctx, forecast);
        if (auto err = std::get_if<TError>(&card)) {
            return *err;
        }

        NSc::TValue divCard = std::get<NSc::TValue>(card);
        // DIALOG-3578: Отображать карточку с картой для случая, когда осадки в процессе
        const NSc::TValue& fact = forecast["fact"];
        if (IsPrecipitation(fact) && HasNowcast(nowcast)) {
            AddNowcastDivCardBlock(ctx, divCard, nowcast);
            return SuggestNowcastNowUrlSlot(ctx);
        } else {
            ctx.AddDivCardBlock(TStringBuf("weather__precipitation"), divCard);
        }
    }

    return SuggestHomeUrlSlot(ctx);
}

TResultValue CasesLogic(TContext& ctx) {
    CorrectGetRequestedCity(ctx);

    ctx.AddSuggest(TStringBuf("forecast_today"));
    ctx.AddSearchSuggest();
    ctx.AddOnboardingSuggest();

    // TODO: Remove unused slots
    TSlot* nowCastSlot = ctx.GetOrCreateSlot("weather_nowcast_alert", "string");
    TSlot* precDayPartSlot = ctx.GetOrCreateSlot("precipitation_day_part", "string");
    TSlot* precForDayPartSlot = ctx.GetOrCreateSlot("precipitation_for_day_part", "num");
    TSlot* currentPrecSlot = ctx.GetOrCreateSlot("precipitation_current", "num");
    TSlot* precChangeHoursSlot = ctx.GetOrCreateSlot("precipitation_change_hours", "num");
    TSlot* phraseNumber = ctx.GetOrCreateSlot("set_number", "num");
    TSlot* precTypeSlot = ctx.GetOrCreateSlot("precipitation_type", "num");

    // clean old values
    precTypeSlot->Value.SetNull();
    nowCastSlot->Value.SetNull();
    currentPrecSlot->Value.SetNull();
    precDayPartSlot->Value.SetNull();
    precForDayPartSlot->Value.SetNull();

    precChangeHoursSlot->Value.SetNumber(0);

    auto forecastVariant = FetchForecastFromWeatherApi(ctx);
    if (auto err = std::get_if<TError>(&forecastVariant)) {
        return *err;
    }
    auto forecast = std::get<NSc::TValue>(forecastVariant);

    auto now = forecast["now"].GetIntNumber();
    if (phraseNumber->Value.IsNull()) {
        phraseNumber->Value.SetIntNumber(now % RANDOM_CAP);
    } else {
        phraseNumber->Value.SetIntNumber(
                (phraseNumber->Value.GetIntNumber() + 1) % RANDOM_CAP);
    }

    auto isGetWeatherScenario = IsGetWeatherScenario(ctx, forecast);
    if (auto err = std::get_if<TError>(&isGetWeatherScenario)) {
        return *err;
    }
    if (std::get<bool>(isGetWeatherScenario)) {
        return RedirectToWeather(ctx);
    }

    auto nowcastVariant = FetchNowcastFromWeatherApi(ctx);
    if (auto err = std::get_if<TError>(&nowcastVariant)) {
        return *err;
    }
    auto nowcast = std::get<NSc::TValue>(nowcastVariant);

    auto forecastLocationSlotVariant = GetForecastLocationSlot(ctx);
    if (auto* forecastLocationSlot = std::get_if<NSc::TValue>(&forecastLocationSlotVariant)) {
        AddForecastLocationSlot(ctx, *forecastLocationSlot);
    }

    auto isNowcastForNowCase = IsNowcastForNowCase(ctx, forecast, nowcast);
    if (auto err = std::get_if<TError>(&isNowcastForNowCase)) {
        return *err;
    }
    if (std::get<bool>(isNowcastForNowCase)) {
        return PrepareNowcastForNow(ctx, forecast, nowcast);
    }

    // find fact value of precipitation
    const NSc::TValue& fact = forecast["fact"];
    bool isCurrentPrecipitation = IsPrecipitation(fact);
    currentPrecSlot->Value.SetIntNumber(isCurrentPrecipitation);

    // set current precipitation type in slot
    // it won't changed if precipitation ended in future
    precTypeSlot->Value.SetIntNumber(fact["prec_type"].GetIntNumber());

    if (IsDayPartForecastCase(ctx)) {
        return PrepareDayPartForecastSlot(ctx, forecast, nowcast);
    }

    if (IsByHoursForecastCase(ctx)) {
        return PrepareByHoursForecastSlot(ctx, forecast, nowcast);
    }

    return PrepareDefaultSlot(ctx, forecast, nowcast);
}
} // namespace

void TWeatherNowcastFormHandler::Register(THandlersMap* handlers) {
    auto cbWeatherNowcastForm = []() {
        return MakeHolder<TWeatherNowcastFormHandler>();
    };
    handlers->RegisterFormHandler(GET_WEATHER_NOWCAST, cbWeatherNowcastForm);
    handlers->RegisterFormHandler(GET_WEATHER_NOWCAST_ELLIPSIS, cbWeatherNowcastForm);
}

TResultValue TWeatherNowcastFormHandler::Do(TRequestHandler& r) {
    auto& ctx = r.Ctx();
    NWeather::SetWeatherProductScenario(r.Ctx());
    try {
        TryParseJsonFromAllSlots(ctx);

        // DIALOG-3616: [Мигающий баг] Рассинхрон в текстовом ответе и карте осадков
        auto when = ctx.GetSlot("when");

        if (!IsSlotEmpty(when) && when->Value.Get("days").GetIntNumber(0) == 0 && when->Value.Get("days_relative").GetBool(false) == true) {
            when->Value.SetNull();
        }

        auto err = CasesLogic(ctx);

        if (err) {
            ctx.AddErrorBlock(*err);
        }
    } catch(...) {
        ctx.AddErrorBlock(TError(TError::EType::WEATHERERROR, CurrentExceptionMessage()));
    }

    return Nothing();
}

} // namespace NBASS

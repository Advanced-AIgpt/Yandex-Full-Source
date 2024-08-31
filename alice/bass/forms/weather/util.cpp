#include "util.h"

#include <util/charset/wide.h>

#include <alice/bass/forms/directives.h>
#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/versioning/versioning.h>

using NAlice::TDateTime;
using NAlice::TDateTimeList;

namespace NBASS::NWeather {

namespace {

constexpr TStringBuf YANDEX_PROGODA_URL = "https://yandex.ru/pogoda";
constexpr TStringBuf LED_SCREEN_DIRECTIVE = "draw_led_screen";
constexpr TStringBuf FORCE_DISPLAY_CARDS_DIRECTIVE = "force_display_cards";
constexpr TStringBuf GIF_URI_PREFIX = "https://static-alice.s3.yandex.net/led-production/";
constexpr TStringBuf WEATHER = "weather";
constexpr TStringBuf GIF_PATH_SEP = "/";
constexpr TStringBuf WEATHER_GIF_VERSION = "1";
constexpr TStringBuf WEATHER_GIF_DEFAULT_SUBVERSION = "4";

constexpr size_t WEATHER_DAYS_AVAILABLE = 10;

bool IsGeoChanged(const TContext& ctx, const TRequestedGeo& city) {
    if (!city.IsSame()) {
        const TContext::TSlot* slot = ctx.GetSlot("where");
        if (!IsSlotEmpty(slot)) {
            return true;
        }
    }
    return false;
}

bool CanIgnoreUnknownWhere(const TContext& ctx) {
    if (!ctx.Meta().HasUtterance()) {
        return false;
    }

    const TContext::TSlot* whereSlot = ctx.GetSlot(TStringBuf("where"));
    if (IsSlotEmpty(whereSlot) || whereSlot->Type != TStringBuf("string")) {
        return false;
    }

    TUtf16String utterance = UTF8ToWide(ctx.Meta().Utterance());
    utterance.to_lower();

    TUtf16String value = UTF8ToWide(whereSlot->Value.GetString());
    value.to_lower();

    return !utterance.Contains(value);
}

TString GetPrecipationGifName(const int precType, const double precStrength) {
    if (precType == 1) {  // rain
        if (precStrength <= 0.25) {
            return "rain_low";
        } else if (precStrength >= 0.75) {
            return "rain_hi";
        } else {
            return "rain_medium";
        }
    } else if (precType == 3) {  // snow
        if (precStrength <= 0.25) {
            return "snow_low";
        } else if (precStrength >= 0.75) {
            return "snow_hi";
        } else {
            return "snow_medium";
        }
    } else if (precType == 2) {  // sleet
        return "snow_rain";
    }
    return "";
}

} // namespace

std::variant<std::unique_ptr<TDateTimeList>, TError> GetDateTimeList(TContext& ctx, const TDateTime::TSplitTime& userTime) {
    const TSlot* dayPart = ctx.GetSlot("day_part");
    const TSlot* when = ctx.GetSlot("when");

    try {
        return TDateTimeList::CreateFromSlot(when, dayPart, TDateTime(userTime), {WEATHER_DAYS_AVAILABLE, true});
    } catch (const yexception& e) {
        return TError(TError::EType::INVALIDPARAM, e.what());
    }
}

std::variant<TRequestedGeo, TError> GetCity(TContext& ctx) {
    TRequestedGeo requestedCity(ctx, TStringBuf("where"), false /* failIfNoSlot */, ctx.HasExpFlag("trequestedgeo") ? "geo" : "geo,biz");

    if (requestedCity.HasError() && CanIgnoreUnknownWhere(ctx)) {
        // MEGAMIND-1798 и ALICE-8057: не повторяем неизвестный where
        requestedCity = TRequestedGeo(ctx, nullptr, false /* failIfNoSlot */, ctx.HasExpFlag("trequestedgeo") ? "geo" : "geo,biz");
    }

    if (requestedCity.HasError()) {
        return TError(TError::EType::NOGEOFOUND, TStringBuilder() << TStringBuf("Requested location not found"));
    }

    // DIALOG-4463: Сломались фоллбэки для странного/невалидного гео
    auto geoType = requestedCity.GetGeoType();
    if (geoType != NGeobase::ERegionType::CITY &&
        geoType != NGeobase::ERegionType::VILLAGE &&
        geoType != NGeobase::ERegionType::AIRPORT &&
        geoType != NGeobase::ERegionType::SETTLEMENT
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

        if (NAlice::IsValidId(city)) {
            requestedCity.ConvertTo(city);
        }
    }

    // ASSISTANT-2976: Поправить страны-города для дождевого
    // Для округов/регионов находим город с тем же именем (например Сингапур).
    TMaybe<NSmallGeo::TRegions::TId> regions =
        NSmallGeo::TRegions::Instance().FindCityWithSameName(requestedCity.GetId());
    if (regions) {
        requestedCity = TRequestedGeo(ctx.GlobalCtx(), regions.GetRef(), requestedCity.IsSame());
    }

    return requestedCity;
}

std::variant<TLatLon, TError> GetLatLon(TContext& ctx) {
    auto where = ctx.GetSlot("where");
    if (IsSlotEmpty(where)) {
        auto location = ctx.Meta().Location();
        return TLatLon{ToString(location.Lat()), ToString(location.Lon())};
    }

    auto cityVariant = GetCity(ctx);
    if (auto err = std::get_if<TError>(&cityVariant)) {
        return *err;
    }

    auto& city = std::get<TRequestedGeo>(cityVariant);
    auto region = city.GetRegion();
    return TLatLon{ToString(region.GetLatitude()), ToString(region.GetLongitude())};
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

NSc::TValue GetHours(const THour& hour) {
    NSc::TValue hours;
    auto& hoursArray = hours.GetArrayMutable();

    const THour* currentHour = &hour;
    while (currentHour && hoursArray.size() < 24) {
        NSc::TValue hourData;
        hourData["local_day_time"] = Sprintf("%02lu:00", currentHour->Hour);
        hourData["temperature"] = currentHour->Temp;
        hourData["icon"] = currentHour->IconUrl(48);
        hoursArray.push_back(hourData);

        currentHour = currentHour->Next;
    }

    return hours;
}

std::variant<NSc::TValue, TError> GetForecastLocationSlot(TContext& ctx) {
    auto cityVariant = GetCity(ctx);
    if (auto err = std::get_if<TError>(&cityVariant)) {
        return *err;
    }

    auto city = std::get<TRequestedGeo>(cityVariant);
    if (!NAlice::IsValidId(city.GetId())) {
        return TError(TError::EType::NOGEOFOUND, "Names for requested location are missing");
    }

    NSc::TValue forecastLocation;
    forecastLocation["geoid"].SetIntNumber(city.GetId());
    if (IsGeoChanged(ctx, city)) {
        forecastLocation["geo_changed"].SetBool(true);
    }
    city.AddAllCaseForms(ctx, &forecastLocation, true /* wantObsolete */);

    return forecastLocation;
}

TRequestedGeo CorrectGetRequestedCity(TContext& ctx) {
    TRequestedGeo requestedCity(ctx, TStringBuf("where"));
    if (!requestedCity.HasError()) {
        TMaybe<NSmallGeo::TRegions::TId> regions = NSmallGeo::TRegions::Instance().FindCityWithSameName(requestedCity.GetId());
        if (regions)
            requestedCity = TRequestedGeo(ctx.GlobalCtx(), regions.GetRef(), requestedCity.IsSame());
    }
    if (!IsSlotEmpty(ctx.GetSlot("where"))) {
        const auto whereSlot = ctx.GetSlot("where");
        if (whereSlot->Type == "string") {
            TMaybe<TString> newName = NSmallGeo::TRegions::Instance().FindRegionInFixList(whereSlot->Value.GetString());
            if (newName) {
                ctx.GetSlot("where")->Value = newName.GetRef();
                ctx.GetSlot("where")->SourceText = newName.GetRef();
                requestedCity = TRequestedGeo(ctx, TStringBuf("where"));
            } else {
                auto fixElement = NSmallGeo::TRegions::Instance().FindLatLonInFixList(whereSlot->Value.GetString());
                if (fixElement)
                    requestedCity = TRequestedGeo(ctx.GlobalCtx(), fixElement->ParentId, true /* same */);
            }
        }
    }
    return requestedCity;
}

void AddForecastLocationSlot(TContext& ctx, NSc::TValue forecastLocation) {
    ctx.CreateSlot("forecast_location", "geo", true /* optional */, forecastLocation);
    if (forecastLocation["geo_changed"].GetBool(false)) {
        ctx.AddAttention(TStringBuf("geo_changed"));
    }
}

std::variant<TString, TError> GetWeatherUrl(TContext& ctx, TMaybe<int> anchorDay) {
    auto latLon = GetLatLon(ctx);
    if (auto err = std::get_if<TError>(&latLon)) {
        return *err;
    }

    auto [lat, lon] = std::get<TLatLon>(latLon);

    TCgiParameters frontendCgi;
    frontendCgi.InsertUnescaped("lat", lat);
    frontendCgi.InsertUnescaped("lon", lon);
    frontendCgi.InsertUnescaped("from", "alice_weathercard");

    TStringBuilder url;

    url << YANDEX_PROGODA_URL << "?" << frontendCgi.Print();
    url << "&utm_source=alice&utm_campaign=card";

    if (anchorDay) {
         url << "#d_" << *anchorDay;
    }

    return TString(url);
}

std::variant<TString, TError> GetWeatherMonthUrl(TContext& ctx, TMaybe<int> anchorDay) {
    auto latLon = GetLatLon(ctx);
    if (auto err = std::get_if<TError>(&latLon)) {
        return *err;
    }

    auto [lat, lon] = std::get<TLatLon>(latLon);

    TCgiParameters frontendCgi;
    frontendCgi.InsertUnescaped("lat", lat);
    frontendCgi.InsertUnescaped("lon", lon);
    frontendCgi.InsertUnescaped("from", "alice_weathercard");

    TStringBuilder url;

    url << YANDEX_PROGODA_URL << "/month?" << frontendCgi.Print();
    url << "&utm_source=alice&utm_campaign=card";

    if (anchorDay) {
         url << "#d_" << *anchorDay;
    }

    return TString(url);
}

std::pair<int, int> GetMinMaxTempsFromDayParts(const TParts& parts) {
    int tempMin = parts.Night.TempMin, tempMax = parts.Night.TempMax;
    TVector<TDayPart> dayParts{parts.Night, parts.Morning, parts.Day, parts.Evening};
    for (const auto& part : dayParts) {
        if (tempMin > part.TempMin) {
            tempMin = part.TempMin;
        }
        if (tempMax < part.TempMax) {
            tempMax = part.TempMax;
        }
    }
    return {tempMin, tempMax};
}

void SetWeatherProductScenario(TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::WEATHER);
}

void TryParseJsonFromAllSlots(TContext& ctx) {
    TVector<TSlot*> slots = ctx.GetSlots();
    for (auto* slot: slots) {
        if (!IsSlotEmpty(slot) && slot->Value.IsString() && TStringBuf("string") != slot->Type) {
            NSc::TValue slotValue;
            if (NSc::TValue::FromJson(slotValue, slot->Value.GetString())) {
                slot->Value = std::move(slotValue);
            }
        }
    }
}

void AddLedDirective(TContext& ctx, const TVector<TString>& imageUris) {
    ctx.AddCommand<TForceDisplayCardsDirective>(FORCE_DISPLAY_CARDS_DIRECTIVE, {});

    NSc::TValue data;
    data["listening_is_possible"].SetBool(true);
    for (const auto& imageUri : imageUris) {
        NSc::TValue& item = data["animation_sequence"].Push();
        item["frontal_led_image"] = imageUri;
    }
    ctx.AddCommand<TDrawLedScreenDirective>(LED_SCREEN_DIRECTIVE, std::move(data));
}

TVector<TString> MakeWeatherGifUris(const TContext& ctx, int temperature, size_t precType, double precStrength, double cloudiness) {
    TVector<TString> gifNames({
        TString::Join("temperature/", ToString(temperature)),
    });

    if (cloudiness < 0.01 && precStrength < 0.01) {  // clear
        gifNames.push_back("sun");
    } else if (precStrength < 0.01) {  // cloudy
        gifNames.push_back("clouds");
    } else if (precType > 0) {  // precipitation
        TString precipitationGifName = GetPrecipationGifName(precType, precStrength);
        if (precipitationGifName) {
            gifNames.push_back(precipitationGifName);
        }
    }

    const auto versionFlagFull = TString::Join(NAlice::NExperiments::EXP_GIF_VERSION, WEATHER, ":");
    const auto subversion = ctx.GetValueFromExpPrefix(
        versionFlagFull
    ).GetOrElse(WEATHER_GIF_DEFAULT_SUBVERSION);
    TVector<TString> gifUris;
    for (const auto& gifName : gifNames) {
        gifUris.push_back(
            NAlice::FormatVersion(
                TString::Join(GIF_URI_PREFIX, WEATHER),
                TString::Join(gifName, ".gif"),
                WEATHER_GIF_VERSION,
                subversion,
                /* sep = */ GIF_PATH_SEP
            )
        );
    }

    return gifUris;
}

} // namespace NBASS::NWeather

#include "util.h"

#include <alice/library/json/json.h>

#include <library/cpp/iterator/functools.h>
#include <library/cpp/json/json_reader.h>

#include <util/generic/hash.h>
#include <util/string/printf.h>

#include <alice/hollywood/library/scenarios/weather/context/api.h>
#include <alice/hollywood/library/scenarios/weather/util/translations.h>

#include <weather/app_host/utils/aggregation/aggregation.h>

using NAlice::TDateTime;
using NAlice::TDateTimeList;

namespace NAlice::NHollywood::NWeather {

namespace {

constexpr size_t WEATHER_DAYS_AVAILABLE = 10;

// For compatibility with legacy Datetime slots
struct TFlatSlot {
    TFlatSlot(TString name, TString type, TString value)
        : Name{name}
        , Type{type}
        , Value{value}
    {}

    TString Name;
    TString Type;
    TString Value;
};

std::unique_ptr<TFlatSlot> ConsructFlatSlot(const TPtrWrapper<TSlot>& slot) {
    if (!slot) {
        return nullptr;
    }
    return std::make_unique<TFlatSlot>(slot->Name, slot->Type, slot->Value.AsString());
}

NJson::TJsonValue LookupGeoBaseForecastLocation(const NGeobase::TLookup& geobase, const NGeobase::TId geoId, const ELanguage language) {
    NSc::TValue caseForms;
    const TStringBuf languageStr = IsoNameByLanguage(language);
    NAlice::AddAllCaseFormsWithFallbackLanguage(geobase, geoId, languageStr, &caseForms, /* wantObsolete = */ true);
    NJson::TJsonValue forecastLocation;
    NJson::ReadJsonFastTree(caseForms.ToJson(), &forecastLocation);
    return forecastLocation;
}

} // namespace

TMaybe<TString> GetFrameNameFromCallback(const NScenarios::TCallbackDirective& callback) {
    const auto& fields = callback.GetPayload().fields();
    if (fields.count("form_update")) {
        const auto& formUpdateFields = fields.at("form_update").struct_value().fields();
        if (formUpdateFields.count("name")) {
            return formUpdateFields.at("name").string_value();
        }
    }
    return Nothing();
}

bool IsNewFrameLegal(const TStringBuf prevFrameName, const TStringBuf newFrameName) {
    if (newFrameName == NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS) {
        return prevFrameName == NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP || prevFrameName == NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS;
    }
    if (newFrameName == NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS) {
        return prevFrameName == NFrameNames::GET_WEATHER_NOWCAST || prevFrameName == NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS;
    }
    if (newFrameName == NFrameNames::GET_WEATHER__DETAILS || newFrameName == NFrameNames::GET_WEATHER__ELLIPSIS) {
        return prevFrameName == NFrameNames::GET_WEATHER || prevFrameName == NFrameNames::GET_WEATHER__DETAILS || prevFrameName == NFrameNames::GET_WEATHER__ELLIPSIS;
    }
    if (newFrameName == NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS) {
        return prevFrameName == NFrameNames::GET_WEATHER_PRESSURE || prevFrameName == NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS;
    }
    if (newFrameName == NFrameNames::GET_WEATHER_WIND__ELLIPSIS) {
        return prevFrameName == NFrameNames::GET_WEATHER_WIND || prevFrameName == NFrameNames::GET_WEATHER_WIND__ELLIPSIS;
    }

    return true;
}

bool IsTakeSlotsFromPrevFrame(const TStringBuf newFrameName, const TExpFlags& expFlags) {
    if (expFlags.contains(NExperiment::WEATHER_USE_CROSS_SCENARIO_ELLIPSIS)) {
        // allow slots unless it's nowcast
        return newFrameName != NFrameNames::GET_WEATHER_NOWCAST;
    }

    return newFrameName != NFrameNames::GET_WEATHER && newFrameName != NFrameNames::GET_WEATHER_NOWCAST &&
           newFrameName != NFrameNames::GET_WEATHER_WIND && newFrameName != NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP &&
           newFrameName != NFrameNames::GET_WEATHER_PRESSURE;
}

bool IsGeoMetaSearchDisabled(const TExpFlags& expFlags) {
    return expFlags.contains(NExperiment::WEATHER_DISABLE_GEO_META_SEARCH);
}

void FillFromPrevFrame(TRTLogger& logger, const TFrame& prevFrame, TFrame& frame) {
    bool whenSlotChanged = false;
    for (const auto& slot: frame.Slots()) {
        if (slot.Name == "when") {
            whenSlotChanged = true;
            break;
        }
    }

    for (const auto& prevSlot: prevFrame.Slots()) {
        if (!frame.FindSlot(prevSlot.Name)) {
            // don't fill "day_part" if "when" changed
            if (prevSlot.Name == "day_part" && whenSlotChanged) {
                continue;
            }

            LOG_INFO(logger) << "Add remembered Weather slot, name " << prevSlot.Name.Quote()
                << ", type " << prevSlot.Type.Quote()
                << ", value " << prevSlot.Value.AsString().Quote();
            frame.AddSlot(prevSlot);
        }
    }
}

void FixDayPartSlot(TRTLogger& logger, const TPtrWrapper<TSlot>& slot) {
    if (!slot) {
        return;
    }

    // "nights", "mornings", etc... -> "night", "morning"
    const auto& value = slot->Value.AsString();
    if (value.EndsWith("s")) {
        LOG_INFO(logger) << "Chop \"s\" from the end of the \"day_part\": " << value.Quote();

        const_cast<TSlot*>(slot.Get())->Value = TSlot::TValue(value.substr(0, value.size() - 1));
    }
}

TStringBuf GetFrameName(const TWeatherState& state) {
    if (state.HasSemanticFrame()) {
        return state.GetSemanticFrame().GetName();
    }
    return TStringBuf();
}

TMaybe<TString> GetWeatherForecastUri(const TFrame& frame) {
    TPtrWrapper<TSlot> slot = frame.FindSlot("weather_forecast");
    if (slot == nullptr) {
        return Nothing();
    }

    NJson::TJsonValue value;
    if (!NJson::ReadJsonFastTree(slot->Value.AsString(), &value)) {
        return Nothing();
    }

    NJson::TJsonValue uri;
    if (value.GetValue("uri", &uri)) {
        if (uri.IsString()) {
            return uri.GetString();
        }
    }
    return Nothing();
}

TString ConstructSlotRememberValue(const NJson::TJsonValue& value) {
    if (value.IsString()) {
        return value.GetString();
    }
    return JsonToString(value);
}

bool IsWeatherChangeScenario(const TStringBuf frameName) {
    return frameName == NFrameNames::GET_WEATHER_CHANGE;
}

bool IsNowcastWeatherScenario(const TStringBuf frameName) {
    return frameName == NFrameNames::GET_WEATHER_NOWCAST || frameName == NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS;
}

bool IsPrecMapWeatherScenario(const TStringBuf frameName) {
    return frameName == NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP || frameName == NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS;
}

bool IsWeatherScenario(const TStringBuf frameName) {
    return frameName == NFrameNames::GET_WEATHER || frameName == NFrameNames::GET_WEATHER__ELLIPSIS;
}

bool IsWindWeatherScenario(const TStringBuf frameName) {
    return frameName == NFrameNames::GET_WEATHER_WIND || frameName == NFrameNames::GET_WEATHER_WIND__ELLIPSIS;
}

bool IsPressureWeatherScenario(const TStringBuf frameName) {
    return frameName == NFrameNames::GET_WEATHER_PRESSURE || frameName == NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS;
}

TString TranslateWindDirection(const TString& direction, const ELanguage language, TRTLogger& logger) {
    TTranslations windDirectionTranslator{logger, language};
    const auto key = TString::Join("wind-", direction);
    const auto translation = windDirectionTranslator.Translate(key);
    return translation == key ? "" : translation;
}

int GetPressureMMFromDay(const TDay& day) {
    if (const auto& hours = day.Hours; !hours.empty()) {
        const auto func = [](const auto& hour) { return hour.PressureMM; };
        return ::NWeather::Avg(NFuncTools::Map(func, hours));
    } else if (const auto& dayPartsMap = day.Parts.Map; !dayPartsMap.empty()) {
        const auto func = [](const auto& kv) { return kv.second->PressureMM; };
        return ::NWeather::Avg(NFuncTools::Map(func, dayPartsMap));
    }

    return 0;
}

TString GetTranslatedWindDirectionFromDay(const TDay& day, const ELanguage language, TRTLogger& logger) {
    // return info only about the day time
    const TString& direction = day.Parts.DayShort.WindDir;
    return TranslateWindDirection(direction, language, logger);
}

double GetWindSpeedFromDay(const TDay& day) {
    // return info only about the day time
    return day.Parts.DayShort.WindSpeed;
}

double GetWindGustFromDay(const TDay& day) {
    // return info only about the day time
    return day.Parts.DayShort.WindGust;
}

bool IsSlotEmpty(const TPtrWrapper<TSlot>& slot) {
    return !slot.IsValid() || slot->Value.AsString() == "null";
}

TWeatherErrorOr<std::unique_ptr<NAlice::TDateTimeList>> GetDateTimeList(const TWeatherContext& ctx, const TDateTime::TSplitTime& userTime) {
    auto& frame = ctx.Frame();
    if (frame.Defined()) {
        std::unique_ptr<TFlatSlot> dayPart = ConsructFlatSlot(frame->FindSlot("day_part"));
        std::unique_ptr<TFlatSlot> when = ConsructFlatSlot(frame->FindSlot("when"));

        try {
            return TDateTimeList::CreateFromSlot(when.get(), dayPart.get(), TDateTime(userTime), {WEATHER_DAYS_AVAILABLE, true});
        } catch (const yexception& e) {
            return TWeatherError(EWeatherErrorCode::INVALIDPARAM) << e.what();
        }
    }
    return TWeatherError(EWeatherErrorCode::INVALIDPARAM) << "Frame empty";
}

NJson::TJsonValue GetHours(const THour& hour) {
    NJson::TJsonValue hours = NJson::TJsonArray();
    auto& hoursArray = hours.GetArraySafe();

    const THour* currentHour = &hour;
    while (currentHour && hoursArray.size() < 24) {
        NJson::TJsonValue hourData;
        hourData["local_day_time"] = Sprintf("%02lu:00", currentHour->Hour);
        hourData["temperature"] = currentHour->Temp;
        hourData["icon"] = currentHour->IconUrl(48);
        hourData["icon_type"] = currentHour->Icon;

        hoursArray.push_back(hourData);
        currentHour = currentHour->Next;
    }
    return hours;
}

TWeatherErrorOr<NJson::TJsonValue> GetForecastLocationSlot(const TWeatherContext& ctx) {
    if (!ctx.WeatherPlace().Defined()) {
        return TWeatherError(EWeatherErrorCode::NOGEOFOUND) << "Got no location from TWeatherPrepareCityHandle::Do";
    }
    const auto& place = *ctx.WeatherPlace();
    const NGeobase::TId id = place.GetCityGeoId();

    if (!NAlice::IsValidId(id)) {
        return TWeatherError(EWeatherErrorCode::NOGEOFOUND) << "Names for requested location are missing";
    }

    NJson::TJsonValue forecastLocation = LookupGeoBaseForecastLocation(ctx.GeobaseLookup(), id, ctx->UserLang);
    forecastLocation["geoid"] = id;
    if (place.GetCityChanged()) {
        forecastLocation["geo_changed"] = true;
    }
    return forecastLocation;
}

TWeatherErrorOr<NJson::TJsonValue> GetOriginalForecastLocationSlot(const TWeatherContext& ctx) {
    const auto& place = *ctx.WeatherPlace();
    const NGeobase::TId id = place.GetOriginalCityGeoId();

    if (!NAlice::IsValidId(id)) {
        return TWeatherError(EWeatherErrorCode::NOGEOFOUND) << "Names for requested location are missing";
    }

    NJson::TJsonValue forecastLocation = LookupGeoBaseForecastLocation(ctx.GeobaseLookup(), id, ctx->UserLang);
    forecastLocation["geoid"] = id;
    return forecastLocation;
}

TWeatherErrorOr<TString> ConstructWeatherUrl(const TWeatherContext& ctx, TMaybe<int> anchorDay, const TString& path, TVector<std::pair<TString, TString>> utms) {
    const auto [lat, lon] = ctx.GetLatLon();

    TCgiParameters frontendCgi;
    frontendCgi.InsertUnescaped("lat", ToString(lat));
    frontendCgi.InsertUnescaped("lon", ToString(lon));
    frontendCgi.InsertUnescaped("from", "alice_weathercard");

    frontendCgi.InsertUnescaped("utm_source", "alice");
    frontendCgi.InsertUnescaped("utm_campaign", "card");
    for (const auto& utm : utms) {
        frontendCgi.InsertUnescaped(utm.first, utm.second);
    }

    TStringBuilder url;
    url << YANDEX_POGODA_URL << path << "?" << frontendCgi.Print();

    if (anchorDay) {
         url << "#d_" << *anchorDay;
    }

    return static_cast<TString>(url);
}

TWeatherErrorOr<TString> GetWeatherUrl(const TWeatherContext& ctx, TMaybe<int> anchorDay) {
    return ConstructWeatherUrl(ctx, anchorDay, TString(""), {std::make_pair("utm_medium", "forecast")});
}

TWeatherErrorOr<TString> GetWeatherMonthUrl(const TWeatherContext& ctx, TMaybe<int> anchorDay) {
    return ConstructWeatherUrl(ctx, anchorDay, TString("/month"), {std::make_pair("utm_medium", "forecast")});
}

TWeatherErrorOr<TString> GetWeatherWindUrl(const TWeatherContext& ctx, TMaybe<int> anchorDay) {
    return ConstructWeatherUrl(ctx, anchorDay, TString(""), {std::make_pair("utm_medium", "wind")});
}

TMaybe<TWeatherError> AddForecastLocationSlots(TWeatherContext& ctx) {
    auto forecastLocationSlotVariant = GetForecastLocationSlot(ctx);
    if (auto err = std::get_if<TWeatherError>(&forecastLocationSlotVariant)) {
        return *err;
    }
    auto forecastLocationSlot = std::get<NJson::TJsonValue>(forecastLocationSlotVariant);
    ctx.AddSlot("forecast_location", "geo", forecastLocationSlot);
    if (forecastLocationSlot["geo_changed"].GetBoolean()) {
        ctx.Renderer().AddAttention("geo_changed");
    }

    auto originalForecastLocationSlotVariant = GetOriginalForecastLocationSlot(ctx);
    if (auto err = std::get_if<TWeatherError>(&originalForecastLocationSlotVariant)) {
        return *err;
    }
    auto originalForecastLocationSlot = std::get<NJson::TJsonValue>(originalForecastLocationSlotVariant);
    ctx.AddSlot("original_forecast_location", "geo", originalForecastLocationSlot);

    return Nothing();
}

std::pair<int, int> GetMinMaxTempsFromDayParts(const TParts& parts) {
    int tempMin = parts.Night.TempMin, tempMax = parts.Night.TempMax;
    for (const auto& [_, part] : parts.Map) {
        if (tempMin > part->TempMin) {
            tempMin = part->TempMin;
        }
        if (tempMax < part->TempMax) {
            tempMax = part->TempMax;
        }
    }
    return {tempMin, tempMax};
}

const TVector<NScenarios::TLayout::TButton>& NoButtons() {
    return Default<TVector<NScenarios::TLayout::TButton>>();
}

void FillShowViewHourItem(::NAlice::NData::TWeatherHourItem& hourItem, const THour& hour) {
    hourItem.SetHour(hour.Hour);
    hourItem.SetTimestamp(hour.HourTS.Seconds());
    hourItem.SetTemperature(hour.Temp);
    hourItem.SetIcon(hour.IconUrl());
    hourItem.SetIconType(hour.Icon);
    hourItem.SetPrecStrength(hour.PrecStrength);
    hourItem.SetPrecType(hour.PrecType);
}

void FillShowViewGeoLocation(::NAlice::NData::TWeatherLocation& location, const TWeatherContext& ctx) {
    auto locationSlot = ctx.FindSlot("forecast_location");
    if (!IsSlotEmpty(locationSlot)) {
        NJson::TJsonValue locationJson;
        NJson::ReadJsonFastTree(locationSlot->Value.AsString(), &locationJson);

        location.SetCity(locationJson["city"].GetString());
        location.SetCityPrepcase(locationJson["city_prepcase"].GetString());
    }
}

NDatetime::TCivilSecond ConvertTCivilTime(const TWeatherProtos::TCivilTime& time) {
    return NDatetime::TCivilSecond(time.GetYear(),
                                   time.GetMonth(),
                                   time.GetDay(),
                                   time.GetHour(),
                                   time.GetMinute(),
                                   time.GetSecond());
}

TString ConstructScledPattern(const int temperature) {
    if (temperature > 99) {
        return ConstructScledPattern(99);
    } else if (temperature < -99) {
        return ConstructScledPattern(-99);
    }
    return TString::Join("  ", Sprintf("% 3d", temperature), "*");
}

void TryAddWeatherGeoId(TRTLogger& logger, NAppHost::IServiceContext& serviceCtx, TWeatherErrorOr<NGeobase::TId> parsedGeoId) {
    if (const auto* err = std::get_if<TWeatherError>(&parsedGeoId)) {
        LOG_ERROR(logger) << err->Message();
    } else {
        TWeatherGeoId weatherGeoId;
        weatherGeoId.SetGeoId(std::get<NGeobase::TId>(parsedGeoId));
        serviceCtx.AddProtobufItem(weatherGeoId, "weather_geoid");
    }
}

void FillBackgroundSoundsFromFact(TBackgroundSounds& backgroundSounds, const TFact& fact) {
    backgroundSounds.SetWeatherCondition(fact.Condition);
    backgroundSounds.SetTemperature(fact.Temp);
    if (TBackgroundSounds::ESeason season; TryFromString(fact.Season, season)) {
        backgroundSounds.SetSeason(season);
    }
    if (TBackgroundSounds::EDayTime dayTime; TryFromString(fact.DayTime, dayTime)) {
        backgroundSounds.SetDayTime(dayTime);
    }
}

bool IsElementNull(const NJson::TJsonValue& value, const TStringBuf key) {
    return !value.Has(key) || value[key].IsNull() || value[key].GetType() == NJson::JSON_UNDEFINED;
}

void FixWhenSlotForNextWeekend(TWeatherContext& ctx) {
    const auto& forecast = *ctx.Forecast();
    auto userTime = forecast.UserTime;
    TPtrWrapper<TSlot> whenSlot = ctx.Frame()->FindSlot("when");

    // special case: when user asks for the next weekend, but it is not available, answer with the nearest weekend
    if (IsSlotEmpty(whenSlot)) {
        return;
    }
    if (TStringBuf("datetime_range_raw") != whenSlot->Type && TStringBuf("datetime_range") != whenSlot->Type) {
        return;
    }

    NJson::TJsonValue whenJson;
    NJson::ReadJsonFastTree(whenSlot->Value.AsString(), &whenJson);

    NJson::TJsonValue& startSrc = whenJson["start"];
    // check if it is really next weekend
    if (IsElementNull(startSrc, "weekend") || IsElementNull(startSrc, "weeks_relative") || startSrc["weeks"] != 1) {
        return;
    }

    // check whether these dates are not available (monday, tuesday, wednesday)
    if (userTime.WDay() <= 0 || userTime.WDay() > 3) {
        return;
    }
    startSrc["weeks"] = 0;
    whenJson["end"]["weeks"] = 0;

    const_cast<TSlot*>(whenSlot.Get())->Value = TSlot::TValue{NJson::WriteJson(whenJson)};

    ctx.Renderer().AddAttention("no_weather_for_next_weekend");
}

}  // namespace NAlice::NHollywood::NWeather

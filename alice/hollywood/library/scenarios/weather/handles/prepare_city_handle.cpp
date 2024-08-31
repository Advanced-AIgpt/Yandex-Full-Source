#include "prepare_city_handle.h"

#include <alice/hollywood/library/scenarios/weather/context/context.h>
#include <alice/hollywood/library/scenarios/weather/proto/weather.pb.h>
#include <alice/hollywood/library/scenarios/weather/request_helper/geometasearch.h>
#include <alice/hollywood/library/scenarios/weather/request_helper/reqwizard.h>
#include <alice/hollywood/library/scenarios/weather/util/error.h>

#include <alice/megamind/protos/common/events.pb.h>
#include <alice/megamind/protos/common/location.pb.h>
#include <alice/protos/data/lat_lon.pb.h>

#include <search/session/compression/report.h>
#include <search/idl/meta.pb.h>

#include <apphost/lib/proto_answers/http.pb.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NWeather {

namespace {

TUtf16String GetUtterance(const TScenarioInputWrapper& input) {
    TString userInput;
    if (input.IsTextInput()) {
        userInput = input.Proto().GetText().GetRawUtterance();
    } else if (input.IsVoiceInput()) {
        userInput = input.Proto().GetVoice().GetAsrData(0).GetUtterance();
    }

    TUtf16String utterance = UTF8ToWide(userInput);
    utterance.to_lower();

    return utterance;
}

bool CanIgnoreUnknownWhere(const TWeatherContext& ctx, const TPtrWrapper<TSlot>& where) {
    if (!where || where->Type != "string") {
        return false;
    }

    TUtf16String utterance = GetUtterance(ctx.RunRequest().Input());
    utterance.to_lower();

    TUtf16String value = UTF8ToWide(where->Value.AsString());
    value.to_lower();

    return !utterance.Contains(value);
}

bool TryParseResult(TRTLogger& logger, TWeatherErrorOr<NGeobase::TId> result, NGeobase::TId& geoId) {
    if (const auto* err = std::get_if<TWeatherError>(&result)) {
        LOG_ERROR(logger) << err->Message();
        return false;
    } else {
        geoId = std::get<NGeobase::TId>(result);
        return true;
    }
}

NGeobase::TId FixGeoIdRostovOnDon(NGeobase::TId id, const TUtf16String& utterance) {
    static constexpr NGeobase::TId ROSTOV_ON_DON_GEO_ID = 39;
    static constexpr NGeobase::TId ROSTOV_GEO_ID = 10838;
    static constexpr TWtringBuf ROSTOV_SUBSTR = u"велик";

    if (id == ROSTOV_GEO_ID && !utterance.Contains(ROSTOV_SUBSTR)) {
        return ROSTOV_ON_DON_GEO_ID;
    }
    return id;
}

} // namespace

void TWeatherPrepareCityHandle::Do(TScenarioHandleContext& ctx) const {
    TWeatherContext weatherCtx{ctx};
    TWeatherPlace weatherPlace;
    auto& logger = weatherCtx.Logger();
    auto& runRequest = weatherCtx.RunRequest();

    NGeobase::TId geoId = NGeobase::UNKNOWN_REGION;
    bool geoIdFromHttp = false;
    if (ctx.ServiceCtx.HasProtobufItem("weather_geoid")) {
        geoIdFromHttp = true;
        const auto weatherGeoId = ctx.ServiceCtx.GetOnlyProtobufItem<TWeatherGeoId>("weather_geoid");
        geoId = weatherGeoId.GetGeoId();
    }

    const TFrame frame = TFrame::FromProto(weatherCtx->ServiceCtx.GetOnlyProtobufItem<TSemanticFrame>("semantic_frame"));
    TPtrWrapper<TSlot> where = frame.FindSlot("where");

    TMaybe<TWeatherError> err = Nothing();
    if (where) {
        if (where->Type == "string" || where->Type == "where") {
            TStringBuf value;
            if (where && (where->Type == "string" || where->Type == "where")) {
                value = where->Value.AsString();
            }

            // TODO(sparkle): after graph release don't parse http here
            if (!geoIdFromHttp) {
                // there have been requests to reqwizard and geometasearch
                TReqwizardRequestHelper<ERequestPhase::After> reqWizard{weatherCtx.Ctx()};
                TGeometasearchRequestHelper<ERequestPhase::After> geoMetaSearch{weatherCtx.Ctx()};

                if (!TryParseResult(logger, reqWizard.TryParseGeoId(), geoId)) {
                    // ReqWizard parsing failure
                    if (IsGeoMetaSearchDisabled(runRequest.ExpFlags())) {
                        // Try parse from sys.geo slot
                        if (where->Type == "geo") {
                            if (const auto parserErr = ParseSysGeo(where->Value.AsString(), geoId)) {
                                LOG_ERROR(logger) << parserErr->Message();
                                err = TWeatherError(EWeatherErrorCode::NOGEOFOUND) << "sys.geo \"where\" slot resolving failure";
                            } else {
                                LOG_INFO(logger) << "Successfully got GeoId " << geoId << " from sys.geo";
                            }
                        } else {
                            err = TWeatherError(EWeatherErrorCode::NOGEOFOUND) << "sys.geo \"where\" slot is missing";
                        }
                    } else {
                        // Try parse from GeoMetaSearch
                        if (!TryParseResult(logger, geoMetaSearch.TryParseGeoId(value), geoId)) {
                            err = TWeatherError(EWeatherErrorCode::NOGEOFOUND) << "string \"where\" slot resolving failure";
                        }
                    }
                }
            }
        } else if (where->Type == "lat_lon") {
            if (const auto parserErr = ParseLatLon(where->Value.AsString(), geoId, weatherCtx.GeobaseLookup())) {
                LOG_ERROR(logger) << parserErr->Message();
                err = TWeatherError(EWeatherErrorCode::NOGEOFOUND) << "lat_lon \"where\" slot resolving failure";
            }
            LOG_INFO(logger) << "Successfully got GeoId " << geoId << " from lat_lon";
        } else if (where->Type == "geo") { // in fact "sys.geo", type name is cut
            if (const auto parserErr = ParseSysGeo(where->Value.AsString(), geoId)) {
                LOG_ERROR(logger) << parserErr->Message();
                err = TWeatherError(EWeatherErrorCode::NOGEOFOUND) << "sys.geo \"where\" slot resolving failure";
            }
            LOG_INFO(logger) << "Successfully got GeoId " << geoId << " from sys.geo";
        } else if (where->Type == "GeoAddr.Address") {
            if (const auto parserErr = ParseGeoAddrAddress(where->Value.AsString(), geoId)) {
                LOG_ERROR(logger) << parserErr->Message();
                err = TWeatherError(EWeatherErrorCode::NOGEOFOUND) << "GeoAddr.Address \"where\" slot resolving failure";
            }
            LOG_INFO(logger) << "Successfully got GeoId " << geoId << " from GeoAddr.Address";
        } else if (where->Type == "geo_id") {
            if (const auto parserErr = ParseGeoId(where->Value.AsString(), geoId)) {
                LOG_ERROR(logger) << parserErr->Message();
                err = TWeatherError(EWeatherErrorCode::NOGEOFOUND) << "geo_id \"where\" slot resolving failure";
            }
            LOG_INFO(logger) << "Successfully got GeoId " << geoId << " from geo_id";
        } else {
            // unknown where type
            err = TWeatherError(EWeatherErrorCode::INVALIDPARAM) << "Unknown where type: " << where->Type.Quote();
        }
    }

    bool geoIdFromUserLocation = false;

    // assume that this id is always valid
    NGeobase::TId userGeoId = weatherCtx.UserLocation().UserRegion();
    TMaybe<double> userLat, userLon;

    // use more precise user's lat+lon if we able to
    const auto& base = weatherCtx.RunRequest().Proto().GetBaseRequest();
    if (base.HasLocation()) {
        const auto& location = base.GetLocation();

        LOG_INFO(logger) << "Take lat+lon from BaseRequest Location: " << location.GetLat() << ", " << location.GetLon();
        userLat = location.GetLat();
        userLon = location.GetLon();

        if (!NAlice::IsValidId(userGeoId)) {
            userGeoId = weatherCtx.GeobaseLookup().GetRegionIdByLocation(location.GetLat(), location.GetLon());
            LOG_INFO(logger) << "Suspected GeoId is " << userGeoId;
        }
    } else if (NAlice::IsValidId(userGeoId)) {
        LOG_INFO(logger) << "Take lat+lon from GeoId " << userGeoId;

        const auto region = weatherCtx.GeobaseLookup().GetRegionById(userGeoId);
        userLat = region.GetLatitude();
        userLon = region.GetLongitude();
    }

    if (!where || (err.Defined() && CanIgnoreUnknownWhere(weatherCtx, where))) {
        geoIdFromUserLocation = true;

        geoId = userGeoId;
        err = Nothing();

        if (userLat && userLon) {
            weatherPlace.MutableUserLatLon()->SetLat(*userLat);
            weatherPlace.MutableUserLatLon()->SetLon(*userLon);
        }
    }

    if (err) {
        LOG_ERROR(logger) << err->Message();
        weatherCtx.AddError(*err);
    } else {
        geoId = FixGeoIdIfCityState(geoId);
        geoId = FixGeoIdRostovOnDon(geoId, GetUtterance(weatherCtx.RunRequest().Input()));

        if (NAlice::IsValidId(geoId)) {
            weatherPlace.SetOriginalCityGeoId(geoId);
        }

        NGeobase::TId clarifiedGeoId = ClarifyGeoId(weatherCtx.GeobaseLookup(), geoId);
        if (geoId != clarifiedGeoId) {
            LOG_INFO(logger) << "Clarified GeoId " << geoId << " -> " << clarifiedGeoId;

            geoId = clarifiedGeoId;
            if (!geoIdFromUserLocation) {
                weatherPlace.SetCityChanged(true);
            }
        }

        if (NAlice::IsValidId(geoId)) {
            weatherPlace.SetCityGeoId(geoId);
        }
    }

    weatherPlace.SetNonUserGeo(userGeoId != geoId);

    weatherCtx->ServiceCtx.AddProtobufItem(weatherPlace, "weather_place");
}

NGeobase::TId ClarifyGeoId(const NGeobase::TLookup& lookup, NGeobase::TId id) {
    if (!NAlice::IsValidId(id)) {
        return id;
    }

    NGeobase::TRegion region = lookup.GetRegionById(id);
    auto geoType = region.GetEType();

    if (geoType != NGeobase::ERegionType::CITY &&
            geoType != NGeobase::ERegionType::VILLAGE &&
            geoType != NGeobase::ERegionType::AIRPORT &&
            geoType != NGeobase::ERegionType::SETTLEMENT) {
        auto city = lookup.GetParentIdWithType(id, static_cast<int>(NGeobase::ERegionType::CITY));
        if (!NAlice::IsValidId(city)) {
            city = region.GetCapitalId();
        }
        if (!NAlice::IsValidId(city)) {
            auto district = lookup.GetParentIdWithType(id, static_cast<int>(NGeobase::ERegionType::DISTRICT));
            city = lookup.GetCapitalId(district);
        }
        if (NAlice::IsValidId(city)) {
            return city;
        }
    }

    return id;
}

NGeobase::TId FixGeoIdIfCityState(NGeobase::TId id) {
    static const THashMap<NGeobase::TId, NGeobase::TId> Fixlist = {
        {10024, 10143}, // Тунис
        {10070, 10465}, // Монако
        {10089, 10525}, // Гибралтар
        {10105, 10619}, // Сингапур
        {20790, 20796}, // Сан-Марино
        {20826, 20828}, // Алжир
        {20968, 21415}, // Гватемала
        {21203, 21204}, // Люксембург
        {21299, 21300}, // Панама
        {21359, 21946}, // Ватикан
        {21475, 21476}, // Джибути
    };

    const auto iter = Fixlist.find(id);
    if (!iter.IsEnd()) {
        return iter->second;
    }

    return id;
}

}  // namespace NAlice::NHollywood::NWeather

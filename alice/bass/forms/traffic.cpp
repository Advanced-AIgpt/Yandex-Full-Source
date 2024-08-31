#include "traffic.h"
#include "directives.h"

#include <alice/bass/forms/geocoder.h>
#include <alice/bass/forms/geodb.h>
#include <alice/bass/forms/maps_static_api.h>
#include <alice/bass/forms/urls_builder.h>

#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/scheduler/scheduler.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <library/cpp/neh/neh.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/timezone_conversion/convert.h>
#include <library/cpp/xml/document/xml-document.h>

#include <library/cpp/string_utils/quote/quote.h>
#include <util/generic/singleton.h>
#include <util/generic/noncopyable.h>

namespace NBASS {
namespace {

static constexpr ui16 CARD_MAP_IMAGE_WIDTH = 520;
static constexpr ui16 CARD_MAP_IMAGE_HEIGHT = 320;
static constexpr ui16 CARD_MAP_IMAGE_ZOOM = 10;
static constexpr ui16 CARD_MAP_USER_CENTERED_IMAGE_ZOOM = 12;

constexpr TStringBuf MAPS_INFO_EXPORT_PATH = "traffic/current/stat.xml";
constexpr TStringBuf TRAFFIC_FORECAST_PATH = "levels_prediction";

class TTrafficData: NNonCopyable::TNonCopyable {
public:
    static void Init(IGlobalContext& globalCtx) {
        Y_ENSURE(!Instance_);
        Instance_.Reset(new TTrafficData(globalCtx));
        globalCtx.Scheduler().Schedule([&globalCtx]() { return Update(globalCtx.GeobaseLookup()); });
    }

    static TTrafficData* Instance() {
        return Instance_.Get();
    }

    NSc::TValue Data() {
        with_lock (Lock) {
            return SavedData;
        }
    }

    bool IsExpired() const {
        return (TInstant::Now() - LastUpdate) > (RefreshPeriod * 2);
    }

    static TDuration Update(const NGeobase::TLookup& geoBase) {
        return Instance()->DoUpdate(geoBase);
    }

private:
    explicit TTrafficData(IGlobalContext& globalCtx)
        : RefreshPeriod(globalCtx.Config().CacheUpdatePeriod())
        , MapsInfoExportBackgroundSF(globalCtx.Sources().MapsInfoExportBackground, globalCtx.Config(),
                                     MAPS_INFO_EXPORT_PATH, SourcesRegistryDelegate)
        , TrafficForecastBackgroundSF(globalCtx.Sources().TrafficForecastBackground, globalCtx.Config(),
                                      TRAFFIC_FORECAST_PATH, SourcesRegistryDelegate)
        , Sensors(globalCtx.Counters().Sensors())
    {
    }

    TDuration DoUpdate(const NGeobase::TLookup& geoBase);

    bool IsCityForecastOld(const NSc::TValue& forecastCityJSON);

private:
    static THolder<TTrafficData> Instance_;
    TDuration RefreshPeriod;
    TDummySourcesRegistryDelegate SourcesRegistryDelegate;
    TSourceRequestFactory MapsInfoExportBackgroundSF;
    TSourceRequestFactory TrafficForecastBackgroundSF;
    TAdaptiveLock Lock;
    NSc::TValue SavedData;
    TInstant LastUpdate;
    NMonitoring::TMetricRegistry& Sensors;
};

THolder<TTrafficData> TTrafficData::Instance_;

TString GenerateStaticMapTrafficUri(TContext& ctx) {
    NMapsStaticApi::TImageUrlBuilder urlBuilder{ctx};
    const TContext::TSlot* where = ctx.GetSlot("where");
    if (IsSlotEmpty(where) && ctx.Meta().HasLocation()) {
        // focus map on the user location, if the user did not ask for a specific place
        urlBuilder.SetCenter(ctx.Meta().Location().Lon(), ctx.Meta().Location().Lat()).SetZoom(CARD_MAP_USER_CENTERED_IMAGE_ZOOM);
    } else {
        // if the user asked for a specific place, focus map on the city
        const TContext::TSlot* resolvedWhere = ctx.GetSlot("resolved_where");
        // todo: don't look for the geo for the second time - take it from the upstream code in the traffic.cpp
        const auto& geobase = ctx.GlobalCtx().GeobaseLookup();
        NGeobase::TRegion region = geobase.GetRegionById(resolvedWhere->Value["geoid"]);
        urlBuilder.SetCenter(region.GetLongitude(), region.GetLatitude()).SetZoom(CARD_MAP_IMAGE_ZOOM);
    }
    urlBuilder.SetSize(CARD_MAP_IMAGE_WIDTH, CARD_MAP_IMAGE_HEIGHT).Set("l", "map,trf,skl");
    return urlBuilder.Build();
}

bool TTrafficData::IsCityForecastOld(const NSc::TValue& forecastCityJSON) {
    return TInstant::Now() - TInstant::Seconds(forecastCityJSON["timestamp"]) > TDuration::Minutes(5);
}

TDuration TTrafficData::DoUpdate(const NGeobase::TLookup& geoBase) {
    // looking for current traffic level
    NHttpFetcher::TRequestPtr req = MapsInfoExportBackgroundSF.Request();
    auto resp = req->Fetch()->Wait();
    if (!resp || resp->IsError()) {
        LOG(DEBUG) << TStringBuf("Fetching traffic info error: ") << (resp ? resp->GetErrorText() : "no response") << Endl;
        return RefreshPeriod;
    }

    NSc::TValue newTrafficData;
    const NXml::TDocument xml(resp->Data, NXml::TDocument::String);
    NXml::TConstNode jamsData = xml.Root();
    for (NXml::TConstNode regionNode = jamsData.FirstChild("region"); !regionNode.IsNull(); regionNode = regionNode.NextSibling("region")) {
        NSc::TValue& regionData = newTrafficData[regionNode.Attr<TString>("id")];
        regionData["level"] = regionNode.FirstChild("level").Value<TString>();
        regionData["icon"] = regionNode.FirstChild("icon").Value<TString>();
        regionData["url"] = regionNode.FirstChild("url").Value<TString>();
        NSc::TValue& hintData = regionData["hint"];
        for (NXml::TConstNode hint = regionNode.FirstChild("hint"); !hint.IsNull(); hint = hint.NextSibling("hint")) {
            hintData[hint.Attr<TString>("lang")] = hint.Value<TString>();
        }
    }
    // looking for the forecasted traffic level
    req = TrafficForecastBackgroundSF.Request();
    resp = req->Fetch()->Wait();
    if (!resp || resp->IsError()) {
        LOG(DEBUG) << TStringBuf("Fetching traffic forecast error: ") << (resp ? resp->GetErrorText() : TStringBuf("no response")) << Endl;
        return RefreshPeriod;
    }

    const auto forecastJSON = NTraffic::NImpl::TransformTrafficResponseToOldApiVersion(
        NSc::TValue::FromJson(resp->Data), TInstant::Now().Seconds(), geoBase);
    if (forecastJSON.IsNull()) {
        LOG(DEBUG) << "Invalid format of traffic forecast" << Endl;
        Sensors.Rate(NMonitoring::TLabels{{"sensor", "traffic_forecast_parse_transform_error"}})->Inc();
        return RefreshPeriod;
    }
    NSc::TValue newForecastData;
    int hour;
    int score;
    bool isForecastOld = false;
    for (const auto& cityKey : forecastJSON.DictKeys()) {
        newForecastData[cityKey].SetArray();
        for (const auto& hourKey : forecastJSON[cityKey]["jams"].DictKeys()){
            NSc::TValue& hourData = newForecastData[cityKey].Push();
            if (TryFromString(hourKey, hour) && TryFromString(forecastJSON[cityKey]["jams"][hourKey]["from"], score)) {
                hourData["hour"] = hour;
                hourData["score"] = score;
            }
        }
        isForecastOld |= IsCityForecastOld(forecastJSON[cityKey]);
    }

    if (isForecastOld) {
        LOG(DEBUG) << TStringBuilder{} << "Traffic forecast resource " << TRAFFIC_FORECAST_PATH << " isn't updating for 5 minutes" << Endl;
        Sensors.Rate(NMonitoring::TLabels{{"sensor", "traffic_forecast_old_resource_error"}})->Inc();
    }

    if (newTrafficData.IsNull() || newForecastData.IsNull()) {
        LOG(DEBUG) << "Traffic data was not updated - at least one of the two sources was missing." << Endl;
        return RefreshPeriod;
    }

    NSc::TValue newData;
    newData["current"] = newTrafficData;
    newData["forecast"] = newForecastData;

    with_lock (Lock) {
        SavedData.Swap(newData);
    }
    LOG(DEBUG) << "Traffic data updated in cache" << Endl;

    LastUpdate = TInstant::Now();

    return RefreshPeriod;
}

} // end of the anonymous namespace


namespace NTraffic {

namespace NImpl {

NSc::TValue TransformTrafficResponseToOldApiVersion(const NSc::TValue& response, const ui64 timestamp,
                                                    const NGeobase::TLookup& geoBase) {
    NSc::TValue output{};
    const auto& input = response["levels"]["predictions"].GetArray();
    for (const auto& regionItem : input) {
        const auto regionId = regionItem["region"].GetIntNumber();
        NSc::TValue oldItem{};
        oldItem["timestamp"] = timestamp;

        for (const auto& prediction : regionItem["predictions"].GetArray()) {
            const auto level = ToString(prediction["level"].GetIntNumber());
            const auto timezone = NDatetime::GetTimeZone(geoBase.GetRegionById(regionId).GetTimezoneName());
            const auto time = TInstant::Seconds(prediction["timestamp"].GetIntNumber());
            const auto hours = NDatetime::ToCivilTime(time, timezone).ToString("%H");
            oldItem["jams"][hours]["from"] = level;
            oldItem["prediction_model"] = "new";
        }
        output[ToString(regionId)] = std::move(oldItem);
    }
    return output;
}

} // namespace NImpl

constexpr TStringBuf TRAFFIC_FORM_NAME = "personal_assistant.scenarios.show_traffic";
constexpr TStringBuf TRAFFIC_DETAILS_FORM_NAME = "personal_assistant.scenarios.show_traffic__details";
constexpr TStringBuf TRAFFIC_ELLIPSIS_FORM_NAME = "personal_assistant.scenarios.show_traffic__ellipsis";
constexpr TStringBuf COLLECT_MAIN_SCREEN_FORM_NAME = "alice.centaur.collect_main_screen.widgets.traffic";
constexpr TStringBuf SHOW_TRAFFIC_LAYER = "yandexnavi://traffic?traffic_on=1";


TResultValue GetTrafficInfo(TContext& ctx, NSc::TValue* trafficJson, NGeobase::TRegion& region) {
    TTrafficData* trafficCache = TTrafficData::Instance();
    if (trafficCache->IsExpired()) {
        return TError(
            TError::EType::SYSTEM,
            TStringBuilder() << TStringBuf("Traffic data is missed or too old")
        );
    }
    NSc::TValue trafficData(trafficCache->Data());

    const auto& geobase = ctx.GlobalCtx().GeobaseLookup();
    for (; NAlice::IsValidId(region.GetId()) && region.GetEType() >= NGeobase::ERegionType::CITY && trafficJson->IsNull();
           region = geobase.GetRegionById(region.GetParentId()))
    {
        TString geoIdStr = ToString(region.GetId());
        const NSc::TValue& trafficForGeo = trafficData["current"][geoIdStr];
        if (!trafficForGeo.IsNull()) {
            (*trafficJson)["level"] = trafficForGeo["level"];
            (*trafficJson)["icon"] = trafficForGeo["icon"];
            (*trafficJson)["hint"] = trafficForGeo["hint"][ctx.MetaLocale().Lang];
            // try to fill traffic forecast for this city - right into the form slot
            const NSc::TValue& trafficForecastForGeo = trafficData["forecast"][geoIdStr];
            if (!trafficForecastForGeo.IsNull()) {
                (*trafficJson)["forecast"] = trafficForecastForGeo;
            }
            // Fill additional fields
            NAlice::AddObsoleteCaseForms(geobase, region.GetId(), ctx.MetaLocale().Lang, trafficJson);
            (*trafficJson)["in_user_city"].SetBool(region.GetId() == ctx.UserRegion());
            break;
        }
    }
    return TResultValue();
}


TResultValue GetTrafficInfoForRegion(TContext& ctx, TRequestedGeo& geo, TContext::TSlot** slot, TString* naviUrl) {
    Y_ASSERT(geo.IsValidId());

    NSc::TValue response;
    NGeobase::TRegion region = geo.GetRegion();
    GetTrafficInfo(ctx, &response, region);

    NGeobase::TRegion resolvedGeo = response.IsNull() ? geo.GetRegion() : region;
    geo.ConvertTo(resolvedGeo.GetId());
    geo.CreateResolvedMeta(ctx, "resolved_where");

    if (resolvedGeo.GetEType() >= NGeobase::ERegionType::CITY) {
        // Always generate url for maps, even if no info about traffic level
        response["url"].SetString(GenerateMapsTrafficUri(ctx, resolvedGeo.GetId()));
        response["static_map_url"].SetString(GenerateStaticMapTrafficUri(ctx));
        if (naviUrl) {
            *naviUrl = response["url"];
        }
    }

    if (response.IsNull()) {
        return TError(TError::EType::NOTRAFFIC, "no traffic data for this geo");
    }

    *slot = ctx.CreateSlot("traffic_info", "traffic_info", true, std::move(response));
    return TResultValue();
}

} // namespace NTraffic

TResultValue TTrafficFormHandler::Do(TRequestHandler& r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::SHOW_TRAFFIC);
    if (NTraffic::TRAFFIC_DETAILS_FORM_NAME == r.Ctx().FormName()) {
        if (r.Ctx().ClientFeatures().SupportsNavigator()) {
            const TContext::TSlot* trafficInfo = r.Ctx().GetSlot("traffic_info");
            if (trafficInfo) {
                const NSc::TValue& trafficInfoValue = trafficInfo->Value;
                if (trafficInfoValue.Has("url")) {
                    AddShowTrafficNaviCommandBlocks(r.Ctx(), trafficInfoValue["url"].GetString());
                }
            }
        }
        return TResultValue();
    }

    TRequestedGeo geo(r.Ctx(), TStringBuf("where"));

    TResultValue err;

    if (geo.HasError()) {
        err = TError(TError::EType::NOGEOFOUND, "no geo found for slot");
    } else {
        TContext::TSlot* slotTrafficInfo = nullptr;
        TString naviUrl;
        err = NTraffic::GetTrafficInfoForRegion(r.Ctx(), geo, &slotTrafficInfo, &naviUrl);
        if (!err && slotTrafficInfo) {
            if (r.Ctx().ClientFeatures().SupportsOpenLink()) {
                r.Ctx().AddAttention(TStringBuf("supports_open_link"));
            }
            const bool shouldAddDivCard = r.Ctx().ClientFeatures().SupportsDivCardsRendering();

            if (r.Ctx().ClientFeatures().SupportsNavigator()) {
                if (!slotTrafficInfo->Value.Has("level")) {
                    AddShowTrafficNaviCommandBlocks(r.Ctx(), naviUrl);
                } else {
                    NSc::TValue payload;
                    if (!naviUrl.empty()) {
                        payload["commands"][0]["command_type"].SetString("open_uri");
                        payload["commands"][0]["data"]["uri"].SetString(naviUrl);
                        payload["commands"][1]["command_type"].SetString("open_uri");
                        payload["commands"][1]["data"]["uri"].SetString(NTraffic::SHOW_TRAFFIC_LAYER);
                    }
                    if (!shouldAddDivCard) {
                        r.Ctx().AddSuggest(TStringBuf("show_traffic__show_map"), std::move(payload));
                    }
                }
            } else {
                if (!shouldAddDivCard) {
                    r.Ctx().AddSuggest(TStringBuf("show_traffic__show_map"));
                }
            }

            if (shouldAddDivCard) {
                NSc::TValue divData;
                TString type;
                if (slotTrafficInfo->Value.Has("level")) {
                    int level = slotTrafficInfo->Value["level"].ForceIntNumber();
                    TString level_name;
                    if (level > 5) {
                        level_name = "traffic_red";
                    } else if (level > 3) {
                        level_name = "traffic_yellow";
                    } else {
                        level_name = "traffic_green";
                    }
                    type = "traffic_with_score";
                    if (const TAvatar* avatar = r.Ctx().Avatar(TStringBuf("traffic"), level_name)) {
                        divData["traffic_lights_image_url"] = avatar->Https;
                    }
                } else {
                    type = "traffic_without_score";
                }
                r.Ctx().AddDivCardBlock(type, std::move(divData));
                r.Ctx().AddAttention(TStringBuf("traffic_cards"));
            }
        }
    }

    if (err) {
        r.Ctx().AddErrorBlock(*err);
    }

    // two default suggests which must be there always!
    r.Ctx().AddSearchSuggest();
    r.Ctx().AddOnboardingSuggest();

    return TResultValue();
}

void TTrafficFormHandler::Register(THandlersMap* handlers, IGlobalContext& globalCtx) {
    TTrafficData::Init(globalCtx);
    auto cbTrafficForm = []() {
        return MakeHolder<TTrafficFormHandler>();
    };
    handlers->RegisterFormHandler(NTraffic::TRAFFIC_FORM_NAME, cbTrafficForm);
    handlers->RegisterFormHandler(NTraffic::TRAFFIC_DETAILS_FORM_NAME, cbTrafficForm);
    handlers->RegisterFormHandler(NTraffic::TRAFFIC_ELLIPSIS_FORM_NAME, cbTrafficForm);
    handlers->RegisterFormHandler(NTraffic::COLLECT_MAIN_SCREEN_FORM_NAME, cbTrafficForm);
}

void TTrafficFormHandler::AddShowTrafficNaviCommandBlocks(TContext& ctx, TStringBuf naviUrl) {
    NSc::TValue showMapIntent;
    showMapIntent["uri"].SetString(naviUrl);
    ctx.AddCommand<TNavigatorShowPointOnMapDirective>(TStringBuf("open_uri"), showMapIntent);
    NSc::TValue showTrafficIntent;
    showTrafficIntent["uri"].SetString(NTraffic::SHOW_TRAFFIC_LAYER);
    ctx.AddCommand<TNavigatorLayerTrafficDirective>(TStringBuf("open_uri"), showTrafficIntent);
}

} // namespace NBASS

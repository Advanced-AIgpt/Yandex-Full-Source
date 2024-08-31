#include "poi.h"

#include "geoaddr.h"
#include "geocoder.h"
#include "maps_static_api.h"
#include "remember_address.h"
#include "route.h"
#include "route_tools.h"
#include "special_location.h"
#include "taxi.h"
#include "urls_builder.h"
#include "afisha/afisha_proxy.h"
#include "common/saved_address.h"
#include "navigator/map_search_intent.h"
#include "navigator/show_on_map_intent.h"
#include "navigator/user_bookmarks.h"
#include "taxi/handler.h"

#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/common/directives.h>
#include <alice/bass/forms/urls_builder.h>

#include <library/cpp/neh/neh.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/threading/future/async.h>
#include <library/cpp/timezone_conversion/convert.h>

#include <search/session/compression/report.h>

#include <util/datetime/base.h>
#include <util/string/printf.h>

namespace NBASS {

static constexpr TStringBuf LAST_FOUND_POI = "last_found_poi";
static constexpr TStringBuf FIND_POI = "personal_assistant.scenarios.find_poi";
static constexpr TStringBuf FIND_POI_NEXT = "personal_assistant.scenarios.find_poi__scroll__next";
static constexpr TStringBuf FIND_POI_PREV = "personal_assistant.scenarios.find_poi__scroll__prev";
static constexpr TStringBuf FIND_POI_BY_INDEX = "personal_assistant.scenarios.find_poi__scroll__by_index";
static constexpr TStringBuf FIND_POI_ELLIPSIS = "personal_assistant.scenarios.find_poi__ellipsis";
static constexpr TStringBuf SORT_TYPE_DISTANCE = "distance";
static constexpr int PLACE_EVENTS_LIMIT = 10;
static constexpr int FIND_POI_GALLERY_LIMIT = 10;

static constexpr ui16 GALLERY_MAP_IMAGE_WIDTH = 260;
static constexpr ui16 GALLERY_MAP_IMAGE_HEIGHT = 200;
static constexpr ui16 GALLERY_WIDE_MAP_IMAGE_WIDTH = 520;
static constexpr ui16 GALLERY_WIDE_MAP_IMAGE_HEIGHT = 200;
static constexpr ui16 GALLERY_MAP_IMAGE_ZOOM = 16;


namespace {

NSc::TValue GetPoiIcons(TContext& context) {
    static const char* poiIconsList[] = {
        "Fill", "Half", "None", // rating stars
        "Net", // for site
        "NetV2",
        "Location S Colored", // for distance
        "Telephone",
        "TelephoneV2",
        "Route",
        "RouteV2",
        "Taxi",
        "TaxiV2"
    };

    NSc::TValue icons;
    for (const auto iconName : poiIconsList) {
        const TAvatar* avatar = context.Avatar(TStringBuf("poi"), iconName);
        if (avatar) {
            icons[iconName] = avatar->Https;
        }
    }

    return icons;
}

NSc::TValue GetPoiSubwayIcons(TContext& context) {
    static const TVector<std::pair<TString,TString>> poiSubwayIconsMap = {
        //{"100000006", ""},                // Московская монорельсовая транспортная система
        {"100000023", "SubwayRed"},         // Первая линия
        {"100000028", "SubwayRed"},         // Первая линия
        {"100000099", "SubwayRed"},         // Сокольническая линия
        {"100000033", "SubwayBlue"},        // Московская линия
        {"100000038", "SubwayRed"},         // Автозаводская линия
        {"100000046", "SubwayGreen"},       // Дзержинская линия
        {"100000051", "SubwayRed"},         // Ленинская линия
        {"100000059", "SubwayCyanGray"},    // Бутовская линия
        {"100000064", "SubwayLightGreen"},  // Люблинско-Дмитровская линия
        {"100000069", "SubwayGreen"},       // Замоскворецкая линия
        {"100000073", "SubwayTurquoise"},   // Каховская линия
        {"100000078", "SubwayGray"},        // Серпуховско-Тимирязевская линия
        {"100000083", "SubwayOrange"},      // Калужско-Рижская линия
        {"100000088", "SubwayRing"},        // Кольцевая линия
        {"100000107", "SubwayYellow"},      // Калининско-Солнцевская линия
        {"100000114", "SubwayPurple"},      // Таганско-Краснопресненская линия
        {"100000122", "SubwayBlue"},        // Арбатско-Покровская линия
        {"100000127", "SubwayCyan"},        // Филёвская линия
        {"100000245", "SubwayRed"},         // Центральная линия
        {"100000256", "SubwayBlue"},        // Автозаводская линия
        {"100000263", "SubwayBlue"},        // Сормовско-Мещерская линия
        {"100000267", "SubwayGreen"},       // Первая линия
        {"100000272", "SubwayRed"},         // 1 линия
        {"100000277", "SubwayCyan"},        // 2 линия
        {"100000283", "SubwayPurple"},      // 5 линия
        {"100000287", "SubwayGreen"},       // 3 линия
        {"100000295", "SubwayOrange"}       // 4 линия
    };

    NSc::TValue icons;
    for (const auto& iconIdName : poiSubwayIconsMap) {
        const TAvatar* avatar = context.Avatar(TStringBuf("poi"), iconIdName.second);
        if (avatar) {
            icons[iconIdName.first] = avatar->Https;
        }
    }

    return icons;
}

TMaybe<TString> TryAddressByUserBookmark(TContext& ctx, TStringBuf query) {
    if (const TMaybe<TUserBookmark> pos{ctx.GetUserBookmarksHelper()->GetUserBookmark(query)}) {
        NSc::TValue resolvedLocation;
        THolder<TGeoObjectResolver> resolver = MakeHolder<TGeoObjectResolver>(ctx, TGeoPosition(pos->Lat(), pos->Lon()));

        resolver->WaitAndParseResponse(&resolvedLocation);
        if (TString addressLine{resolvedLocation["address_line"].ForceString()}) {
            return std::move(addressLine);
        }
    }

    return Nothing();
}

void PushAddressText(TContext& ctx, TStringBuf query, TStringBuilder* out) {
    Y_ASSERT(out);

    TMaybe<TString> text;

    if (ctx.ClientFeatures().SupportsNavigator()) {
        text = TryAddressByUserBookmark(ctx, query);
    }

    if (text) {
        *out << *text;
    } else {
        *out << query;
    }
}

TString GenerateMapImageUrl(TContext& ctx, const NSc::TValue& foundPoi, bool wide) {
    NMapsStaticApi::TImageUrlBuilder urlBuilder{ctx};

    const NSc::TValue& location = foundPoi["location"];
    if (location.IsNull() || !location.Has("lon") || !location.Has("lat")) {
        return TString();
    }

    float lon = location["lon"];
    float lat = location["lat"];

    if (wide) {
        urlBuilder.SetCenter(lon, lat).SetZoom(GALLERY_MAP_IMAGE_ZOOM).SetSize(GALLERY_WIDE_MAP_IMAGE_WIDTH, GALLERY_WIDE_MAP_IMAGE_HEIGHT).AddPoint(lon, lat, "pm2rdl");
    } else {
        urlBuilder.SetCenter(lon, lat).SetZoom(GALLERY_MAP_IMAGE_ZOOM).SetSize(GALLERY_MAP_IMAGE_WIDTH, GALLERY_MAP_IMAGE_HEIGHT).AddPoint(lon, lat, "pm2rdl");
    }
    return urlBuilder.Build();
}

NSc::TValue GetPoiDetailsFormUpdate(const NSc::TValue& foundPoi) {
    NSc::TValue formUpdate;
    formUpdate["name"] = FIND_POI;
    TContext::TSlot objectIdSlot("object_id", "string");
    objectIdSlot.Value = foundPoi["object_id"].GetString();;
    formUpdate["slots"].Push(objectIdSlot.ToJson());

    return formUpdate;
}

TResultValue UpdatePoiData(TContext& ctx, const NSc::TValue& foundPoi, NSc::TValue& poiData, bool extended = false) {
    if (poiData.IsNull()) {
        return TError(TError::EType::NOGEOFOUND,
                      TStringBuilder() << TStringBuf("No organization found for id : ") << foundPoi["object_id"].GetString()
        );
    }

    static const TStringBuf fieldsToCopy[] = {
        "object_uri", "object_catalog_uri", "object_catalog_photos_uri", "object_catalog_reviews_uri", "phone_uri", "geo_uri", "url", "subtitles", "categories", "nearby_stops_meta_data", "reviews_keyphrases"
    };

    for (const auto field: fieldsToCopy) {
        const NSc::TValue& found = foundPoi[field];
        if (!found.IsNull()) {
           poiData[field] = found;
        }
    }

    if (poiData.Has("url")) {
        poiData["url"] = AddUtmReferrer(ctx.MetaClientInfo(), poiData["url"]);
    }

    if (ctx.Meta().HasLocation() && foundPoi.Has("location")) {
        NSc::TValue fromPoint;
        NSc::TValue& userLocation = fromPoint["location"];
        userLocation["lat"] = ctx.Meta().Location().Lat();
        userLocation["lon"] = ctx.Meta().Location().Lon();

        poiData["route_uri"] = GenerateRouteUri(ctx, fromPoint, foundPoi);

        if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_TAXI_NEW)) {
            NSc::TValue formUpdate = NTaxi::THandler::GetFormUpdate(foundPoi);
            poiData["taxi_form_update"] = formUpdate;
        } else {
            poiData["taxi_uri"] = GenerateTaxiUri(ctx, fromPoint, foundPoi);
        }
    }

    if (foundPoi.Has("location")) {
        poiData["map_image_url"] = GenerateMapImageUrl(ctx, foundPoi, /* wide */ false);
        poiData["wide_map_image_url"] = GenerateMapImageUrl(ctx, foundPoi, /* wide */ true);

        if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_FIND_POI_GALLERY_OPEN_SHOW_ROUTE)) {
            NSc::TValue formUpdate;
            TRouteFormHandler::GetFormUpdate(foundPoi, formUpdate);
            poiData["route_form_update"] = formUpdate;
        }
    }

    poiData["icons"] = GetPoiIcons(ctx);

    if (extended) {
        poiData["subway_icons"] = GetPoiSubwayIcons(ctx);
    }

    if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_FIND_POI_ONE)) {
        poiData["details_form_update"] = GetPoiDetailsFormUpdate(foundPoi);
    }

    return TResultValue();
}

void StripPoiData(NSc::TValue& poiData) {
    static const TStringBuf fieldsToStrip[] = {
        "Address", "Categories", "Chains", "InternalCompanyInfo", "Links", "Phones", "Properties"
    };

    if (poiData.Has("company_meta_data")) {
        for (const auto field: fieldsToStrip) {
            if (poiData["company_meta_data"].Has(field)) {
                poiData["company_meta_data"].Delete(field);
            }
        }
    }
}

TResultValue PreparePoiData(TContext& ctx, const NSc::TValue& foundPoi, NSc::TValue& poiData, bool extended) {
    TStringBuf objId = foundPoi["object_id"].GetString();

    TGeoObjectResolver resolver(ctx, objId, extended);
    if (TResultValue geoError = resolver.WaitAndParseOrganizationResponse(&poiData)) {
        return geoError;
    }

    if (TResultValue err = UpdatePoiData(ctx, foundPoi, poiData, extended)) {
        return err;
    }

    return TResultValue();
}

TResultValue PreparePoiData(TContext& ctx, const TStringBuf searchText, const TVector<NSc::TValue>& foundPoiResults, NSc::TValue& poiData) {
    TVector<TStringBuf> objIds;
    for (auto& foundPoi: foundPoiResults) {
        objIds.push_back(foundPoi["object_id"].GetString());
    }

    NSc::TValue allPoiData;
    TGeoObjectResolver resolver(ctx, searchText, objIds);
    if (TResultValue geoError = resolver.WaitAndParseMultiOrganizationResponse(&allPoiData)) {
        return geoError;
    }

    poiData.Clear();
    for (size_t i = 0; i < foundPoiResults.size(); ++i) {
        if (!UpdatePoiData(ctx, foundPoiResults[i], allPoiData[i])) {
            StripPoiData(allPoiData[i]);
            poiData.Push(allPoiData[i]);
        }
    }

    return TResultValue();
}

} // anon namespace

void TPoiFormHandler::PushSuggestBlock(TContext& ctx, const TStringBuf suggestType) const {
    ctx.AddSuggest(suggestType);
}

void TPoiFormHandler::PushEventsBlock(TContext& ctx, const NSc::TValue& foundPoi) const {
    if (!ctx.ClientFeatures().SupportsDivCards()) {
        return;
    }
    const NSc::TValue& placeId = foundPoi.TrySelect("references/afisha");
    if (placeId.IsNull()) {
        return;
    }
    NAfisha::TAfishaProxy proxy = NAfisha::TAfishaProxy(ctx);
    TMaybe<NSc::TValue> result = proxy.GetPlaceRepertory(placeId, PLACE_EVENTS_LIMIT);
    if (!result) {
        return;
    }

    NSc::TValue events;
    for (const NSc::TValue& item : (*result)["data"]["placeRepertory"]["items"].GetArray()) {
        events.Push(item);
    }
    if (events.IsNull() || events.GetArray().empty()) {
        return;
    }

    NSc::TValue answer;
    answer["place"] = (*result)["data"]["place"];
    answer["place"]["id"] = placeId;
    answer["events"] = events;

    ctx.AddDivCardBlock("found_poi_events", answer);
}

TResultValue TPoiFormHandler::PushDivCardBlock(TContext& ctx, const NSc::TValue& poiData) const {
    if (!ctx.ClientFeatures().SupportsDivCards()) {
        return TResultValue();
    }

    ctx.AddDivCardBlock("found_poi", poiData);

    return TResultValue();
}

TResultValue TPoiFormHandler::PushGalleryDivCardBlock(TContext &ctx, const TStringBuf searchText, const NSc::TValue& poiData) const
{
    if (!ctx.ClientFeatures().SupportsDivCards()) {
        return TResultValue();
    }

    NSc::TValue data;
    data["serp_uri"] = GenerateSearchUri(&ctx, searchText, TCgiParameters());
    data["poi_data"] = poiData;
    ctx.AddDivCardBlock("found_poi_gallery", data);
    ctx.AddAttention("use_v2_button_icons");

    return TResultValue();
}

TResultValue TPoiFormHandler::PushNewDivCardBlock(TContext& ctx, const NSc::TValue& poiData) const {
    if (!ctx.ClientFeatures().SupportsDivCards()) {
        return TResultValue();
    }

    NSc::TValue photosData;

    static const TStringBuf fieldsToCopy[] = {
        "map_image_url", "wide_map_image_url", "photos", "geo_uri", "route_uri", "route_form_update", "object_catalog_uri", "object_catalog_photos_uri"
    };

    for (const auto field: fieldsToCopy) {
        photosData[field] = poiData[field];
    }

    ctx.AddDivCardBlock("found_poi_one_photo_gallery", photosData);
    ctx.AddDivCardBlock("found_poi_one", poiData);

    return TResultValue();
}

namespace {

void FillLastFoundPoiSlot(TContext& ctx, NSc::TValue* foundPoi) {
    NGeobase::TId userGeoId = ctx.UserRegion();
    if (!foundPoi->IsNull()) {
        NAlice::FillInUserCity(userGeoId, foundPoi);
        ctx.CreateSlot(LAST_FOUND_POI, TGeoObjectResolver::GeoObjectType(*foundPoi), true, *foundPoi);
    } else {
        // Create slot "last_found_poi" with null value
        ctx.CreateSlot(LAST_FOUND_POI, "poi", true, *foundPoi);
    }
}

} // namespace anonymous

void TPoiFormHandler::AddPoiSuggests(TContext& ctx, const NSc::TValue& foundPoi)
{
    if (!(ctx.MetaClientInfo().IsSmartSpeaker() || ctx.MetaClientInfo().IsElariWatch())) {
        if (foundPoi.Has("phone")) {
            PushSuggestBlock(ctx, TStringBuf("find_poi__call"));
        }
        if (foundPoi.Has("url")) {
            PushSuggestBlock(ctx, TStringBuf("find_poi__open_site"));
        }
        if (foundPoi.Has("geo_uri")) {
            PushSuggestBlock(ctx, TStringBuf("find_poi__show_on_map"));
        }
    }

    const auto& meta = ctx.Meta();
    if (meta.HasLocation() && foundPoi.Has("location")) {
        NSc::TValue routeInfo;
        NSc::TValue geoFrom;
        geoFrom["type"] = TStringBuf("geo");
        geoFrom["location"]["lon"] = meta.Location().Lon();
        geoFrom["location"]["lat"] = meta.Location().Lat();
        GetRouteInfo(ctx, geoFrom, foundPoi, &routeInfo);

        if (!routeInfo.IsNull()) {
            if (routeInfo.Has("car")) {
                if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_TAXI_NEW)) {
                    NTaxi::THandler::AddTaxiSuggest(ctx, "", LAST_FOUND_POI);
                } else {
                    TTaxiOrderHandler::AddTaxiSuggest(ctx, "", LAST_FOUND_POI);
                }
                PushSuggestBlock(ctx, TStringBuf("find_poi__go_by_car"));
            }
            if (routeInfo.Has("public_transport")) {
                PushSuggestBlock(ctx, TStringBuf("find_poi__go_by_public_transport"));
            }
            if (routeInfo.Has("pedestrian")) {
                PushSuggestBlock(ctx, TStringBuf("find_poi__go_by_foot"));
            }
        }
    }
}

TResultValue TPoiFormHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();

    TContext::TSlot* slotObjectId = ctx.GetSlot("object_id");
    TContext::TSlot* slotWhat = ctx.GetSlot("what");
    TContext::TSlot* slotWhere = ctx.GetSlot("where");
    TContext::TSlot* slotOpen = ctx.GetSlot("open");

    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::FIND_POI);

    if (!IsSlotEmpty(slotObjectId)) {
        return DoFindPoi(ctx, slotObjectId->Value.GetString());
    }

    if (IsSlotEmpty(slotWhat)) {
        if (IsSlotEmpty(slotWhere)) {
            ctx.CreateSlot("what", "string", false);
            return TResultValue();
        }
        ctx.CreateSlot(TStringBuf("what"), slotWhere->Type, slotWhere->Optional, slotWhere->Value, slotWhere->SourceText);
        slotWhere->Reset();
    }

    if (r.Ctx().ClientFeatures().SupportsNavigator()) {
        TSpecialLocation locationName = TSpecialLocation::GetNamedLocation(slotWhat);
        if (locationName.IsUserAddress()) {
            const TSavedAddress userAddress = r.Ctx().GetSavedAddress(locationName, slotWhat->SourceText);
            if (userAddress.IsValid()) {
                NSc::TValue foundPoi = SavedAddressToGeo(r.Ctx(), userAddress);
                FillLastFoundPoiSlot(r.Ctx(), &foundPoi);

                TGeoPosition pos = TGeoPosition(userAddress.Latitude(), userAddress.Longitude());
                TShowOnMapIntent navigatorIntent(ctx, pos);
                return navigatorIntent.Do();
            } else {
                // User asks to show his home or work, but we don't know the address
                // switch to "remember_named_location" form
                return TSaveAddressHandler::SetAsResponse(r.Ctx(), locationName);
            }
        }

        if (!TSpecialLocation::IsSpecialLocation(slotWhere)) {
            TStringBuilder bookmarkName;
            bookmarkName << slotWhat->Value;
            if (!IsSlotEmpty(slotWhere)) {
                bookmarkName << " " << slotWhere->Value;
            }

            if (TMaybe<TUserBookmark> userBookmark{r.Ctx().GetUserBookmarksHelper()->GetUserBookmark(bookmarkName)}) {
                NSc::TValue foundPoi = userBookmark->ToResolvedGeo(ctx);
                FillLastFoundPoiSlot(r.Ctx(), &foundPoi);

                TGeoPosition pos = TGeoPosition(userBookmark->Lat(), userBookmark->Lon());
                TShowOnMapIntent navigatorIntent(ctx, pos, userBookmark->Name());
                return navigatorIntent.Do();
            }
        }
    }

    TStringBuilder searchText;
    searchText << slotWhat->Value.GetString();

    TMaybe<TGeoPosition> searchPos;
    // Всегда передаём локацию пользователя в ll, даже если пользователь указал where
    // (иначе может быть странное ранжирование, например: "ул. ленина" в другом городе)
    const auto& meta = r.Ctx().Meta();
    if (meta.HasLocation()) {
        searchPos = InitGeoPositionFromLocation(meta.Location());
    }

    TStringBuf sortBy;

    if (IsSlotEmpty(slotWhere)) {
        if (!searchPos) {
            // Ask to fill "where"
            // Create "where" slot, if it was missed
            r.Ctx().CreateSlot("where", "string", false);
            return TResultValue();
        }
    } else {
        TSpecialLocation locationName = TSpecialLocation::GetNamedLocation(slotWhere);
        if (!locationName.IsError()) {
            if (locationName.IsNearLocation()) {
                if (!searchPos) {
                    // User asks to find something near his location, but we don't know his location
                    r.Ctx().CreateSlot("where", "string", false);
                    r.Ctx().AddAttention("no_user_location", NSc::Null());
                    return TResultValue();
                }

                if ((r.Ctx().ClientFeatures().SupportsNavigator()) &&
                    locationName == TSpecialLocation::EType::NEAREST)
                {
                    return TRouteFormHandler::SetAsResponse(r.Ctx(), "what", "where");
                }
            } else if (locationName.IsUserAddress()) {
                const TSavedAddress userAddress = r.Ctx().GetSavedAddress(locationName, slotWhere->SourceText);
                if (userAddress.IsValid()) {
                    searchPos = TGeoPosition(userAddress.Latitude(), userAddress.Longitude());
                } else {
                    // User asks to find something near his home or work, but we don't know the address
                    // switch to "remember_named_location" form
                    return TSaveAddressHandler::SetAsResponse(r.Ctx(), locationName);
                }
            } else if (locationName.IsCurrentGeo()) {
                TResultValue geoError;
                NGeobase::TId regionId = locationName.GetGeo(r.Ctx(), &geoError);
                if (geoError) {
                    r.Ctx().CreateSlot("where", "string", false);
                    r.Ctx().AddAttention("no_user_location", NSc::Null());
                    return TResultValue();
                }
                const auto& geobase = ctx.GlobalCtx().GeobaseLookup();
                NGeobase::TLinguistics names = geobase.GetLinguistics(regionId, ctx.MetaLocale().Lang);
                if (searchText) {
                    searchText << ' ';
                }
                if (!names.Preposition.empty()) {
                    searchText << names.Preposition << ' ';
                }
                searchText << names.PrepositionalCase;
            }
            sortBy = SORT_TYPE_DISTANCE;
        } else {
            if (searchText) {
                searchText << ' ';
            }
            PushAddressText(ctx, slotWhere->Value.GetString(), &searchText);
        }
    }

    // Multiple business filters are supported, but currenlty only a single filter at a time is used.
    TVector<TStringBuf> businessFilters = {};

    if (!IsSlotEmpty(slotOpen)) {
        TStringBuf open_value = slotOpen->Value.GetString();
        if (open_value == "open_now") {
            businessFilters.push_back("open_now:1");
        } else if (open_value == "open_24h") {
            businessFilters.push_back("open_24h:1");
        }
    }

    if (r.Ctx().ClientFeatures().SupportsNavigator()) {
        /**
         * XXX:
         * The block below is just a quick fix that fills last_found_poi slot.
         * In fact, inside TMapSearchNavigatorIntent there is a command that forces a client to wrap GeoResolver again.
         * Actually, there isn't any guarantee that both GeoResolver answers  are consistent!!!
         *
         * Probably, we need to force POI selection from BASS or to obtain last_found_poi from device state.
         */
        TContext::TSlot* slotLastFoundPoi = ctx.GetSlot(LAST_FOUND_POI);
        if (IsSlotEmpty(slotLastFoundPoi)) {
            TGeoObjectResolver resolver(r.Ctx(), searchText, searchPos, "geo,biz", sortBy, businessFilters);
            NSc::TValue foundPoi;
            TResultValue err = resolver.WaitAndParseResponse(&foundPoi);
            if (err) {
                return err;
            }
            FillLastFoundPoiSlot(r.Ctx(), &foundPoi);
        }

        TMapSearchNavigatorIntent navigatorIntentHandler(r.Ctx(), searchText, searchPos);
        return navigatorIntentHandler.Do();
    } else {
        return DoFindPoi(r.Ctx(), searchText, searchPos, sortBy, businessFilters);
    }
}

TResultValue TPoiFormHandler::DoFindPoi(TContext& ctx, TString searchText, TMaybe<TGeoPosition> searchPos, TStringBuf sortBy, TVector<TStringBuf>& businessFilters) {
    TContext::TSlot* slotResultIndex = ctx.GetOrCreateSlot("result_index", "num");
    if (IsSlotEmpty(slotResultIndex) || slotResultIndex->Value.GetIntNumber() < 1) {
        slotResultIndex->Value.SetIntNumber(1);
    }

    if (ctx.FormName() == FIND_POI || ctx.FormName() == FIND_POI_ELLIPSIS) {
        // We initialize new search, so force result_index to 1
        slotResultIndex->Value.SetIntNumber(1);
    } else if (ctx.FormName() == FIND_POI_NEXT) {
        slotResultIndex->Value.SetIntNumber(slotResultIndex->Value.GetIntNumber() + 1);
    } else if (ctx.FormName() == FIND_POI_PREV) {
        slotResultIndex->Value.SetIntNumber(Max(static_cast<i64>(1), slotResultIndex->Value.GetIntNumber() - 1));
    }

    TGeoObjectResolver resolver(ctx, searchText, searchPos, "geo,biz,objects",
            sortBy, businessFilters, slotResultIndex->Value.GetIntNumber(),
            /* includeExperimentalMeta */ true);

    NSc::TValue resolvedWhere;
    if (!ctx.HasExpFlag(EXP_FIND_POI_GALLERY) || !ctx.ClientFeatures().SupportsDivCards()) {
        NSc::TValue foundPoi, foundNextPoi;

        TResultValue err = resolver.WaitAndParseResponse(&foundPoi, &foundNextPoi, &resolvedWhere);
        if (err) {
            return err;
        }


        FillLastFoundPoiSlot(ctx, &foundPoi);

        if (!foundPoi.IsNull()) {
            if (!foundNextPoi.IsNull()) {
                PushSuggestBlock(ctx, TStringBuf("find_poi__next"));
            }

            if (sortBy != SORT_TYPE_DISTANCE) {
                PushSuggestBlock(ctx, TStringBuf("find_poi__near"));
            }

            if (!(ctx.MetaClientInfo().IsSmartSpeaker() || ctx.MetaClientInfo().IsElariWatch())) {
                if (foundPoi.Has("object_id")) {
                    NSc::TValue poiData;
                    if (!ctx.HasExpFlag(EXPERIMENTAL_FLAG_FIND_POI_ONE)) {

                        if (TResultValue orgSearchError = PreparePoiData(ctx, foundPoi, poiData, /*extended*/ false)) {
                            return orgSearchError;
                        }
                        PushSuggestBlock(ctx, TStringBuf("find_poi__details"));
                        PushDivCardBlock(ctx, poiData);
                    } else {
                        if (TResultValue orgSearchError = PreparePoiData(ctx, foundPoi, poiData, /*extended*/ true)) {
                            return orgSearchError;
                        }
                        ctx.AddTextCardBlock("find_poi__what_i_found_one");
                        ctx.CreateSlot("gallery_results_count", TStringBuf("num"), /* optional */ true, NSc::TValue{static_cast<unsigned long long>(1)});
                        PushNewDivCardBlock(ctx, poiData);
                    }
                }
            }

            if (!ctx.HasExpFlag(EXPERIMENTAL_FLAG_FIND_POI_ONE)) {
                AddPoiSuggests(ctx, foundPoi);
            }

            PushEventsBlock(ctx, foundPoi);
        }
    } else {
        TVector<NSc::TValue> foundPoiResults;
        NSc::TValue poiData;
        TResultValue err = resolver.WaitAndParseResponse(foundPoiResults, FIND_POI_GALLERY_LIMIT, &resolvedWhere);
        if (err) {
            return err;
        }

        PreparePoiData(ctx, searchText, foundPoiResults, poiData);

        size_t galleryResultsCount = poiData.ArraySize();

        if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_FIND_POI_ONE) && galleryResultsCount == 1) {
            Y_ASSERT(!foundPoiResults.empty());
            FillLastFoundPoiSlot(ctx, &foundPoiResults.front());
            PreparePoiData(ctx, foundPoiResults.front(), poiData, /*extended*/ true);
            ctx.AddTextCardBlock("find_poi__what_i_found_one");
            PushNewDivCardBlock(ctx, poiData);
            PushEventsBlock(ctx, foundPoiResults.front());
        } else if (galleryResultsCount > 0) {
            ctx.AddTextCardBlock("find_poi__what_i_found");
            PushGalleryDivCardBlock(ctx, searchText, poiData);
        } else if (foundPoiResults.size() > 0) {
            // we didn't find any organizations, but have some poi
            FillLastFoundPoiSlot(ctx, &foundPoiResults.front());

            if (foundPoiResults.size() > 1) {
                PushSuggestBlock(ctx, TStringBuf("find_poi__next"));
            }

            if (sortBy != SORT_TYPE_DISTANCE) {
                PushSuggestBlock(ctx, TStringBuf("find_poi__near"));
            }

            AddPoiSuggests(ctx, foundPoiResults.front());
        }
        ctx.CreateSlot("gallery_results_count", TStringBuf("num"), /* optional */ true, NSc::TValue{static_cast<unsigned long long>(galleryResultsCount)});
    }

    ctx.CreateSlot("resolved_where", "geo", true, resolvedWhere);

    ctx.AddSearchSuggest();
    ctx.AddOnboardingSuggest();

    if (ctx.ClientFeatures().SupportsCloudUi() && ctx.HasExpFlag(ENABLE_OPEN_LINK_AND_CLOUD_UI)) {
        TryAddOpenUriDirective(ctx, GenerateSearchUri(&ctx, searchText, TCgiParameters()));
        LOG(INFO) << "Adding open_uri directive";
    }

    return TResultValue();
}

TResultValue TPoiFormHandler::DoFindPoi(TContext &ctx, TStringBuf objectId) {
    NSc::TValue foundPoi, resolvedWhere, poiData;
    TGeoObjectResolver resolver(ctx, objectId, /*extendedInfo*/ false, /*proto*/ true);

    TResultValue err = resolver.WaitAndParseResponse(&foundPoi, nullptr, &resolvedWhere);
    if (err) {
        return err;
    }

    FillLastFoundPoiSlot(ctx, &foundPoi);

    if (TResultValue orgSearchError = PreparePoiData(ctx, foundPoi, poiData, /*extended*/ true)) {
        return orgSearchError;
    }

    ctx.AddSearchSuggest();
    ctx.AddOnboardingSuggest();

    ctx.AddTextCardBlock("find_poi__what_i_found_one");
    PushNewDivCardBlock(ctx, poiData);
    PushEventsBlock(ctx, foundPoi);

    return TResultValue();
}

// static
TResultValue TPoiFormHandler::SetAsResponse(TContext& ctx, TStringBuf what) {
    TIntrusivePtr<TContext> newContext = ctx.SetResponseForm(FIND_POI, false /* setCurrentFormAsCallback */);
    Y_ENSURE(newContext);
    newContext->CreateSlot("what", "string", true, what);
    newContext->CopySlotFrom(ctx, LAST_FOUND_POI);
    return TResultValue();
}

IParallelHandler::TTryResult TPoiFormHandler::TryToHandle(TContext& ctx, TStringBuf query) {
    if (ctx.ClientFeatures().SupportsNavigator()) {
        if (TMaybe<TUserBookmark> bookmark{ctx.GetUserBookmarksHelper()->GetUserBookmark(query)}; bookmark.Defined()) {
            SetAsResponse(ctx, query);
            if (const auto error = ctx.RunResponseFormHandler()) {
                return *error;
            }
            return IParallelHandler::ETryResult::Success;
        }

        TMaybe<TGeoPosition> searchPos = InitGeoPositionFromLocation(ctx.Meta().Location());
        TGeoObjectResolver resolver(ctx, query, searchPos, "geo,biz", "");

        NSc::TValue foundPoi;
        resolver.WaitAndParseResponse(&foundPoi);

        if (foundPoi.IsNull()) {
            return IParallelHandler::ETryResult::NonSuitable;
        }

        FillLastFoundPoiSlot(ctx, &foundPoi);
    }

    SetAsResponse(ctx, query);
    if (const auto error = ctx.RunResponseFormHandler()) {
        return *error;
    }
    return IParallelHandler::ETryResult::Success;
}

void TPoiFormHandler::Register(THandlersMap* handlers) {
    auto cb = []() {
        return MakeHolder<TPoiFormHandler>();
    };
    handlers->emplace(FIND_POI, cb);
    handlers->emplace(FIND_POI_NEXT, cb);
    handlers->emplace(FIND_POI_PREV, cb);
    handlers->emplace(FIND_POI_BY_INDEX, cb);
    handlers->emplace(FIND_POI_ELLIPSIS, cb);
}

} // namespace NBASS

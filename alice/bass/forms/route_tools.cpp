#include "route_tools.h"

#include "geodb.h"
#include "remember_address.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/geoaddr.h>
#include <alice/bass/forms/navigator/bookmarks_matcher.h>
#include <alice/bass/forms/navigator/show_on_map_intent.h>
#include <alice/bass/forms/navigator/user_bookmarks.h>
#include <alice/bass/forms/special_location.h>
#include <alice/bass/forms/urls_builder.h>

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <yandex/maps/proto/common2/response.pb.h>
#include <yandex/maps/proto/driving/summary.pb.h>
#include <yandex/maps/proto/driving/via_point.pb.h>
#include <yandex/maps/proto/masstransit/summary.pb.h>

#include <util/generic/strbuf.h>
#include <util/generic/ymath.h>
#include <util/string/builder.h>
#include <library/cpp/string_utils/quote/quote.h>

namespace NBASS {

namespace {

using namespace yandex::maps::proto;

constexpr ui16 SHOW_ROUTE_GALLERY_IMAGE_WIDTH = 320;
constexpr ui16 SHOW_ROUTE_GALLERY_IMAGE_HEIGHT = 221;
constexpr float SHOW_ROUTE_GALLERY_PADDING_FACTOR = 0.05;

void PrepareRouterRequest(NHttpFetcher::TRequestPtr& request,
                          const NSc::TValue& locationFrom,
                          const NSc::TValue& locationVia,
                          const NSc::TValue& locationTo,
                          TStringBuf locale)
{
    TStringBuilder rll;
    rll << locationFrom["lon"] << ',' << locationFrom["lat"] << '~';
    if (!locationVia.IsNull()) {
        rll << locationVia["lon"] << ',' << locationVia["lat"] << '~';
    }
    rll << locationTo["lon"] << ',' << locationTo["lat"];
    request->AddCgiParam(TStringBuf("rll"), rll);

    request->AddCgiParam(TStringBuf("lang"), locale);
    request->AddCgiParam(TStringBuf("origin"), GEOSEARCH_ORIGIN);

    request->AddHeader(TStringBuf("Accept"), TStringBuf("application/x-protobuf"));
}

NHttpFetcher::THandle::TRef CreateCarRouteRequest(
        TContext& ctx,
        NHttpFetcher::IMultiRequest::TRef multiRequest,
        const NSc::TValue& locationFrom,
        const NSc::TValue& locationVia,
        const NSc::TValue& locationTo,
        const int carRoutesCount)
{
    // Docs here:
    //   https://wiki.yandex-team.ru/maps/dev/core/routing/software/router/newrouterapi/
    NHttpFetcher::TRequestPtr request = ctx.GetSources().CarRoutes().AttachRequest(multiRequest);
    PrepareRouterRequest(request, locationFrom, locationVia, locationTo, ctx.MetaLocale().ToString());

    request->AddCgiParam(TStringBuf("results"), ToString(carRoutesCount));
    request->AddCgiParam(TStringBuf("mode"), TStringBuf("approx"));

    return request->Fetch();
}

NHttpFetcher::THandle::TRef CreateMassTransitRouteRequest(
        TContext& ctx,
        NHttpFetcher::IMultiRequest::TRef multiRequest,
        const NSc::TValue& locationFrom,
        const NSc::TValue& locationTo,
        bool pedestrianOnly)
{
    // Docs here:
    //   https://wiki.yandex-team.ru/users/kshalnev/newrouterapi/
    //   https://wiki.yandex-team.ru/maps/dev/core/masstransit/api/mtrouter/
    NHttpFetcher::TRequestPtr request =
            pedestrianOnly ? ctx.GetSources().PedestrianRoutes().AttachRequest(multiRequest)
                           : ctx.GetSources().MassTransitRoutes().AttachRequest(multiRequest);

    PrepareRouterRequest(request, locationFrom, NSc::Null() /* locationVia */, locationTo, ctx.MetaLocale().ToString());

    return request->Fetch();
}

TResultValue ExtractCarRouteResponse(NHttpFetcher::THandle::TRef req, TContext& ctx,
        const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to, NSc::TValue* routeInfo)
{
    NHttpFetcher::TResponse::TRef resp = req->Wait();
    if (resp->IsError()) {
        TStringBuilder errText;
        errText << TStringBuf("Fetching car route info error: ") << resp->GetErrorText();
        LOG(DEBUG) << errText << Endl;
        return TError(TError::EType::SYSTEM, errText);
    }

    driving::summary::Summaries pbResp;
    if (!pbResp.ParseFromString(resp->Data)) {
        return TError(TError::EType::CONVERTERROR,
                      TStringBuilder() << "Can't parse route info protobuf");
    }

    for (int routeIndex = 0; routeIndex < pbResp.summaries_size(); ++routeIndex) {
        NSc::TValue &carRoute = (routeIndex == 0) ? (*routeInfo)["car"] : (*routeInfo)["car_add_routes"][routeIndex - 1];
        const auto& routeWeight = pbResp.summaries(routeIndex).weight();
        carRoute["length"]["text"] = routeWeight.distance().text();
        carRoute["length"]["value"] = routeWeight.distance().value();

        carRoute["time"]["text"] = routeWeight.time().text();
        carRoute["time"]["value"] = routeWeight.time().value();

        carRoute["jams_time"]["text"] = routeWeight.time_with_traffic().text();
        carRoute["jams_time"]["value"] = routeWeight.time_with_traffic().value();

        carRoute["maps_uri"] = GenerateRouteUri(ctx, from, via, to, TStringBuf("auto"));
    }

    return TResultValue();
}

TResultValue TryParseMassTransitRouteResponse(const TString& respData, bool needTranfers, NSc::TValue& route) {
    masstransit::summary::Summaries pbResp;
    if (!pbResp.ParseFromString(respData)) {
        return TError(TError::EType::CONVERTERROR,
                      TStringBuilder() << "Can't parse route info protobuf");
    }

    if (pbResp.summaries_size() > 0) {
        const auto& routeWeight = pbResp.summaries(0).weight();

        route["time"]["value"] = routeWeight.time().value();
        route["time"]["text"] = routeWeight.time().text();

        route["walking_dist"]["value"] = routeWeight.walking_distance().value();
        route["walking_dist"]["text"] = routeWeight.walking_distance().text();

        if (needTranfers) {
            route["transfers"] = routeWeight.transfers_count();
        }
    }

    return TResultValue();
}

TResultValue ExtractMassTransitRouteResponse(NHttpFetcher::THandle::TRef req, TContext& ctx,
        const NSc::TValue from, const NSc::TValue& to, NSc::TValue* routeInfo)
{
    NHttpFetcher::TResponse::TRef resp = req->Wait();
    if (resp->IsError()) {
        TStringBuilder errText;
        errText << TStringBuf("Fetching masstransit route info error: ") << resp->GetErrorText();
        LOG(DEBUG) << errText << Endl;
        return TError(TError::EType::SYSTEM, errText);
    }

    NSc::TValue publicTrRoute;
    if (auto err = TryParseMassTransitRouteResponse(resp->Data, true /* needTransfers */, publicTrRoute)) {
        return err;
    }

    if (!publicTrRoute.IsNull()) {
        publicTrRoute["maps_uri"] = GenerateRouteUri(ctx, from, to, TStringBuf("mt"));
        (*routeInfo)["public_transport"] = std::move(publicTrRoute);
    }

    return TResultValue();
}

TResultValue ExtractPedestrianRouteResponse(NHttpFetcher::THandle::TRef req, TContext& ctx,
        const NSc::TValue& from, const NSc::TValue& to, NSc::TValue* routeInfo)
{
    NHttpFetcher::TResponse::TRef resp = req->Wait();
    if (resp->IsError()) {
        TStringBuilder errText;
        errText << TStringBuf("Fetching pedestrian route info error: ") << resp->GetErrorText();
        LOG(DEBUG) << errText << Endl;
        return TError(TError::EType::SYSTEM, errText);
    }

    NSc::TValue pedestrianRoute;
    if (auto err = TryParseMassTransitRouteResponse(resp->Data, false /* needTransfers */, pedestrianRoute)) {
        return err;
    }

    if (!pedestrianRoute.IsNull()) {
        pedestrianRoute["maps_uri"] = GenerateRouteUri(ctx, from, to, TStringBuf("pd"));
        (*routeInfo)["pedestrian"] = std::move(pedestrianRoute);
    }
    return TResultValue();
}

TResultValue AskForUserAddress(TContext& ctx, TSpecialLocation locationName) {
    return TSaveAddressHandler::SetAsResponse(ctx, locationName);
}

}

TRouteResolver::TRouteResolver(TContext& Context)
    : Context{Context}
    , UserGeoid{NGeobase::UNKNOWN_REGION}
{
    const auto& meta = Context.Meta();
    if (meta.HasLocation()) {
        UserGeoid = NAlice::ReduceGeoIdToCity(Context.GlobalCtx().GeobaseLookup(), Context.UserRegion());
        UserPosition = InitGeoPositionFromLocation(meta.Location());
    }
}

TResultValue TRouteResolver::ResolveLocationImpl(TStringBuf whatSlotName, TStringBuf whereSlotName, NSc::TValue* firstLocation,
                                                 NSc::TValue* secondLocation, TString* roadName)
{
    Y_ASSERT(firstLocation != nullptr);

    TContext::TSlot* slotWhat = Context.GetOrCreateSlot(whatSlotName, "string");
    TContext::TSlot* slotWhere = Context.GetOrCreateSlot(whereSlotName, "string");

    const TSpecialLocation locationName = TSpecialLocation::GetNamedLocation(slotWhat);
    if (locationName.IsUserAddress()) {
        TSavedAddress address = Context.GetSavedAddress(locationName, slotWhat->SourceText);
        if (address.IsValid()) {
            *firstLocation = SavedAddressToGeo(Context, address);
        }

        // switch to remember_named_location form if user address is unknown or invalid
        if (firstLocation->IsNull()) {
            NSc::TValue needRememberLocationInfo;
            needRememberLocationInfo["locationName"].SetString(locationName);
            needRememberLocationInfo["slotWhat"].SetString(whatSlotName);
            needRememberLocationInfo["slotWhere"].SetString(whereSlotName);
            Context.CreateSlot(TStringBuf("need_remember_location"),
                           TStringBuf("need_remember_location"),
                           /* optional */ true,
                           std::move(needRememberLocationInfo));
            return AskForUserAddress(Context, locationName);
        }
    } else {
        TString sortBy;

        if (!IsSlotEmpty(slotWhat)) {
            SearchText = slotWhat->Value.GetString();
        }

        if (!IsSlotEmpty(slotWhere)) {
            TSpecialLocation locationName = TSpecialLocation::GetNamedLocation(slotWhere);
            if (!locationName.IsError()) {
                if (locationName.IsCurrentGeo()) {
                    TResultValue geoError;
                    NGeobase::TId regionId = locationName.GetGeo(Context, &geoError);
                    if (geoError) {
                        return geoError;
                    }
                    const auto& geobase = Context.GlobalCtx().GeobaseLookup();
                    NGeobase::TLinguistics names = geobase.GetLinguistics(regionId, Context.MetaLocale().Lang);
                    SearchText = (SearchText ? *SearchText + " " : "")  + names.Preposition + " " + names.PrepositionalCase;
                } else if (!SearchPos) {
                    // User asks to find something near his location, but we don't know his location
                    Context.CreateSlot(whereSlotName, "string", false);
                    Context.AddAttention("no_user_location");
                    return TResultValue();
                }
                sortBy = "distance";
            } else {
                SearchText = (SearchText ? *SearchText + " " : "") + TString{slotWhere->Value.GetString()};
            }
        }

        THolder<TGeoObjectResolver> resolver;
        bool needGeoSearch = true;
        if (SearchText) {
            TMaybe<TUserBookmark> userFavPos;
            if (Context.ClientFeatures().SupportsNavigator()) {
                userFavPos = Context.GetUserBookmarksHelper()->GetUserBookmark(*SearchText);
            }
            if (userFavPos) {
                needGeoSearch = false;
                *firstLocation = userFavPos->ToResolvedGeo(Context);
            } else {
                SearchText = TGeoAddrMap::FromRequest(Context, *SearchText, false /* useInherited */).BestNormalizedToponym();
                resolver.Reset(new TGeoObjectResolver(Context, *SearchText, SearchPos, "geo,biz", sortBy));
            }
        } else {
            // searchText is empty, if "what" and "where" are empty
            if (SearchPos) {
                resolver.Reset(new TGeoObjectResolver(Context, *SearchPos)); // Make reverse resolving of user's location
            } else {
                Context.CreateSlot(whereSlotName, "string", false /* optional */);
                return TResultValue();
            }
        }

        if (needGeoSearch) {
            if (roadName) {
                resolver->WaitAndParseGeoCoderRoadResponse(*SearchText, roadName);
                LOG(DEBUG) << "Found road name: " << *roadName << Endl;
            }

            const TResultValue errGeoSearch = resolver->WaitAndParseResponse(firstLocation, secondLocation);
            if (errGeoSearch) {
                return errGeoSearch;
            }

            // replace country with capital when perform geo search by text
            // we should not do this while resolving user position or user's bookmark (Navi)
            if (SearchText) {
                TGeoObjectResolver::ReplaceCountryWithCapital(Context, firstLocation);
                TGeoObjectResolver::ReplaceCountryWithCapital(Context, secondLocation);
            }
        }
    }

    if (firstLocation->IsNull()) {
        NSc::TValue data;
        data["slot"] = TString::Join(whatSlotName, "/", whereSlotName);
        data["what"] = slotWhat->SourceText.GetString();
        data["where"] = slotWhere->SourceText.GetString();

        // TODO(somebody): please move it to scenarios
        slotWhat->Reset();
        slotWhere->Reset();
        slotWhere->Optional = false;
        Context.AddErrorBlock(
            TError(TError::EType::NOGEOFOUND, "no geo found for slot"), data);
        return TResultValue();
    }

    NAlice::FillInUserCity(UserGeoid, firstLocation);
    NAlice::FillInUserCity(UserGeoid, secondLocation);

    return TResultValue();
}

TResultValue TRouteResolver::ResolveLocation(TStringBuf whatSlotName, TStringBuf whereSlotName, TStringBuf locationSlotName,
                             NSc::TValue* location, NSc::TValue* anotherLocation, TString* roadName)
{
    Y_ASSERT(location != nullptr);

    SearchPos.Clear();
    SearchText.Clear();
    if (const TResultValue err = SpecifySearchPos(Context, whereSlotName, UserPosition, FromPosition, SearchPos)) {
        return err;
    }

    if (const TResultValue err = ResolveLocationImpl(whatSlotName, whereSlotName, location, anotherLocation, roadName)) {
        return err;
    }

    TContext::TSlot* slotResolvedLocation = Context.CreateSlot(locationSlotName, TGeoObjectResolver::GeoObjectType(*location));
    slotResolvedLocation->Value.CopyFrom(*location);

    LOG(DEBUG) << "Resolved location :" << location->ToJson() << Endl;
    return TResultValue();
}

TResultValue TRouteResolver::ResolveLocationTo(NSc::TValue* location, NSc::TValue* anotherLocation)
{
    return TRouteResolver::ResolveLocation("what_to", "where_to", "resolved_location_to",
                            location, anotherLocation);
}

TResultValue TRouteResolver::ResolveLocationUnknown(NSc::TValue* location, NSc::TValue* anotherLocation)
{
    return TRouteResolver::ResolveLocation("what_unknown", "where_unknown", "resolved_location_unknown",
                            location, anotherLocation);
}

TResultValue TRouteResolver::ResolveLocationVia(NSc::TValue* location, NSc::TValue* anotherLocation, TString* roadName)
{
    return ResolveLocation("what_via", "where_via", "resolved_location_via",
                            location, anotherLocation, roadName);
}

TResultValue TRouteResolver::ResolveLocationFrom(NSc::TValue* location, NSc::TValue* anotherLocation)
{
    return ResolveLocation("what_from", "where_from", "resolved_location_from",
                            location, anotherLocation);
}

TResultValue TRouteResolver::ResolveViaLocationByRoadName(TStringBuf roadName,
                                                          const TGeoPosition& fromPosition,
                                                          const TGeoPosition& toPosition,
                                                          TGeoPosition* viaPosition) {
    TResultValue result;

    THolder<NHttpFetcher::TRequest> request = Context.GetSources().RouterVia().Request(
        [&result, viaPosition] (const TString& responseData) {
        driving::via_point::ViaPoint viaPoint;
        if (!viaPoint.ParseFromString(responseData)) {
            result = TError(
                TError::EType::CONVERTERROR,
                TStringBuilder() << "cannot parse protobuf"
            );
        }
        if (viaPoint.has_via_point()) {
            *viaPosition = TGeoPosition(viaPoint.via_point().lat(), viaPoint.via_point().lon());
            LOG(DEBUG) << "Via point for road: " << viaPosition->Lat << ','
                        << viaPosition->Lon << Endl;
            return NHttpFetcher::TFetchStatus::Success();
        }
        return NHttpFetcher::TFetchStatus::Failure("noRoadFound");
    });

    TCgiParameters cgi;
    cgi.InsertUnescaped(
        TStringBuf("rll"),
        TStringBuilder() << fromPosition.Lon << ',' <<  fromPosition.Lat
        << '~' << toPosition.Lon << ',' << toPosition.Lat
    );
    cgi.InsertUnescaped("vlang", Context.Meta().Lang());
    cgi.InsertUnescaped("text", roadName);
    if (Context.Meta().DeviceState().NavigatorState().UserSettings().AvoidTolls()) {
        cgi.InsertUnescaped("avoid", "tolls");
    }
    request->AddCgiParams(cgi);

    request->AddHeader(TStringBuf("Accept"), TStringBuf("application/x-protobuf"));

    NHttpFetcher::TResponse::TRef resp = request->Fetch()->Wait();
    if (resp->IsError() && !result) {
        TStringBuilder errText;
        errText << TStringBuf("Fetching from router error: ") << resp->GetErrorText();
        LOG(DEBUG) << errText << Endl;
        return TError(
            TError::EType::SYSTEM,
            errText
        );
    }
    return result;
}

NSc::TValue SavedAddressToGeo(TContext& ctx, const TSavedAddress& address) {
    if (address.IsNull()) {
        return NSc::TValue();
    }

    TGeoPosition userCoordinates(address.Latitude(), address.Longitude());
    NSc::TValue resolvedLocation = LocationToGeo(ctx, userCoordinates);
    if (resolvedLocation.IsNull()) {
        LOG(ERR) << "Failed to resolve user saved address : " << address.ToJson() << Endl;
    }

    return resolvedLocation;
}

TResultValue SpecifySearchPos(TContext& context, TStringBuf whereSlotName,
                              const TMaybe<TGeoPosition>& userPosition, const TMaybe<TGeoPosition>& fromPosition,
                              TMaybe<TGeoPosition>& searchPos)
{
    searchPos = fromPosition ? fromPosition : userPosition;

    const TContext::TSlot* slotWhere = context.GetOrCreateSlot(whereSlotName, "string");
    const TSpecialLocation locationName = TSpecialLocation::GetNamedLocation(slotWhere);
    if (locationName == TSpecialLocation::EType::NEAR_ME) {
        searchPos = userPosition;
    } else if (locationName.IsUserAddress()) {
        const TSavedAddress userAddress = context.GetSavedAddress(locationName, slotWhere->SourceText);
        if (userAddress.IsValid()) {
            searchPos = TGeoPosition(userAddress.Latitude(), userAddress.Longitude());
        } else {
            // switch to remember_named_location form if user address is unknown
            return AskForUserAddress(context, locationName);
        }
    }

    return TResultValue();
}

NSc::TValue LocationToGeo(TContext& ctx, const TGeoPosition& location) {
    NSc::TValue resolvedLocation;
    THolder<TGeoObjectResolver> resolver = MakeHolder<TGeoObjectResolver>(ctx, location);

    if (TResultValue geoError = resolver->WaitAndParseResponse(&resolvedLocation)) {
        return NSc::TValue();
    }

    return resolvedLocation;
}

TString GenerateNavigatorUri(TContext& ctx, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to, const TString& fallbackUrl) {
    const NSc::TValue& fromLocation =  from["location"];
    const NSc::TValue& toLocation = to["location"];

    TCgiParameters naviCgi;
    naviCgi.InsertUnescaped("lat_from", ToString(fromLocation["lat"]));
    naviCgi.InsertUnescaped("lon_from", ToString(fromLocation["lon"]));
    if (!via.IsNull()) {
        const NSc::TValue& viaLocation = via["location"];
        naviCgi.InsertUnescaped("lat_via_0", ToString(viaLocation["lat"]));
        naviCgi.InsertUnescaped("lon_via_0", ToString(viaLocation["lon"]));
    }
    naviCgi.InsertUnescaped("lat_to", ToString(toLocation["lat"]));
    naviCgi.InsertUnescaped("lon_to", ToString(toLocation["lon"]));
    return GenerateNavigatorUri(ctx, "build_route_on_map", naviCgi, fallbackUrl);
}

TString GenerateRouteUri(TContext& ctx, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to, TStringBuf routeType) {
    const NSc::TValue& fromLocation =  from["location"];
    const NSc::TValue& toLocation = to["location"];

    TCgiParameters mapsCgi;
    TStringBuilder rtext;
    rtext << fromLocation["lat"] << ',' << fromLocation["lon"] << '~';
    if (!via.IsNull()) {
        const NSc::TValue& viaLocation = via["location"];
        rtext << viaLocation["lat"] << ',' << viaLocation["lon"] << '~';
        mapsCgi.InsertUnescaped(TStringBuf("via"), TStringBuf("1"));
    }
    rtext << toLocation["lat"] << ',' << toLocation["lon"];
    mapsCgi.InsertUnescaped(TStringBuf("rtext"), rtext);

    if (!routeType.empty()) {
        mapsCgi.InsertUnescaped(TStringBuf("rtt"), routeType);
    }

    if (ctx.ClientFeatures().SupportsIntentUrls()) {
        TString mapsLink = GenerateMapsUri(ctx, mapsCgi);
        if (routeType == TStringBuf("auto")) {
            return GenerateNavigatorUri(ctx, from, via, to, mapsLink);
        }

        // TODO: generate links for Yandex.Transport (if route_type="mt")
        return mapsLink;
    }
    // Default url
    return GenerateMapsUri(ctx, mapsCgi, true /* needSimpleUrl */);
}

TString GenerateRouteUri(TContext& ctx, const NSc::TValue& from, const NSc::TValue& to, TStringBuf routeType) {
    return GenerateRouteUri(ctx, from, NSc::TValue::Null() /* via */, to, routeType);
}

TResultValue GetRouteInfo(TContext& ctx, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to,
                          NSc::TValue* routeInfo, const int carRoutesCount, const bool carRouteImg) {
    NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::MultiRequest();

    const NSc::TValue& locationFrom =  from["location"];
    const NSc::TValue& locationTo = to["location"];
    const NSc::TValue& locationVia = via["location"];
    // Get info about routes (in parallel requests)
    NHttpFetcher::THandle::TRef reqsRouteCar = CreateCarRouteRequest(ctx, multiRequest,
           locationFrom, locationVia, locationTo, carRoutesCount);
    NHttpFetcher::THandle::TRef reqRouteMassTr = CreateMassTransitRouteRequest(ctx, multiRequest,
            locationFrom, locationTo, false /*pedestrianOnly */);
    NHttpFetcher::THandle::TRef reqRoutePedestrian = CreateMassTransitRouteRequest(ctx, multiRequest,
            locationFrom, locationTo, true /*pedestrianOnly */);

    // MEGAMIND-2550: make parallel car route image request to map api
    NSc::TValue carImgResp;
    TResultValue errCarImg;
    if (carRouteImg) {
        constexpr auto routeIndex = 0;
        constexpr auto carRoute = TStringBuf("route");
        constexpr auto showJams = TStringBuf("true");
        errCarImg = ResolveStaticMapRouter(ctx, from, via, to, carRoute, carImgResp, showJams, routeIndex);
        if (errCarImg) {
            LOG(DEBUG) << "failed ResolveStaticMapRouter: " << errCarImg->Msg << Endl;
        }
    }
    multiRequest->WaitAll();

    (*routeInfo).SetNull();
    TResultValue errCar = ExtractCarRouteResponse(reqsRouteCar, ctx, from, via, to, routeInfo);
    if (carRouteImg && routeInfo->Has("car") && !errCarImg) {
        (*routeInfo)["car"]["image_url"] = carImgResp["url"];
    }
    TResultValue errMassTr = ExtractMassTransitRouteResponse(reqRouteMassTr, ctx, from, to, routeInfo);
    TResultValue errPedestrian = ExtractPedestrianRouteResponse(reqRoutePedestrian, ctx, from, to, routeInfo);

    if (errCar && errMassTr && errPedestrian) {
        return TError(
            TError::EType::SYSTEM,
            TStringBuilder() << TStringBuf("Failed to fetch routes info")
        );
    }
    return TResultValue();
}

TResultValue GetRouteInfo(TContext& ctx, const NSc::TValue& from, const NSc::TValue& to, NSc::TValue* routeInfo) {
    return GetRouteInfo(ctx, from, NSc::TValue::Null() /* via */, to, routeInfo);
}


TResultValue ResolveStaticMapRouter(TContext& ctx, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to,
                                    TStringBuf routingMode, NSc::TValue& result, TStringBuf showJams, const int routeIndex) {
    const NSc::TValue& locationFrom =  from["location"];
    const NSc::TValue& locationTo = to["location"];
    const NSc::TValue& locationVia = via["location"];

    NHttpFetcher::TRequestPtr request = ctx.GetSources().ApiMapsStaticMapRouter().Request();

    TStringBuilder rll;
    rll << locationFrom["lon"] << ',' << locationFrom["lat"] << '~' ;
    if (!locationVia.IsNull()) {
        rll << locationVia["lon"] << ',' << locationVia["lat"] << '~' ;
    }
    rll << locationTo["lon"] << ',' << locationTo["lat"];

    request->SetMethod("GET");
    request->AddCgiParam("rll", rll);
    request->AddCgiParam("lang", ctx.Meta().Lang());
    request->AddCgiParam("show_jams", showJams);
    request->AddCgiParam("routing_mode", routingMode);
    request->AddCgiParam("origin", GEOSEARCH_ORIGIN);
    request->AddCgiParam("route_index", ToString(routeIndex));
    TStringBuilder mapSize;
    mapSize << SHOW_ROUTE_GALLERY_IMAGE_WIDTH <<  ',' << SHOW_ROUTE_GALLERY_IMAGE_HEIGHT;
    request->AddCgiParam("map_size", mapSize);
    request->AddCgiParam("padding_factor", ToString(SHOW_ROUTE_GALLERY_PADDING_FACTOR));
    request->AddCgiParam("key", ctx.GlobalCtx().Secrets().StaticMapRouterKey);

    TContext::TSlot* slotWhatFrom = ctx.GetSlot("what_from", "named_location");
    TContext::TSlot* slotWhatTo = ctx.GetSlot("what_to", "named_location");
    TContext::TSlot* slotWhatVia = ctx.GetSlot("what_via", "named_location");
    TStringBuilder markers;
    if (!IsSlotEmpty(slotWhatFrom))
        markers << slotWhatFrom->Value.GetString();
    else
        markers << "ya_ru";

    if (!IsSlotEmpty(slotWhatVia))
        markers << ',' << slotWhatVia->Value.GetString();
    else if (!locationVia.IsNull())
        markers << ",round";

    if (!IsSlotEmpty(slotWhatTo))
        markers <<  ',' << slotWhatTo->Value.GetString();
    else
        markers << ",round";
    request->AddCgiParam("markers", markers);

    request->SetContentType("application/json");

    NHttpFetcher::TResponse::TRef handler = request->Fetch()->Wait();
    if (handler->IsError()) {
        return TError(TError::EType::SYSTEM,
                      TStringBuilder() << TStringBuf("static map router request error: ") << handler->GetErrorText());
    }

    NSc::TValue response = NSc::TValue::FromJson(handler->Data);
    if (response.IsNull()) {
        TStringBuilder errText;
        errText << "Can not parse static map response" << Endl;
        return TError(TError::EType::SYSTEM, errText);
    }
    if (response.Has("error")) {
        return TError(TError::EType::NOROUTE, response.TrySelect("error/message").GetString());
    }
    result["url"] = response.TrySelect("data/url");
    return TResultValue();
}


} // namespace NBASS


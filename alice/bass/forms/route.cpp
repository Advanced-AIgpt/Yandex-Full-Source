#include "route.h"

#include "geocoder.h"
#include "route_helpers.h"
#include "route_tools.h"

#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/metrics/metrics.h>

#include <alice/bass/forms/geoaddr.h>
#include <alice/bass/forms/geodb.h>
#include <alice/bass/forms/common/saved_address.h>
#include <alice/bass/forms/navigator/how_long.h>
#include <alice/bass/forms/navigator/map_search_intent.h>
#include <alice/bass/forms/navigator/route_intents.h>
#include <alice/bass/forms/special_location.h>
#include <alice/bass/forms/taxi.h>
#include <alice/bass/forms/taxi/handler.h>
#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/forms/search/search.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

#include <library/cpp/scheme/scheme.h>

#include <util/string/builder.h>
#include <library/cpp/string_utils/quote/quote.h>

namespace NBASS {

namespace {
constexpr TStringBuf SHOW_ROUTE = "personal_assistant.scenarios.show_route";
constexpr TStringBuf SHOW_ROUTE_THERE = "personal_assistant.scenarios.show_route_there";
constexpr TStringBuf SHOW_ROUTE_ELLIPSIS = "personal_assistant.scenarios.show_route__ellipsis";
constexpr TStringBuf SHOW_ROUTE_GALLERY_EXPERIMENT = "show_route_gallery";
constexpr TStringBuf SHOW_ROUTE_GALLERY_ALT_ROUTES_EXPERIMENT = "show_route_gallery_alt_routes";
constexpr TStringBuf SHOW_ROUTE_GALLERY_LONGWAY_FIX_EXPERIMENT = "show_route_gallery_longway_fix";
constexpr TStringBuf ROUTE_MEGAMIND_EXPERIMENT = "mm_enable_protocol_scenario=Route";

THashMap<TString, TString> routeTypeToRoutingMode = {
    {"car", "route"},
    {"public_transport", "masstransit"},
    {"pedestrian", "pedestrian"}
};

const TStringBuf LOCATION_SLOTS[] = {"what_from", "what_to", "what_via", "where_from", "where_to", "where_via"};

const int MAX_CAR_ROUTES = 3;
}

bool TRouteFormHandler::FillToSlots(TContext& ctx) {
    const TContext::TSlot* whereTo = ctx.GetOrCreateSlot("where_to", "string");
    const TContext::TSlot* whatTo = ctx.GetOrCreateSlot("what_to", "string");
    const TContext::TSlot* locationTo = ctx.GetSlot("resolved_location_to");

    // some *_to slots are not empty, so nothing to do here
    if (!IsSlotEmpty(whatTo) || !IsSlotEmpty(whereTo) || !IsSlotEmpty(locationTo)) {
        return true;
    }

    // all *_to slots are empty and only these clients can provide any additional info
    if (!ctx.ClientFeatures().SupportsNavigator()) {
        return false;
    }

    TContext::TSlot* whatVia = ctx.GetSlot("what_via");
    TContext::TSlot* whereVia = ctx.GetSlot("where_via");

    // no information about route destination at all
    if (IsSlotEmpty(whatVia) && IsSlotEmpty(whereVia)) {
        return false;
    }

    // 1. User has current route in navigator and wants to add some via-point: fill where_to slot with final destination
    // OR
    // 2. There are no route but user request looks like adding via-point: set that point as destination
    const auto currentLocation = ctx.Meta().DeviceState().NavigatorState().CurrentRoute().Points();
    if (currentLocation.Size() > 1) {
        TStringBuilder res;
        const auto& lastLocation = currentLocation[currentLocation.Size() - 1];
        res << lastLocation.Lon();
        res << " ";
        res << lastLocation.Lat();
        ctx.CreateSlot(TStringBuf("where_to"), TStringBuf("string"), true /* optional */, NSc::TValue(res));
    } else {
        if (!IsSlotEmpty(whatVia)) {
            ctx.CreateSlot(TStringBuf("what_to"), whatVia->Type, whatVia->Optional, whatVia->Value);
            whatVia->Reset();
        }
        if (!IsSlotEmpty(whereVia)) {
            ctx.CreateSlot(TStringBuf("where_to"), whereVia->Type, whereVia->Optional, whereVia->Value);
            whereVia->Reset();
        }
    }

    return true;
}

// static
void TRouteFormHandler::GetFormUpdate(const NSc::TValue& locationTo, NSc::TValue& formUpdate) {
    formUpdate["name"] = SHOW_ROUTE;
    TContext::TSlot resolvedLocationSlot("resolved_location_to", "geo");
    resolvedLocationSlot.Value = locationTo;
    formUpdate["slots"].SetArray().Push(resolvedLocationSlot.ToJson());
}

bool TRouteFormHandler::HasNamedLocation(const TContext& ctx) const {
    for (auto slotName : LOCATION_SLOTS) {
        if (!IsSlotEmpty(ctx.GetSlot(slotName, "named_location"))) {
            return true;
        }
    }
    return false;
}

bool TRouteFormHandler::HasHowLongIntent(const TContext& ctx) const {
    const TContext::TSlot* actionType = ctx.GetSlot("route_action_type", "route_action_type");
    const TStringBuf actionTypeValue = !IsSlotEmpty(actionType) ? actionType->Value.GetString() : TStringBuf("");
    return (actionTypeValue == "how_long" || actionTypeValue == "how_far");
}

bool CheckConfirmation(TContext& ctx) {
    if (ctx.HasExpFlag(NAVIGATOR_ALICE_CONFIRMATION) || NRoute::CheckNavigatorState(ctx, NRoute::WAITING_STATE)) {
        TContext::TSlot* confirmationSlot = ctx.GetOrCreateSlot("confirmation", "confirmation");
        if (confirmationSlot->Value.GetString() == "yes") {
            TExternalConfirmationIntent navigatorIntentHandler(ctx, true /* isConfirmed */);
            navigatorIntentHandler.Do();
            return true;
        } else if (confirmationSlot->Value.GetString() == "no") {
            TExternalConfirmationIntent navigatorIntentHandler(ctx, false /* isConfirmed */);
            navigatorIntentHandler.Do();
            return true;
        }
    }
    return false;
}

TResultValue TRouteFormHandler::Do(TRequestHandler& r) {
    // TODO: probably we should do it asynchronously (parallel to other requests), but the code will become complicated
    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::ROUTE);

    TRouteResolver routeResolver = TRouteResolver(ctx);

    bool hasHowLongIntent = HasHowLongIntent(ctx);

    // remember whether *_from and *_to slots were defined or not before slots filling
    bool wereFromSlotsDefined = !IsSlotEmpty(ctx.GetSlot("what_from")) || !IsSlotEmpty(ctx.GetSlot("where_from"));
    bool wereToSlotsDefined = !IsSlotEmpty(ctx.GetSlot("what_to")) || !IsSlotEmpty(ctx.GetSlot("where_to"));

    if (ctx.HasExpFlag(ROUTE_MEGAMIND_EXPERIMENT)) {
        for (const auto& slotName: LOCATION_SLOTS) {
            TContext::TSlot* slot = ctx.GetSlot(slotName);
            if (IsSlotEmpty(slot) || slot->Type != "string") {
                continue;
            }
            const auto normVal = NNlu::TRequestNormalizer::Normalize(LANG_RUS, slot->Value);
            LOG(DEBUG) << "Normalize for Route scenario slot: " << slotName << ", value: " << slot->Value << " -> " << normVal << Endl;
            slot->Value = normVal;
        }
    }

    if (CheckConfirmation(ctx)) {
        return TResultValue();
    }

    if (!FillToSlots(ctx)) {
        if (NRoute::CheckNavigatorState(ctx, NRoute::WAITING_STATE)) {
            TExternalConfirmationIntent navigatorIntentHandler(ctx, true /* isConfirmed */);
            return navigatorIntentHandler.Do();
        }
        if (ctx.Meta().DeviceState().NavigatorState().HasCurrentRoute()) {
            // "how long (or far) to drive" with active route and no destination specified
            if (hasHowLongIntent) {
                return TNavigatorHowLongHandler::SetAsResponse(ctx);
            }

            // show current route when user said "let's drive" without any from or to
            if (!wereFromSlotsDefined) {
                TShowGuidanceIntent navigatorIntentHandler(ctx);
                return navigatorIntentHandler.Do();
            }
        }

        TContext::TSlot* whereTo = ctx.GetOrCreateSlot("where_to", "string");
        whereTo->Optional = false;
        ctx.AddSearchSuggest();
        ctx.AddOnboardingSuggest();
        return TResultValue();
    }

    // Resolve location_from
    NSc::TValue locationFrom, anotherFrom;
    const TContext::TSlot* slotInputResolvedFrom = ctx.GetSlot("resolved_location_from");
    if (!IsSlotEmpty(slotInputResolvedFrom) && slotInputResolvedFrom->Value.Has("location")) {
        locationFrom = slotInputResolvedFrom->Value;
    } else if (const TResultValue err = routeResolver.ResolveLocationFrom(&locationFrom, &anotherFrom))
    {
        // todo: maybe fallback to search
        return err;
    }

    if (locationFrom.IsNull()) {
        // Failed to resolve location_from, error block was already added
        ctx.AddSearchSuggest();
        ctx.AddOnboardingSuggest();
        return TResultValue();
    }

    if (locationFrom.Has("location")) {
        // Use resolved location_from as the start point for location_via
        const NSc::TValue& location = locationFrom["location"];
        routeResolver.FromPosition = TGeoPosition(location["lat"].GetNumber(), location["lon"].GetNumber());
    }

    // Resolve location_via
    NSc::TValue locationVia, anotherVia;
    TString roadName;
    TMaybe<TGeoPosition> viaSearchPos;
    TMaybe<TString> viaSearchText;
    const TContext::TSlot* slotInputResolvedVia = ctx.GetSlot("resolved_location_via");
    if (!IsSlotEmpty(slotInputResolvedVia) && slotInputResolvedVia->Value.Has("location")) {
        locationVia = slotInputResolvedVia->Value;
    } else if (!IsSlotEmpty(ctx.GetSlot("what_via")) || !IsSlotEmpty(ctx.GetSlot("where_via"))) {
        if (const TResultValue err = routeResolver.ResolveLocationVia(&locationVia, &anotherVia, &roadName)) {
            // todo: maybe fallback to search
            return err;
        }
        // remember search text and search position for future map search in Navigator
        viaSearchPos = routeResolver.SearchPos;
        viaSearchText = routeResolver.SearchText;
    }

    // Resolve location_to
    NSc::TValue locationTo, anotherTo;
    if (locationVia.Has("location")) {
        // Use resolved location_via as the start point for location_to
        const NSc::TValue& location = locationVia["location"];
        routeResolver.FromPosition = TGeoPosition(location["lat"].GetNumber(), location["lon"].GetNumber());
    }

    const TContext::TSlot* slotInputResolvedTo = ctx.GetSlot("resolved_location_to");
    if (!IsSlotEmpty(slotInputResolvedTo) && slotInputResolvedTo->Value.Has("location")) {
        locationTo = slotInputResolvedTo->Value;
    } else {
        if (const TResultValue err = routeResolver.ResolveLocationTo(&locationTo, &anotherTo))
        {
            // todo: maybe fallback to search
            return err;
        }
    }

    if (locationTo.IsNull()) {
    // Failed to resolve location_to, error block was already added
        ctx.AddSearchSuggest();
        ctx.AddOnboardingSuggest();
        return TResultValue();
    }

    // If roadName is not empty we should try to resolve via location again
    if (!roadName.empty() && locationTo.Has("location") && locationFrom.Has("location")) {
        auto startPoint = TGeoPosition(
            locationFrom["location"]["lat"].GetNumber(),
            locationFrom["location"]["lon"].GetNumber()
        );
        auto endPoint = TGeoPosition(
            locationTo["location"]["lat"].GetNumber(),
            locationTo["location"]["lon"].GetNumber()
        );
        TGeoPosition viaPoint;
        const TResultValue err = routeResolver.ResolveViaLocationByRoadName(roadName, startPoint, endPoint, &viaPoint);
        if (!err) {
            locationVia["location"]["lat"] = viaPoint.Lat;
            locationVia["location"]["lon"] = viaPoint.Lon;
        }
    }

    const TContext::TSlot* slotRouteType = ctx.GetOrCreateSlot("route_type", "route_type");
    const TStringBuf requestedRouteType = !IsSlotEmpty(slotRouteType) ? slotRouteType->Value.GetString() : TStringBuf("");

    // try to build route in Navigator first of all - no need in route info it this case
    if (ctx.ClientFeatures().SupportsNavigator()) {
        // build car route only if user did not select other route type explicitly
        bool buildRoute = requestedRouteType.empty() || requestedRouteType == "auto" || requestedRouteType == "non_pedestrian";
        // do not build route if user question was "how long to drive to ...?"
        buildRoute = buildRoute && !hasHowLongIntent;
        if (buildRoute) {
            // we do not use user location as start point for routes in navigator
            bool doNotUseLocationFrom = (!wereFromSlotsDefined || TSpecialLocation::IsNearLocation(ctx.GetSlot("where_from")));

            const NSc::TValue& fromLocation = doNotUseLocationFrom ? NSc::Null() : locationFrom.TrySelect("location");
            const NSc::TValue& viaLocation = locationVia.TrySelect("location");
            const NSc::TValue& toLocation = locationTo.TrySelect("location");

            // search poi on map
            if (!locationVia.IsNull() && viaSearchText && TGeoObjectResolver::GeoObjectType(locationVia) == "poi")  {
                // add mapsearch intent
                TMapSearchNavigatorIntent naviSearchIntentHandler(ctx, *viaSearchText, viaSearchPos);
                if (TResultValue err = naviSearchIntentHandler.Do()) {
                    return err;
                }

                // build route if do not have one or start/destination were defined in request
                if (!ctx.Meta().DeviceState().NavigatorState().HasCurrentRoute() || wereFromSlotsDefined || wereToSlotsDefined) {
                    TBuildRouteNavigatorIntent navigatorIntentHandler(ctx, fromLocation, NSc::Null() /* via */, toLocation);
                    return navigatorIntentHandler.Do();
                }
            } else {
                TBuildRouteNavigatorIntent navigatorIntentHandler(ctx, fromLocation, viaLocation, toLocation);
                return navigatorIntentHandler.Do();
            }
        }
    }

    TContext::TSlot* slotRouteInfo = ctx.CreateSlot("route_info", "route_info");
    const bool showGalleryExperiment = ctx.HasExpFlag(SHOW_ROUTE_GALLERY_EXPERIMENT);
    const bool carRouteImg = showGalleryExperiment && ctx.HasExpFlag(SHOW_ROUTE_GALLERY_LONGWAY_FIX_EXPERIMENT);
    TResultValue errRoute = GetRouteInfo(ctx, locationFrom, locationVia, locationTo, &(slotRouteInfo->Value), MAX_CAR_ROUTES, carRouteImg);
    if (errRoute) {
        return errRoute;
    }

    bool addNoRouteError = false;
    if (slotRouteInfo->Value.IsNull()) {
        // if it is a question about distance or travel time between A and B without direct route, we can try to answer using generic search
        if (hasHowLongIntent && !HasNamedLocation(ctx)) {
            if (ctx.HasExpFlag("route_disable_search_change_form")) {
                ctx.AddTextCardBlock("render_error__noroute");
                return TResultValue();
            }
            TContext::TPtr newCtx = TSearchFormHandler::SetAsResponse(ctx, false, ctx.Meta().Utterance());
            if (newCtx) {
                Y_STATS_INC_COUNTER_IF(!ctx.IsTestUser(), "route_switch_to_search");
                // make sure the final intent will be search (e.g. not switched to find_poi in case of navigator)
                TContext::TSlot* slotDisableChangeIntent = newCtx->CreateSlot("disable_change_intent", "bool");
                slotDisableChangeIntent->Value.SetBool(true);
                return ctx.RunResponseFormHandler();
            }
        }
        addNoRouteError = true;
    }

    // Add suggests
    const bool shouldAddSuggest = !(showGalleryExperiment && requestedRouteType.empty());
    if (slotRouteInfo->Value.Has("car")) {
        if (!ctx.HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_TAXI_NEW)) {
            if (!IsSlotEmpty(ctx.GetSlot("what_from")) || !IsSlotEmpty(ctx.GetSlot("where_from"))) {
                NTaxi::THandler::AddTaxiSuggest(ctx, "resolved_location_from", "resolved_location_to");
            } else {
                NTaxi::THandler::AddTaxiSuggest(ctx, "", "resolved_location_to");
            }
        } else {
            if (!IsSlotEmpty(ctx.GetSlot("what_from")) || !IsSlotEmpty(ctx.GetSlot("where_from"))) {
                TTaxiOrderHandler::AddTaxiSuggest(ctx, "resolved_location_from", "resolved_location_to");
            } else {
                TTaxiOrderHandler::AddTaxiSuggest(ctx, "", "resolved_location_to");
            }
        }

        if (requestedRouteType != TStringBuf("auto")) {
            if (shouldAddSuggest)
                ctx.AddSuggest(TStringBuf("show_route__go_by_car"));
        }
    }
    if (slotRouteInfo->Value.Has("public_transport") && requestedRouteType != TStringBuf("public_transport")) {
        if (shouldAddSuggest)
            ctx.AddSuggest(TStringBuf("show_route__go_by_public_transport"));
    }
    if (slotRouteInfo->Value.Has("pedestrian") && requestedRouteType != TStringBuf("pedestrian")) {
        if (shouldAddSuggest)
            ctx.AddSuggest(TStringBuf("show_route__go_by_foot"));
    }
    if (!anotherFrom.IsNull() && !showGalleryExperiment) {
        ctx.AddSuggest(TStringBuf("show_route__another_location_from"), anotherFrom);
    }
    if (!anotherTo.IsNull() && !showGalleryExperiment) {
        ctx.AddSuggest(TStringBuf("show_route__another_location_to"), anotherTo);
    }
    if (!showGalleryExperiment) {
        // Add suggest to open route in Yandex Maps and slot with route uri
        ctx.AddSuggest(TStringBuf("show_route__show_on_map"));
    }
    TContext::TSlot* slotRouteMapsUri = ctx.CreateSlot("route_maps_uri", "string");
    if (ctx.ClientFeatures().SupportsNavigator() &&
        (requestedRouteType.empty() || requestedRouteType == "auto" || requestedRouteType == "non_pedestrian"))
    {
        slotRouteMapsUri->Value = GenerateRouteUri(ctx, locationFrom, locationVia, locationTo, "auto");
    } else {
        slotRouteMapsUri->Value = GenerateRouteUri(ctx, locationFrom, locationVia, locationTo);
    }

    TString navigatorUri = GenerateNavigatorUri(ctx, locationFrom, locationVia, locationTo);
    ctx.CreateSlot("route_navigator_uri", "string", true /* optional */, NSc::TValue(navigatorUri));

    // search suggest should be last
    ctx.AddSearchSuggest();
    ctx.AddOnboardingSuggest();

    TResultValue err;
    if (!addNoRouteError && (err = PushGalleryDivCardBlock(ctx, locationFrom, locationVia, locationTo, slotRouteType))) {
        LOG(ERR) << "Error while adding div card block: " << err->Msg << Endl;
        addNoRouteError = true;
    }
    if (addNoRouteError) {
        ctx.AddErrorBlock(TError::EType::NOROUTE, "no route found");
    }
    return TResultValue();
}

// static
TResultValue TRouteFormHandler::SetAsResponse(TContext& ctx) {
    ctx.SetResponseForm(SHOW_ROUTE, false /* setCurrentFormAsCallback */);
    return ctx.RunResponseFormHandler();
}

// static
TResultValue TRouteFormHandler::SetAsResponse(TContext& ctx, TStringBuf whatSlotName, TStringBuf whereSlotName) {
    TIntrusivePtr<TContext> newContext = ctx.SetResponseForm(SHOW_ROUTE, false /* setCurrentFormAsCallback */);
    Y_ENSURE(newContext);
    TContext::TSlot* whatSlot = ctx.GetSlot(whatSlotName);
    TContext::TSlot* whereSlot = ctx.GetSlot(whereSlotName);
    newContext->CreateSlot("what_to", whatSlot->Type, true, whatSlot->Value);
    newContext->CreateSlot("where_to", whereSlot->Type, true, whereSlot->Value);
    return ctx.RunResponseFormHandler();
}

void TRouteFormHandler::Register(THandlersMap* handlers) {
    auto cbRouteForm = []() {
        return MakeHolder<TRouteFormHandler>();
    };
    handlers->emplace(SHOW_ROUTE, cbRouteForm);
    handlers->emplace(SHOW_ROUTE_THERE, cbRouteForm);
    handlers->emplace(SHOW_ROUTE_ELLIPSIS, cbRouteForm);
}

TResultValue TRouteFormHandler::AddDivCardBlockByRouteType(TContext& ctx, NSc::TValue& cardData, ShowRouteDivCardData& data, const TRouteOptions opts, const NSc::TValue& imageUrl) {
    NSc::TValue out;
    const TContext::TSlot* slotRouteInfo = ctx.GetSlot("route_info", "route_info");
    if (IsSlotEmpty(slotRouteInfo))
        return TError(TError::EType::SYSTEM, TStringBuf("Slot route_info is empty"));

    if (imageUrl.IsNull()) {
        NSc::TValue resp;
        if (TResultValue error = ResolveStaticMapRouter(ctx, data.from, data.via, data.to,
                                 routeTypeToRoutingMode[data.routeType], resp, data.showJams)) {
            return error;
        }
        out["image_url"] = resp["url"];
    } else {
        LOG(DEBUG) << "use image url from cache arg" << Endl;
        out["image_url"] = imageUrl;
    }

    TStringBuilder mapsUrlKey;
    mapsUrlKey << data.routeType << "/" << "maps_uri";
    out["maps_url"] = slotRouteInfo->Value.TrySelect(mapsUrlKey);

    TStringBuilder timeKey;
    timeKey << data.routeType << "/" << data.timeKeyPrefix << "/value";
    out["time"] = slotRouteInfo->Value.TrySelect(timeKey).ForceNumber();

    out["text_key"] = data.routeType;

    if (const TAvatar* avatar = ctx.Avatar(TStringBuf("route"), data.iconName))
        out["icon"] = avatar->Https;
    cardData.Push(out);

    if (!opts.Test(ADDITIONAL_ROUTES) || !ctx.HasExpFlag(SHOW_ROUTE_GALLERY_ALT_ROUTES_EXPERIMENT)) {
        return TResultValue();
    }
    TStringBuilder addRoutesKey;
    addRoutesKey << data.routeType << "_add_routes";
    for (size_t routeIndex = 0; routeIndex < slotRouteInfo->Value[addRoutesKey].GetArray().size(); ++routeIndex) {
        NSc::TValue resp;
        NSc::TValue outAdd;
        const NSc::TValue& addRouteInfo = slotRouteInfo->Value[addRoutesKey][routeIndex];
        if (TResultValue error = ResolveStaticMapRouter(ctx, data.from, data.via, data.to,
                                 routeTypeToRoutingMode[data.routeType], resp, data.showJams, routeIndex + 1)) {
            continue;
        }
        outAdd["image_url"] = resp["url"];
        outAdd["maps_url"] = addRouteInfo["maps_uri"];

        TStringBuilder timeKey;
        timeKey << data.timeKeyPrefix << "/value";
        outAdd["time"] = addRouteInfo.TrySelect(timeKey).ForceNumber();

        outAdd["text_key"] = data.routeType;

        if (const TAvatar* avatar = ctx.Avatar(TStringBuf("route"), data.iconName))
            outAdd["icon"] = avatar->Https;
        cardData.Push(outAdd);
    }

    return TResultValue();
}

TResultValue TRouteFormHandler::AddDivCardBlockCar(TContext& ctx, NSc::TValue& cardData, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to, const NSc::TValue& imageUrl, const TRouteOptions opts) {
    ShowRouteDivCardData data;
    data.from = from;
    data.to = to;
    data.via = via;
    data.routeType = TStringBuf("car");
    data.iconName = TStringBuf("route_icon_car");
    data.timeKeyPrefix = TStringBuf("jams_time");
    return AddDivCardBlockByRouteType(ctx, cardData, data, opts, imageUrl);
}

TResultValue TRouteFormHandler::AddDivCardBlockPedestrian(TContext& ctx, NSc::TValue& cardData, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to) {
    ShowRouteDivCardData data;
    data.from = from;
    data.to = to;
    data.via = via;
    data.routeType = TStringBuf("pedestrian");
    data.iconName = TStringBuf("route_icon_pedestrian");
    data.showJams = TStringBuf("false");
    return AddDivCardBlockByRouteType(ctx, cardData, data);
}

TResultValue TRouteFormHandler::AddDivCardBlockPublicTransport(TContext& ctx, NSc::TValue& cardData, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to) {
    ShowRouteDivCardData data;
    data.from = from;
    data.to = to;
    data.via = via;
    data.routeType = TStringBuf("public_transport");
    data.iconName = TStringBuf("route_icon_ot");
    return AddDivCardBlockByRouteType(ctx, cardData, data);
}

TResultValue TRouteFormHandler::TryAddAllDivCardBlocks(TContext& ctx, NSc::TValue& cardData, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to) {
    const TContext::TSlot *slotRouteInfo = ctx.GetSlot("route_info", "route_info");
    bool isPedestrianFirst = false;
    bool hasCarRoute = !IsSlotEmpty(slotRouteInfo) && slotRouteInfo->Value.Has("car");
    bool hasPTRoute = !IsSlotEmpty(slotRouteInfo) && slotRouteInfo->Value.Has("public_transport");
    bool hasPedestrianRoute = !IsSlotEmpty(slotRouteInfo) && slotRouteInfo->Value.Has("pedestrian");
    if (ctx.HasExpFlag(TStringBuf("poi_cards_by_foot_first")) && hasPedestrianRoute) {
        double pedTime = slotRouteInfo->Value.TrySelect(TStringBuf("pedestrian/time/value")).ForceNumber(-1);
        isPedestrianFirst = pedTime >= 0 && pedTime < 5 * 60;
    }

    int successNum = 0;
    TStringBuf successType;
    TResultValue err;

    const bool showGalleryExperiment = ctx.HasExpFlag(SHOW_ROUTE_GALLERY_EXPERIMENT);
    const int maxCards = showGalleryExperiment && ctx.HasExpFlag(SHOW_ROUTE_GALLERY_LONGWAY_FIX_EXPERIMENT) ? 1 : 3;
    LOG(DEBUG) << "Limit max cards in add all div cards: " << maxCards << Endl;
    if (hasPedestrianRoute && isPedestrianFirst) {
        if (!(err = AddDivCardBlockPedestrian(ctx, cardData, from, via, to))) {
            ++successNum;
            successType = TStringBuf("pedestrian");
            ctx.AddAttention("close_destination");
        }
    }
    if (successNum < maxCards && hasCarRoute) {
        const auto img_url = slotRouteInfo->Value.TrySelect(TStringBuf("car/image_url"));
        if (!(err = AddDivCardBlockCar(ctx, cardData, from, via, to, img_url))) {
            ++successNum;
            successType = TStringBuf("car");
        }
    }
    if (successNum < maxCards && hasPTRoute && !(err = AddDivCardBlockPublicTransport(ctx, cardData, from, via, to))) {
        ++successNum;
        successType = TStringBuf("public_transport");
    }
    if (successNum < maxCards && hasPedestrianRoute && !isPedestrianFirst) {
        if (!(err = AddDivCardBlockPedestrian(ctx, cardData, from, via, to))) {
            ++successNum;
            successType = TStringBuf("pedestrian");
        }
    }

    if (err && successNum == 0)
        return err;
    TStringBuilder textCardBlockName;
    textCardBlockName << "show_route_gallery__";
    if (successNum > 1) {
        textCardBlockName << "all";
    } else {
        textCardBlockName << successType;
    }
    ctx.AddTextCardBlock(textCardBlockName);
    return TResultValue();
}

TResultValue TRouteFormHandler::PushGalleryDivCardBlock(TContext& ctx, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to, const TContext::TSlot* slotRouteType)
{
    if (!ctx.ClientFeatures().SupportsDivCardsRendering() || !ctx.HasExpFlag(SHOW_ROUTE_GALLERY_EXPERIMENT)) {
        return TResultValue();
    }
    NSc::TValue cardData;
    if (IsSlotEmpty(slotRouteType)) {
        if (TResultValue err = TryAddAllDivCardBlocks(ctx, cardData, from, via, to))
            return err;
    }
    else {
        const TStringBuf routeType = slotRouteType->Value.GetString();
        const TContext::TSlot *slotRouteInfo = ctx.GetSlot("route_info", "route_info");
        bool hasCarRoute = !IsSlotEmpty(slotRouteInfo) && slotRouteInfo->Value.Has("car");
        bool hasPTRoute = !IsSlotEmpty(slotRouteInfo) && slotRouteInfo->Value.Has("public_transport");
        bool hasPedestrianRoute = !IsSlotEmpty(slotRouteInfo) && slotRouteInfo->Value.Has("pedestrian");
        if (routeType == "auto") {
            if (!hasCarRoute)
                return TError(TError::EType::SYSTEM, TStringBuf("Slot route_info for car is empty"));
            const auto img_url = slotRouteInfo->Value.TrySelect(TStringBuf("car/image_url"));
            if (TResultValue err = AddDivCardBlockCar(ctx, cardData, from, via, to, img_url, TRouteOptions{ADDITIONAL_ROUTES}))
                return err;
            ctx.AddTextCardBlock("show_route_gallery__car");
        } else if (routeType == "public_transport") {
            if (!hasPTRoute)
                return TError(TError::EType::SYSTEM, TStringBuf("Slot route_info for public transport is empty"));
            if (TResultValue err = AddDivCardBlockPublicTransport(ctx, cardData, from, via, to))
                return err;
            ctx.AddTextCardBlock("show_route_gallery__public_transport");
        } else if (routeType == "pedestrian") {
            if (!hasPedestrianRoute)
                return TError(TError::EType::SYSTEM, TStringBuf("Slot route_info for pedestrian is empty"));
            if (TResultValue err = AddDivCardBlockPedestrian(ctx, cardData, from, via, to))
                return err;
            ctx.AddTextCardBlock("show_route_gallery__pedestrian");
        } else {
            if (!(hasCarRoute || hasPedestrianRoute || hasPTRoute))
                return TError(TError::EType::SYSTEM, TStringBuf("Slot route_info is empty"));
            if (TResultValue err = TryAddAllDivCardBlocks(ctx, cardData, from, via, to))
                return err;
        }
    }
    NSc::TValue data;
    data["show_route_data"] = cardData;
    ctx.AddDivCardBlock("show_route_gallery", data);
    return TResultValue();
}
}

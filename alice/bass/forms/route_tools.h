#pragma once

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/geo_resolver.h>
#include <alice/bass/util/error.h>

#include <library/cpp/scheme/scheme.h>

namespace NBASS {

class TRouteResolver {
public:
    TMaybe<TGeoPosition> FromPosition;
    TMaybe<TGeoPosition> SearchPos;
    TMaybe<TString> SearchText;


public:
    /**
     * @param[in] Context - context object
     **/
    TRouteResolver(TContext& Context);

    /**
     Resolve location based on default field where_*, what_* and save result to resolved_location_*
     ResolveLocationTo: (where_to, what_to, resolved_location_to)
     ResolveLocationFrom: (where_from, what_from, resolved_location_from)
     ResolveLocationVia: (where_via, what_via, resolved_location_via)
     * @param[out] location - resolved location
     * @param[out] anotherLocation - second resolved location for suggest
     * @param[out] roadName - store road name if such road exist in GeoCoder (used then for better via-location resolving)
     * @return error object if no location resolved or something else went wrong
     */
    TResultValue ResolveLocationFrom(NSc::TValue* location, NSc::TValue* anotherLocation);
    TResultValue ResolveLocationTo(NSc::TValue* location, NSc::TValue* anotherLocation);
    TResultValue ResolveLocationUnknown(NSc::TValue* location, NSc::TValue* anotherLocation);
    TResultValue ResolveLocationVia(NSc::TValue* location, NSc::TValue* anotherLocation, TString* roadName = nullptr);

    /** Resolves via location by given route start/end and via-road name
     * @param[in] context - context object
     * @param[in] fromPosition - route start position
     * @param[in] toPosition - route end position
     * @param[out] viaPosition - resolved via coordinates if exist
     * @return error object if no location resolved or something else went wrong
     */
    TResultValue ResolveViaLocationByRoadName(TStringBuf roadName,
                                              const TGeoPosition& fromPosition,
                                              const TGeoPosition& toPosition,
                                              TGeoPosition* viaPosition);

private:
    TContext& Context;
    TMaybe<TGeoPosition> UserPosition;
    NGeobase::TId UserGeoid;

private:
    /**
     Resolves location by given what and where slot names, put it into slot locationSlotName
     * @param[in] whatSlotName - what to find (slot may contain values like "аптека", "кафе")
     * @param[in] whereSlotName - where to search (slot may contain address or some special value ("home", ""))
     * @param[in] locationSlotName - slot to save resolved location
     * @param[in] searchPos - position relative to which the search will occur
     * @return error object if no location resolved or something else went wrong
     */

    TResultValue ResolveLocation(TStringBuf whatSlotName, TStringBuf whereSlotName, TStringBuf locationSlotName,
                                 NSc::TValue* location, NSc::TValue* anotherLocation, TString* roadName = nullptr);
    TResultValue ResolveLocationImpl(TStringBuf whatSlotName, TStringBuf whereSlotName, NSc::TValue* firstLocation,
                                     NSc::TValue* secondLocation, TString* roadName);
};

/** Resolves location given in saved address (user's home or work)
 * @param ctx - context object
 * @param address - user home/work address from DataSync or Navigator state
 * @return 'geo' slot value
 */
NSc::TValue SavedAddressToGeo(TContext& ctx, const TSavedAddress& address);

TString GenerateNavigatorUri(TContext& ctx, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to, const TString& fallbackUrl = TString());
TString GenerateRouteUri(TContext& ctx, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to, TStringBuf routeType = TStringBuf(""));
TString GenerateRouteUri(TContext& ctx, const NSc::TValue& from, const NSc::TValue& to, TStringBuf routeType = TStringBuf(""));

/** Get information about routes between two points.
 * Three types of routes are supported: car, public transport and pedestrian.
 * @param[in] ctx - context object
 * @param[in] from - start point (as NSc::TValue, containing value from slot of "poi" or "geo" type)
 * @param[in] via - via point (as NSc::TValue, containing value from slot of "poi" or "geo" type)
 * @param[in] to - destination point (as NSc::TValue, containing value from slot of "poi" or "geo" type)
 * @param[out] routeInfo - information about found routes
 * @return error object if all requests to route APIs have failed or empty maybe otherwise
 */
TResultValue GetRouteInfo(TContext& ctx, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to,
                          NSc::TValue* routeInfo, const int carRoutesCount = 1, const bool carRouteImg = false);
TResultValue GetRouteInfo(TContext& ctx, const NSc::TValue& from, const NSc::TValue& to, NSc::TValue* routeInfo);

TResultValue ResolveStaticMapRouter(TContext& ctx, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to,
                                    TStringBuf routingMode, NSc::TValue& result, TStringBuf showJams = "true",
                                    const int routeIndex = 0);

/** Resolves search position by given data. Also resolve user's location like "home"
 * @param[in] context - context object
 * @param[in] whereSlotName - where to search (slot may contain address or some special value ("home", ""))
 * @param[in] UserPosition - user position if known
 * @param[in] fromPosition - route start position
 * @param[out] searchPos - resolved search position
 * @return attention object if user's position has not been resolved
 */
TResultValue SpecifySearchPos(TContext& context, TStringBuf whereSlotName,
                              const TMaybe<TGeoPosition>& userPosition, const TMaybe<TGeoPosition>& fromPosition,
                              TMaybe<TGeoPosition>& searchPos);

/* Convert coords from TGeoPosition to geo structure */
NSc::TValue LocationToGeo(TContext& ctx, const TGeoPosition& location);
}

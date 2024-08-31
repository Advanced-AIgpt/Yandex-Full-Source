#include "route_intents.h"

#include <alice/bass/forms/geo_resolver.h>
#include <maps/doc/proto/yandex/maps/proto/common2/response.pb.h>
#include <maps/doc/proto/yandex/maps/proto/common2/geo_object.pb.h>
#include <maps/doc/proto/yandex/maps/proto/search/route_point.pb.h>


namespace NBASS {

namespace {

constexpr TStringBuf FROM = "from";
constexpr TStringBuf VIA = "via_0";
constexpr TStringBuf TO = "to";
constexpr TStringBuf LAT = "lat";
constexpr TStringBuf LON = "lon";
constexpr TStringBuf CONTEXT = "context";
constexpr TStringBuf ROUTE_POINT_CONTEXT = "route_point_context";

using namespace yandex::maps::proto::common2;
using namespace yandex::maps::proto::search;

} //namespace

TBuildRouteNavigatorIntent::TBuildRouteNavigatorIntent(TContext& ctx, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to)
    : INavigatorIntent(ctx, TStringBuf("build_route_on_map") /* scheme */)
    , RoutePoints({{FROM, from}, {TO, to}, {VIA, via}})
{}

NHttpFetcher::THandle::TRef TBuildRouteNavigatorIntent::CreateRoutePointContextRequest(NSc::TValue& location) {
    if (location.IsNull()) {
        return {};
    }

    NHttpFetcher::TRequestPtr request = Context.GetSources().GeoMetaSearchRoutePointContext().AttachRequest(MultiRequest,
        [&location](
            TStringBuf responseData) {
            response::Response resp;
            Y_PROTOBUF_SUPPRESS_NODISCARD resp.ParseFromString(TString{responseData});
            TVector<NSc::TValue> results;
            for (const auto& geoObject : resp.reply().geo_object()) {
                for (const auto& metadata : geoObject.metadata()) {
                    if (metadata.HasExtension(route_point::ROUTE_POINT_METADATA)) {
                        auto& ext = metadata.GetExtension(route_point::ROUTE_POINT_METADATA);
                        location[ROUTE_POINT_CONTEXT].SetString(ext.route_point_context());
                        break;
                    }
                }
            }
            return NHttpFetcher::TFetchStatus::Success();
        });

    request->AddCgiParam(TStringBuf("origin"), GEOSEARCH_ORIGIN);
    request->AddCgiParam(TStringBuf("lang"), Context.Meta().Lang());
    request->AddCgiParam(TStringBuf("mode"), TStringBuf("reverse"));
    request->AddCgiParam(TStringBuf("ms"), TStringBuf("pb"));

    request->AddCgiParam(TStringBuf("type"), TStringBuf("geo"));
    request->AddCgiParam(TStringBuf("results"), TStringBuf("1"));
    request->AddCgiParam(TStringBuf("snippets"), TStringBuf("route_point/1.x"));

    request->AddCgiParam(TStringBuf("ll"), TGeoPosition::FromJson(location)->GetLonLatString());

    return request->Fetch();
}

void TBuildRouteNavigatorIntent::RequestRoutePointContext() {
    MultiRequest = NHttpFetcher::WeakMultiRequest();

    THashMap<TStringBuf, NHttpFetcher::THandle::TRef> pointContextRequests;
    for (auto& point : RoutePoints) {
        pointContextRequests.insert({point.first, CreateRoutePointContextRequest(point.second)});
    }

    MultiRequest->WaitAll();

    for (auto& request : pointContextRequests) {
        if (request.second) {
            request.second->Wait();
        }
    }
}

TResultValue TBuildRouteNavigatorIntent::SetupSchemeAndParams() {
    RequestRoutePointContext();

    for (auto& point : RoutePoints) {
        AddPointParam(point.first /* pointName */, point.second /* point */);
    }

    if (!Params.empty()) {
        Params.InsertUnescaped(TStringBuf("confirmation"), TStringBuf("1"));
    }

    AddShortAnswerBlock();

    return TResultValue();
}

TDirectiveFactory::TDirectiveIndex TBuildRouteNavigatorIntent::GetDirectiveIndex() {
    return GetAnalyticsTagIndex<TNavigatorBuildRouteDirective>();
}

void TBuildRouteNavigatorIntent::AddPointParam(TStringBuf pointName, const NSc::TValue& point) {
    if (point.IsNull()) {
        return;
    }
    Params.InsertUnescaped(TStringBuilder() << LAT << "_" << pointName, ToString(point[LAT]));
    Params.InsertUnescaped(TStringBuilder() << LON << "_" << pointName, ToString(point[LON]));
    if (TStringBuf routePointContext = point.TrySelect(ROUTE_POINT_CONTEXT).GetString()) {
        Params.InsertUnescaped(TStringBuilder() << CONTEXT << "_" << pointName, routePointContext);
    }
}

TResultValue TBuildRouteNavigatorIntent::Do() {
    if (Context.HasExpFlag(NAVIGATOR_ALICE_CONFIRMATION)) {
        TContext::TSlot* confirmationSlot = Context.GetOrCreateSlot("confirmation", "confirmation");
        if (IsSlotEmpty(confirmationSlot)) {
            confirmationSlot->Optional = false;

            Context.AddSuggest(TStringBuf("show_route__confirm"));
            Context.AddSuggest(TStringBuf("show_route__decline"));
            return INavigatorIntent::Do();
        }
        // route.cpp has a function CheckConfirmation that checks the case when the slot is not empty
    }
    return INavigatorIntent::Do();
}

}

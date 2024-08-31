#include "taxi.h"

#include "directives.h"
#include "geo_resolver.h"
#include "geodb.h"
#include "special_location.h"
#include "urls_builder.h"

#include <alice/bass/forms/route_tools.h>

#include <alice/bass/libs/globalctx/globalctx.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <kernel/geodb/countries.h>

namespace NBASS {

namespace {
NGeobase::TId GetLocationGeoId(const NSc::TValue& location) {
    if (!location.IsNull()) {
        const NSc::TValue& geoLocation = location.Has("geo") ? location["geo"] : location;
        return geoLocation["geoid"].GetIntNumber(NGeobase::UNKNOWN_REGION);
    }

    return NGeobase::UNKNOWN_REGION;
}

bool IsTaxiAvailable(const NGeobase::TLookup& geobase, NGeobase::TId regionId) {
    return !NAlice::IsValidId(regionId) || geobase.IsIdInRegion(regionId, NGeoDB::RUSSIA_ID);
}

void CopySlot(TContext::TSlot* oldSlot, TStringBuf newSlotName, NSc::TValue* formUpdate) {
    if (!IsSlotEmpty(oldSlot)) {
        TContext::TSlot newSlot(newSlotName, oldSlot->Type);
        newSlot.Value = oldSlot->Value;
        formUpdate->GetOrAdd("slots").SetArray().Push(newSlot.ToJson(nullptr));
    }
}

} // end anon namespace

static constexpr TStringBuf TAXI_ORDER = "personal_assistant.scenarios.taxi_order";
static constexpr TStringBuf TAXI_ORDER_ELLIPSIS = "personal_assistant.scenarios.taxi_order__ellipsis";

TResultValue TTaxiOrderHandler::Do(TRequestHandler& r) {
    auto& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::TAXI);
    if (ctx.MetaClientInfo().IsSmartSpeaker() || ctx.MetaClientInfo().IsYaAuto()) {
        ctx.AddAttention(TStringBuf("taxi_not_supported_on_device"), {} /* data */);
        ctx.AddOnboardingSuggest();
        return TResultValue();
    }

    const auto result = DoImpl(r);
    ctx.AddSearchSuggest();
    ctx.AddOnboardingSuggest();
    return result;
}

void TTaxiOrderHandler::Register(THandlersMap* handlers) {
    auto cbTaxiOrderForm = []() {
        return MakeHolder<TTaxiOrderHandler>();
    };

    handlers->emplace(TAXI_ORDER, cbTaxiOrderForm);
    handlers->emplace(TAXI_ORDER_ELLIPSIS, cbTaxiOrderForm);
}

void TTaxiOrderHandler::AddTaxiSuggest(TContext& context, TStringBuf fromSlotName, TStringBuf toSlotName)
{
    if (context.MetaClientInfo().IsSmartSpeaker()) {
        return;
    }

    TContext::TSlot* fromSlot = context.GetSlot(fromSlotName);
    TContext::TSlot* toSlot = context.GetSlot(toSlotName);

    // will add taxi suggest only in Russia
    const NGeobase::TLookup& geobase = context.GlobalCtx().GeobaseLookup();
    NGeobase::TId fromRegion = IsSlotEmpty(fromSlot) ? NGeobase::UNKNOWN_REGION : GetLocationGeoId(fromSlot->Value);
    NGeobase::TId toRegion = IsSlotEmpty(toSlot) ? NGeobase::UNKNOWN_REGION : GetLocationGeoId(toSlot->Value);

    if (!IsTaxiAvailable(geobase, context.UserRegion())
        || !IsTaxiAvailable(geobase, fromRegion)
        || !IsTaxiAvailable(geobase, toRegion)) {
        return;
    }

    NSc::TValue formUpdate;
    formUpdate["name"] = TAXI_ORDER;

    CopySlot(fromSlot, "resolved_location_from", &formUpdate);
    CopySlot(toSlot, "resolved_location_to", &formUpdate);

    context.AddSuggest(TStringBuf("taxi_order_fallback"), NSc::Null(), formUpdate);
}

TResultValue TTaxiOrderHandler::DoImpl(TRequestHandler& r) {
    // First of all we need to know destination
    TContext& ctx = r.Ctx();
    TContext::TSlot* checkWhereTo = ctx.GetSlot("where_to");
    const TContext::TSlot* checkWhatTo = ctx.GetSlot("what_to");
    const TContext::TSlot* checkResolvedLocationTo = ctx.GetSlot("resolved_location_to");
    if (IsSlotEmpty(checkWhatTo) && IsSlotEmpty(checkWhereTo) && IsSlotEmpty(checkResolvedLocationTo)) {
        if (!checkWhereTo) {
            checkWhereTo = ctx.CreateSlot("where_to", "string");
        }
        checkWhereTo->Optional = false;
        return TResultValue();
    }

    // TODO: probably we should do it asynchronously (parallel to other requests), but the code will become complicated
    TRouteResolver routeResolver = TRouteResolver(ctx);

    // Resolve location_from
    NSc::TValue locationFrom, anotherFrom;
    const TContext::TSlot* slotInputResolvedFrom = ctx.GetSlot("resolved_location_from");
    const TContext::TSlot* slotWhatFrom = ctx.GetSlot("what_from");
    const TContext::TSlot* slotWhereFrom = ctx.GetSlot("where_from");
    if (!IsSlotEmpty(slotInputResolvedFrom) && slotInputResolvedFrom->Value.Has("location")) {
        locationFrom = slotInputResolvedFrom->Value;
    } else if (!IsSlotEmpty(slotWhatFrom) || !IsSlotEmpty(slotWhereFrom)) {
        if (const TResultValue err = routeResolver.ResolveLocationFrom(&locationFrom, &anotherFrom))
        {
            return err;
        }

        if (locationFrom.IsNull()) {
            // Failed to resolve location_from, error block was already added
            return TResultValue();
        }
    }

    // Resolve location_to
    NSc::TValue locationTo, anotherTo;

    const TContext::TSlot* slotInputResolvedTo = ctx.GetSlot("resolved_location_to");
    const TContext::TSlot* slotWhatTo = ctx.GetSlot("what_to");
    const TContext::TSlot* slotWhereTo = ctx.GetSlot("where_to");
    if (!IsSlotEmpty(slotInputResolvedTo) && slotInputResolvedTo->Value.Has("location")) {
        locationTo = slotInputResolvedTo->Value;
    } else if (!IsSlotEmpty(slotWhatTo) || !IsSlotEmpty(slotWhereTo)){
        if (locationFrom.Has("location")) {
            // Use resolved location_from as the start point for location_to
            const NSc::TValue& location = locationFrom["location"];
            routeResolver.FromPosition = TGeoPosition(location["lat"].GetNumber(), location["lon"].GetNumber());
        }
        if (const TResultValue err = routeResolver.ResolveLocationTo(&locationTo, &anotherTo))
        {
            return err;
        }

        if (locationTo.IsNull()) {
            // Failed to resolve location_to, error block was already added
            return TResultValue();
        }
    }

    // we do not use user location as start point for taxi order
    if (IsSlotEmpty(slotWhatFrom) && TSpecialLocation::IsNearLocation(slotWhereFrom)) {
        locationFrom.SetNull();
    }

    TString taxiUri = GenerateTaxiUri(ctx, locationFrom, locationTo);

    // add command block and suggest
    NSc::TValue taxiOrderData;
    taxiOrderData["uri"].SetString(taxiUri);
    ctx.AddCommand<TTaxiOpenAppWithOrderDirective>(TStringBuf("open_uri"), taxiOrderData);
    ctx.AddSuggest(TStringBuf("taxi_order__open_app"), taxiOrderData);

    return TResultValue();
}

}

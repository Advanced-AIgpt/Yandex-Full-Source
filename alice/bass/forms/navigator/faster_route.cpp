#include "faster_route.h"

#include "route_intents.h"

#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/route.h>
#include <alice/bass/forms/route_helpers.h>
#include <util/string/cast.h>

#include <alice/library/analytics/common/product_scenarios.h>

namespace NBASS {

namespace {
class TSwitchToFasterAlternativeIntent : public INavigatorIntent {
public:
    TSwitchToFasterAlternativeIntent(TContext& ctx, TStringBuf routeId, bool needConfirmation)
        : INavigatorIntent(ctx, TStringBuf("switch_to_faster_alternative") /* scheme */)
        , RouteId(routeId)
        , NeedConfirmation(needConfirmation)
    {}

protected:
    TResultValue SetupSchemeAndParams() override {
        Params.InsertUnescaped(TStringBuf("need_confirmation"), ToString(NeedConfirmation));
        Params.InsertUnescaped(TStringBuf("route_id"), ToString(RouteId));

        if (NeedConfirmation) {
            ListeningIsPossible = true;
        }
        return TResultValue();
    }

    TStringBuf RouteId;
    bool NeedConfirmation;

    TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() override {
        return GetAnalyticsTagIndex<TNavigatorSwitch2FasterAlternativeDirective>();
    }
};
} // namespace

TResultValue TNavigatorFasterRouteHandler::Do(TRequestHandler& r) {
    auto& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::NAVI_COMMANDS);

    TContext::TSlot* confirmationSlot = ctx.GetOrCreateSlot(TStringBuf("confirmation"), TStringBuf("confirmation"));

    if (IsSlotEmpty(confirmationSlot)) {
        confirmationSlot->Optional = false;

        ctx.AddSuggest(TStringBuf("faster_route__confirm"));
        ctx.AddSuggest(TStringBuf("faster_route__decline"));
        ctx.AddSuggest(TStringBuf("faster_route__show"));

        return TResultValue();
    }

    TStringBuf confirmation = confirmationSlot->Value.GetString();
    TContext::TSlot* routeIdSlot = ctx.GetOrCreateSlot(TStringBuf("route_id"), TStringBuf("string"));
    TStringBuf routeId = routeIdSlot->Value.GetString();

    bool hasWaitingState = NRoute::CheckNavigatorState(ctx, NRoute::WAITING_STATE);

    if (confirmation == "yes" && !hasWaitingState) {
        TSwitchToFasterAlternativeIntent navigatorIntentHandler(ctx, routeId, false /* needConfirmation */);
        return navigatorIntentHandler.Do();
    }
    if (confirmation == "no" && !hasWaitingState) {
        return TResultValue();
    }
    if (confirmation == "yes" && hasWaitingState) {
        TExternalConfirmationIntent navigatorIntentHandler(ctx, true /* isConfirmed */);
        return navigatorIntentHandler.Do();
    }
    if (confirmation == "no" && hasWaitingState) {
        TExternalConfirmationIntent navigatorIntentHandler(ctx, false /* isConfirmed */);
        return navigatorIntentHandler.Do();
    }

    if (confirmation == "show") {
        ctx.AddSuggest(TStringBuf("faster_route__confirm"));
        ctx.AddSuggest(TStringBuf("faster_route__decline"));
        TSwitchToFasterAlternativeIntent navigatorIntentHandler(ctx, routeId, true /* needConfirmation */);
        return navigatorIntentHandler.Do();
    }

    return TResultValue();
}

void TNavigatorFasterRouteHandler::Register(THandlersMap* handlers) {
    auto cbNavigatorFasterRouteForm = []() {
        return MakeHolder<TNavigatorFasterRouteHandler>();
    };

    handlers->emplace(NAVI_FASTER_ROUTE, cbNavigatorFasterRouteForm);
    handlers->emplace(NAVI_FASTER_ROUTE_ELLIPSIS, cbNavigatorFasterRouteForm);
}

}

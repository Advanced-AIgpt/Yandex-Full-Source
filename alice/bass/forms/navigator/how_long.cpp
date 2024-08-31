#include "how_long.h"

#include <alice/bass/forms/route.h>

#include <alice/library/analytics/common/product_scenarios.h>

namespace NBASS {

TResultValue TNavigatorHowLongHandler::Do(TRequestHandler& r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::NAVI_COMMANDS);

    const TString& formName = r.Ctx().FormName();

    const auto& currentRoute = r.Ctx().Meta().DeviceState().NavigatorState().CurrentRoute();

    if (currentRoute.IsNull()) {
        if (formName == NAVI_HOW_LONG_TRAFFIC_JAM) {
            r.Ctx().AddErrorBlock(TError::EType::NOCURRENTROUTE, TStringBuf("No current route in Navigator state"));
            return TResultValue();
        } else {
            return TRouteFormHandler::SetAsResponse(r.Ctx());
        }
    }

    if (r.Ctx().MetaClientInfo().IsYaAuto()) {
        r.Ctx().AddStopListeningBlock();
    }

    if (formName == NAVI_WHEN_WE_GET) {
        if (currentRoute.HasArrivalTimestamp()) {
            TContext::TSlot* distanceSlot = r.Ctx().GetOrCreateSlot("arrival_timestamp", "num");
            distanceSlot->Value = currentRoute.ArrivalTimestamp();
        }
        return TResultValue();
    }

    if (formName == NAVI_HOW_LONG_DRIVE) {
        if (currentRoute.HasDistanceToDestination()) {
            TContext::TSlot* distanceSlot = r.Ctx().GetOrCreateSlot("distance_to_destination", "num");
            distanceSlot->Value = currentRoute.DistanceToDestination();
        }
        if (currentRoute.HasTimeToDestination()) {
            TContext::TSlot* timeSlot = r.Ctx().GetOrCreateSlot("time_to_destination", "num");
            timeSlot->Value = currentRoute.TimeToDestination();
        }

        return TResultValue();
    }

    if (formName == NAVI_HOW_LONG_TRAFFIC_JAM) {
        TContext::TSlot* distanceSlot = r.Ctx().GetOrCreateSlot("distance_in_traffic_jam", "num");
        if (currentRoute.HasDistanceInTrafficJam()) {
            distanceSlot->Value = currentRoute.DistanceInTrafficJam();
        } else {
            distanceSlot->Value.SetIntNumber(0);
        }

        TContext::TSlot* timeSlot = r.Ctx().GetOrCreateSlot("time_in_traffic_jam", "num");
        if (currentRoute.HasTimeInTrafficJam()) {
            timeSlot->Value = currentRoute.TimeInTrafficJam();
        } else {
            timeSlot->Value.SetIntNumber(0);
        }

        return TResultValue();
    }

    return TResultValue();
}

//static
TResultValue TNavigatorHowLongHandler::SetAsResponse(TContext& ctx, TStringBuf formName ) {
    ctx.SetResponseForm(formName, false /* setCurrentFormAsCallback */);
    return ctx.RunResponseFormHandler();
}

void TNavigatorHowLongHandler::Register(THandlersMap* handlers) {
    auto cbNavigatorHowLongForm = []() {
        return MakeHolder<TNavigatorHowLongHandler>();
    };

    handlers->emplace(NAVI_WHEN_WE_GET, cbNavigatorHowLongForm);
    handlers->emplace(NAVI_HOW_LONG_DRIVE, cbNavigatorHowLongForm);
    handlers->emplace(NAVI_HOW_LONG_TRAFFIC_JAM, cbNavigatorHowLongForm);
}

}

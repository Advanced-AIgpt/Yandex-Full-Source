#include "route_manager_common.h"

namespace NAlice::NHollywoodFw::NRouteManager {

namespace {

void AddSuggest(TRender& render, const TStringBuf& suggestPhraseName, const TStringBuf& imageUrl) {
    render.AddSuggestion(ROUTE_MANAGER_NLG, suggestPhraseName, "", SUGGEST_TYPE, imageUrl);
}

void AddSwitchLayoutSuggest(TRender& render, const TRouteManagerCapability::TState& state) {
    if (state.GetLayout() == TRouteManagerCapability::TState::Map) {
        AddSuggest(render, SUGGEST_NLG_SHOW_RIDE, SUGGEST_ICON_SHOW_RIDE);
    } else if (state.GetLayout() == TRouteManagerCapability::TState::Ride) {
        AddSuggest(render, SUGGEST_NLG_SHOW_ROUTE, SUGGEST_ICON_SHOW_ROUTE);
    }
}

} // namespace

void AddSuggests(TRender& render, const TRouteManagerCapability::TState& state) {
    switch (state.GetRoute()) {
        case TRouteManagerCapability::TState::UnknownRoute:
            AddSuggest(render, SUGGEST_NLG_CALL_SUPPORT, SUGGEST_ICON_CALL_SUPPORT);
            break;
        case TRouteManagerCapability::TState::Stopped:
            AddSuggest(render, SUGGEST_NLG_CALL_SUPPORT, SUGGEST_ICON_CALL_SUPPORT);
            AddSuggest(render, SUGGEST_NLG_CONTINUE, SUGGEST_ICON_START);
            AddSwitchLayoutSuggest(render, state);
            break;
        case TRouteManagerCapability::TState::Stopping:
            AddSuggest(render, SUGGEST_NLG_CALL_SUPPORT, SUGGEST_ICON_CALL_SUPPORT);
            AddSuggest(render, SUGGEST_NLG_CONTINUE, SUGGEST_ICON_START);
            AddSwitchLayoutSuggest(render, state);
            break;
        case TRouteManagerCapability::TState::Moving:
            AddSuggest(render, SUGGEST_NLG_CALL_SUPPORT, SUGGEST_ICON_CALL_SUPPORT);
            AddSuggest(render, SUGGEST_NLG_STOP, SUGGEST_ICON_STOP);
            AddSwitchLayoutSuggest(render, state);
            break;
        case TRouteManagerCapability::TState::WaitingPassenger:
            AddSuggest(render, SUGGEST_NLG_CALL_SUPPORT, SUGGEST_ICON_CALL_SUPPORT);
            AddSuggest(render, SUGGEST_NLG_START, SUGGEST_ICON_START);
            break;
        case TRouteManagerCapability::TState::Finished:
            AddSuggest(render, SUGGEST_NLG_CALL_SUPPORT, SUGGEST_ICON_CALL_SUPPORT);
            break;
        case TRouteManagerCapability_TState_TRoute_TRouteManagerCapability_TState_TRoute_INT_MIN_SENTINEL_DO_NOT_USE_:
        case TRouteManagerCapability_TState_TRoute_TRouteManagerCapability_TState_TRoute_INT_MAX_SENTINEL_DO_NOT_USE_:
            AddSuggest(render, SUGGEST_NLG_CALL_SUPPORT, SUGGEST_ICON_CALL_SUPPORT);
            break;
    }
}

}  // namespace NAlice::NHollywoodFw::NRouteManager

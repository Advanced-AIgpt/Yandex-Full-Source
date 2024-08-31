#pragma once

#include <alice/hollywood/library/framework/framework.h>

#include <alice/protos/endpoint/capabilities/route_manager/route_manager.pb.h>

namespace NAlice::NHollywoodFw::NRouteManager {

inline constexpr TStringBuf SUGGEST_ICON_START = "https://static-alice.s3.yandex.net/scenarios/route_manager/start_ride.png";
inline constexpr TStringBuf SUGGEST_ICON_STOP = "https://static-alice.s3.yandex.net/scenarios/route_manager/stop_ride.png";
inline constexpr TStringBuf SUGGEST_ICON_SHOW_RIDE = "https://static-alice.s3.yandex.net/scenarios/route_manager/show_ride.png";
inline constexpr TStringBuf SUGGEST_ICON_SHOW_ROUTE = "https://static-alice.s3.yandex.net/scenarios/route_manager/show_route.png";
inline constexpr TStringBuf SUGGEST_ICON_CALL_SUPPORT = "https://static-alice.s3.yandex.net/scenarios/route_manager/call_support.png";

inline constexpr TStringBuf ROUTE_MANAGER_NLG = "route_manager";
inline constexpr TStringBuf SUGGEST_NLG_START = "suggest_start";
inline constexpr TStringBuf SUGGEST_NLG_STOP = "suggest_stop";
inline constexpr TStringBuf SUGGEST_NLG_SHOW_RIDE = "suggest_show_ride";
inline constexpr TStringBuf SUGGEST_NLG_SHOW_ROUTE = "suggest_show_route";
inline constexpr TStringBuf SUGGEST_NLG_CALL_SUPPORT = "suggest_call_to_operator";
inline constexpr TStringBuf SUGGEST_NLG_CONTINUE = "suggest_continue";

inline constexpr TStringBuf SUGGEST_TYPE = "route_manager_suggest";

void AddSuggests(TRender& render, const TRouteManagerCapability::TState& state);

}  // namespace NAlice::NHollywoodFw::NRouteManager

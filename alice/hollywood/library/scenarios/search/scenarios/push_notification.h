#pragma once

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/scenarios/search/context/context.h>
#include <alice/hollywood/library/scenarios/search/proto/apply_arguments.pb.h>

namespace NAlice::NHollywood::NSearch {

inline constexpr TStringBuf SEARCH_PUSH_TITLE = "Алиса";
inline constexpr TStringBuf ICON_ALICE = "https://yastatic.net/s3/home/apisearch/alice_icon.png";
inline constexpr TStringBuf ICON_ID = "2";
inline constexpr TStringBuf PUSH_ID = "alice_web_search_push";
inline constexpr TStringBuf PUSH_PROJECT = "bass";
inline constexpr TStringBuf BELL_TYPE = "search_push";
inline constexpr TStringBuf THROTTLE_ID = "alice_web_search_policy";
inline constexpr TStringBuf CLIENT_IDS[] = {TStringBuf("ru.yandex.mobile"), TStringBuf("ru.yandex.searchplugin"), TStringBuf("bell")};
inline constexpr TStringBuf SERP_TITLE_TEXT = "Вы спрашивали у Алисы в Станции";
inline constexpr TStringBuf SITE_TITLE_TEXT = "Нажмите, чтобы перейти на ";


bool PreparePushRequests(const TScenarioRunRequestWrapper& request,
    TRunResponseBuilder& builder, TScenarioHandleContext& ctx, const TString& query, const TString& link);
THttpProxyRequest CreateSupProxyRequest(const TSearchApplyArguments& applyArgs, TScenarioHandleContext& ctx);

} // namespace NAlice::NHollywood

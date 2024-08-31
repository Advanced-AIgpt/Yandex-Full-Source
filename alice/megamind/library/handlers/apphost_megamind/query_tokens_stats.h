#pragma once

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/kv_saas/response.h>
#include <alice/megamind/library/request_composite/client/client.h>
#include <alice/megamind/library/request_composite/view.h>
#include <alice/megamind/library/util/status.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice::NMegamind {

inline constexpr TStringBuf AH_ITEM_QUERY_TOKENS_STATS_HTTP_REQUEST_NAME = "mm_query_tokens_stats_http_request";
inline constexpr TStringBuf AH_ITEM_QUERY_TOKENS_STATS_HTTP_RESPONSE_NAME = "mm_query_tokens_stats_http_response";

TStatus AppHostQueryTokensStatsSetup(IAppHostCtx& ahCtx, const TString& utterance, TRequestComponentsView<TClientComponent> view);
TStatus AppHostQueryTokensStatsPostSetup(IAppHostCtx& ahCtx, NKvSaaS::TTokensStatsResponse& protoResponse);

} // namespace NAlice::NMegamind

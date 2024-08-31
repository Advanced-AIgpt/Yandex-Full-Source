#include "query_tokens_stats.h"

#include "node.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/request_builder.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/kv_saas/request.h>

namespace NAlice::NMegamind {

TStatus AppHostQueryTokensStatsSetup(IAppHostCtx& ahCtx, const TString& utterance, TRequestComponentsView<TClientComponent> view) {
    TAppHostHttpProxyMegamindRequestBuilder request;
    ESourcePrepareType status;
    if (auto err = NKvSaaS::CreateQueryTokensStatsRequest(utterance, view, request).MoveTo(status)) {
        return std::move(*err);
    }

    if (status == ESourcePrepareType::Succeeded) {
        request.CreateAndPushRequest(ahCtx, AH_ITEM_QUERY_TOKENS_STATS_HTTP_REQUEST_NAME);
    }

    return Success();
}

TStatus AppHostQueryTokensStatsPostSetup(IAppHostCtx& ahCtx, NKvSaaS::TTokensStatsResponse& protoResponse) {
    NAppHostHttp::THttpResponse httpResponse;
    if (auto err = GetFirstProtoItem(ahCtx.ItemProxyAdapter(), AH_ITEM_QUERY_TOKENS_STATS_HTTP_RESPONSE_NAME, httpResponse)) {
        return std::move(*err);
    }

    if (httpResponse.GetStatusCode() != HTTP_OK) {
        return TError{TError::EType::Critical} << "KvSaaS tokens stats returned error: " << httpResponse.ShortUtf8DebugString();
    }

    if (auto err = NKvSaaS::ParseResponse<NKvSaaS::TTokensStatsResponse>(httpResponse.GetContent()).MoveTo(protoResponse)) {
        return std::move(*err);
    }

    return Success();
}

} // namespace NAlice::NMegaamind

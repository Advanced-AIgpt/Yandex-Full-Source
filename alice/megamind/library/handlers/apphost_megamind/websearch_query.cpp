#include "websearch_query.h"

#include <alice/megamind/library/apphost_request/item_names.h>

namespace NAlice::NMegamind {

using namespace NMegamindAppHost;

namespace {

void PutQueryIntoContext(TItemProxyAdapter& itemProxyAdapter, const TString& query) {
    TWebSearchQueryProto webSearchQueryProto;
    webSearchQueryProto.SetQuery(query);
    itemProxyAdapter.PutIntoContext(webSearchQueryProto, AH_ITEM_WEB_SEARCH_QUERY);
}

} // anonymous namespace

TStatus AppHostWebSearchQuerySetup(IAppHostCtx& ahCtx, const IContext& ctx, const IEvent& event) {
    if (const auto query = ctx.Responses().WizardResponse().GetSearchQuery(ctx.ExpFlags()); query.Defined() && event.IsUserGenerated()) {
        LOG_DEBUG(ctx.Logger()) << "Using wizard result for search query: " << *query;
        PutQueryIntoContext(ahCtx.ItemProxyAdapter(), *query);
        return Success();
    }
    PutQueryIntoContext(ahCtx.ItemProxyAdapter(), event.GetUtterance());
    return Success();
}

TStatus AppHostWebSearchQueryPostSetup(IAppHostCtx& ahCtx, TWebSearchQueryProto& webSearchQueryProto) {
    auto webSearchQueryResponse = ahCtx.ItemProxyAdapter().GetFromContext<TWebSearchQueryProto>(AH_ITEM_WEB_SEARCH_QUERY);
    if (webSearchQueryResponse.IsSuccess()) {
        webSearchQueryResponse.MoveTo(webSearchQueryProto);
        return Success();
    }
    return *webSearchQueryResponse.Error();
}

} // namespace NAlice::NMegaamind

#include "websearch.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/request_builder.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/requestctx/requestctx.h>
#include <alice/megamind/library/search/search.h>

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/web_search_source.pb.h>

#include <alice/library/network/headers.h>
#include <alice/library/scenarios/data_sources/data_sources.h>

#include <util/system/guard.h>

namespace NAlice::NMegamind {

namespace {

template <typename TSearchPart>
void GetSearchPartFromContext(TItemProxyAdapter& itemAdapter, TMaybe<TSearchPart>& part, TStringBuf itemName) {
    auto errOrVal = itemAdapter.GetFromContext<TSearchPart>(itemName);
    if (errOrVal.IsSuccess()) {
        errOrVal.MoveTo(part);
    }
}

} // namespace

TStatus AppHostWebSearchSetup(IAppHostCtx& ahCtx, const TSpeechKitRequest& skr, const IEvent& event,
                              TWebSearchRequestBuilder& builder)
{
    auto& logger = ahCtx.Log();

    TAppHostHttpProxyMegamindRequestBuilder request;
    const auto status = builder.Build(skr, event, logger, request);

    if (!status.IsSuccess()) {
        return status.Status();
    }

    switch (status.Value()) {
        case ESourcePrepareType::Succeeded: {
            // TODO (petrk) A temporary quickfix. Should be a general approach.
            if (const auto currentDc = ahCtx.GlobalCtx().Config().GetCurrentDC(); !currentDc.Empty()) {
                request.AddHeader(NNetwork::HEADER_X_BALANCER_DC_HINT, currentDc);
            }
            const auto& requestProto = request.CreateAndPushRequest(ahCtx, AH_ITEM_WEBSEARCH_HTTP_REQUEST_NAME, "web search url");
            ahCtx.ItemProxyAdapter().IntermediateFlush();
            ahCtx.ItemProxyAdapter().AddFlag(AH_FLAG_EXPECT_WEBSEARCH_RESPONSE);
            if (skr.ExpFlag(EXP_DUMP_WEBSEARCH_REQUEST)) {
                LOG_INFO(ahCtx.Log()) << "WebSearch request dump: " << requestProto.ShortUtf8DebugString();
            }
            break;
        }

        case ESourcePrepareType::NotNeeded:
            break; // Default error is "not requested".
    }

    return Success();
}

TSearchResponse AppHostWebSearchPostSetup(IAppHostCtx& ahCtx) {
    TMaybe<NWebSearch::TTunnellerRawResponse> tunnellerRawResponsePart;
    TMaybe<NWebSearch::TStaticBlenderFactors> staticBlenderFactorsPart;
    TMaybe<NWebSearch::TReportGrouping> reportGroupingPart;
    TMaybe<NScenarios::TDataSource> docsDataSource;
    TMaybe<TSearchResponse::TBgFactors> bgFactorsPart;
    static const TString& webSearchDocsItemName = NScenarios::GetDataSourceContextName(EDataSourceType::WEB_SEARCH_DOCS);
    GetSearchPartFromContext(ahCtx.ItemProxyAdapter(), tunnellerRawResponsePart, AH_ITEM_SEARCH_PART_TUNNELLER_RAW_RESPONSE);
    GetSearchPartFromContext(ahCtx.ItemProxyAdapter(), staticBlenderFactorsPart, AH_ITEM_SEARCH_PART_STATIC_BLENDER_FACTORS);
    GetSearchPartFromContext(ahCtx.ItemProxyAdapter(), reportGroupingPart, AH_ITEM_SEARCH_PART_REPORT_GROUPING);
    GetSearchPartFromContext(ahCtx.ItemProxyAdapter(), docsDataSource, webSearchDocsItemName);
    GetSearchPartFromContext(ahCtx.ItemProxyAdapter(), bgFactorsPart, AH_ITEM_SEARCH_PART_BG_FACTORS);
    TSearchResponse response{
        std::move(tunnellerRawResponsePart),
        std::move(staticBlenderFactorsPart),
        std::move(reportGroupingPart),
        std::move(docsDataSource),
        std::move(bgFactorsPart),
        ahCtx.Log()
    };
    return response;
}

} // namespace NAlice::NMegaamind

#include "split_web_search.h"

#include <alice/hollywood/library/dispatcher/common_handles/util/util.h>

#include <alice/hollywood/library/metrics/metrics.h>
#include <alice/hollywood/library/util/service_context.h>

#include <alice/library/analytics/common/names.h>
#include <alice/library/json/json.h>
#include <alice/library/metrics/util.h>
#include <alice/library/scenarios/data_sources/data_sources.h>
#include <alice/library/websearch/response/response.h>
#include <alice/protos/websearch/websearch_parts.pb.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <google/protobuf/wrappers.pb.h>

namespace NAlice::NHollywood {

namespace {

const TStringBuf WEBSEARCH_HTTP_RESPONSE = "websearch_http_response";

} // anonymous namespace

// This function deliberately throws exceptions, so that we can see them in error booster
void SplitWebSearch(NAppHost::IServiceContext& ctx, TGlobalContext& globalContext) {
    const auto start = TInstant::Now();

    const auto appHostParams = GetAppHostParams(ctx);
    auto logger = CreateLogger(globalContext, GetRTLogToken(appHostParams, ctx.GetRUID()));
    try {
        const auto responseProto = ctx.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(WEBSEARCH_HTTP_RESPONSE);
        Y_ENSURE(responseProto.GetStatusCode() == 200, "Non 200 return code from WebSearch");

        const TSearchResponse searchResponse{responseProto.GetContent(), /* requestActivationId= */ "", logger, /* initDataSources= */ true};
        {
            NScenarios::TDataSource dataSource;
            *dataSource.MutableWebSearchDocs() = searchResponse.WebSearchDocs();
            ctx.AddProtobufItem(dataSource, NScenarios::GetDataSourceContextName(EDataSourceType::WEB_SEARCH_DOCS));
        }
        {
            NScenarios::TDataSource dataSource;
            *dataSource.MutableWebSearchDocsRight() = searchResponse.WebSearchDocsRight();
            ctx.AddProtobufItem(dataSource, NScenarios::GetDataSourceContextName(EDataSourceType::WEB_SEARCH_DOCS_RIGHT));
        }
        {
            NScenarios::TDataSource dataSource;
            *dataSource.MutableWebSearchWizplaces() = searchResponse.WebSearchWizplaces();
            ctx.AddProtobufItem(dataSource, NScenarios::GetDataSourceContextName(EDataSourceType::WEB_SEARCH_WIZPLACES));
        }
        {
            NScenarios::TDataSource dataSource;
            *dataSource.MutableWebSearchSummarization() = searchResponse.WebSearchSummarization();
            ctx.AddProtobufItem(dataSource, NScenarios::GetDataSourceContextName(EDataSourceType::WEB_SEARCH_SUMMARIZATION));
        }
        {
            NScenarios::TDataSource dataSource;
            *dataSource.MutableWebSearchWizard() = searchResponse.WebSearchWizard();
            ctx.AddProtobufItem(dataSource, NScenarios::GetDataSourceContextName(EDataSourceType::WEB_SEARCH_WIZARD));
        }
        {
            NScenarios::TDataSource dataSource;
            *dataSource.MutableWebSearchBanner() = searchResponse.WebSearchBanner();
            ctx.AddProtobufItem(dataSource, NScenarios::GetDataSourceContextName(EDataSourceType::WEB_SEARCH_BANNER));
        }
        {
            NScenarios::TDataSource dataSource;
            *dataSource.MutableWebSearchRenderrer() = searchResponse.WebSearchRenderrer();
            ctx.AddProtobufItem(dataSource, NScenarios::GetDataSourceContextName(EDataSourceType::WEB_SEARCH_RENDERRER));
        }

        // Add megamind search deps
        {
            NWebSearch::TTunnellerRawResponse response;
            if (searchResponse.Data().Has(NAnalyticsInfo::TUNNELLER_RAW_RESPONSE)) {
                response.MutableRawResponse()->set_value(TString{searchResponse.Data()[NAnalyticsInfo::TUNNELLER_RAW_RESPONSE].GetString()});
            }
            ctx.AddProtobufItem(response, AH_ITEM_SEARCH_PART_TUNNELLER_RAW_RESPONSE);
        }
        {
            if (const auto* bgFactors = searchResponse.BgFactors()) {
                ctx.AddProtobufItem(*bgFactors, AH_ITEM_SEARCH_PART_BG_FACTORS);
            }
        }
        {
            NWebSearch::TStaticBlenderFactors response;
            if (const auto* staticBlenderFactors = searchResponse.StaticBlenderFactors()) {
                for (const auto factor : *staticBlenderFactors) {
                    response.MutableFactors()->Add(factor);
                }
            }
            ctx.AddProtobufItem(response, AH_ITEM_SEARCH_PART_STATIC_BLENDER_FACTORS);
        }
        {
            NWebSearch::TReportGrouping response;
            for (const auto& group : searchResponse.Report()["Grouping"].GetArray()) {
                response.MutableRawGrouping()->Add(group.ToJson());
            }
            ctx.AddProtobufItem(response, AH_ITEM_SEARCH_PART_REPORT_GROUPING);
        }

        UpdateMiscHandleSensors(globalContext, EMiscHandle::SPLIT_WEB_SEARCH, ERequestResult::SUCCESS,
                               (TInstant::Now() - start).MilliSeconds());
    } catch (yexception& e) {
        UpdateMiscHandleSensors(globalContext, EMiscHandle::SPLIT_WEB_SEARCH, ERequestResult::ERROR,
                               (TInstant::Now() - start).MilliSeconds());

        LOG_ERROR(logger) << "SplitWebSearch failed, reason: " << e.what();
        throw;
    }
}


} // namespace NAlice::NHollywood

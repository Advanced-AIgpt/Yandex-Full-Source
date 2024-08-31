#include "web_search.h"

#include "utils.h"

#include <alice/bass/forms/search/serp.h>

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/video_common/defs.h>

#include <alice/library/analytics/common/names.h>
#include <alice/library/experiments/flags.h>

#include <library/cpp/cgiparam/cgiparam.h>

namespace {

constexpr ui32 SEARCH_PAGES_NUM = 2;
constexpr ui32 SEARCH_SINGLE_PAGE_SERP_RESULTS_COUNT = 10;
constexpr ui32 SEARCH_ITEMS_COUNT = SEARCH_PAGES_NUM * SEARCH_SINGLE_PAGE_SERP_RESULTS_COUNT;
const TString SEARCH_ITEMS_COUNT_STRING = ToString(SEARCH_ITEMS_COUNT);

} // namespace

namespace NBASS::NVideo {

TWebSearchByProviderHandle::TWebSearchByProviderHandle(TStringBuf patchedQuery, const TVideoClipsRequest& request,
                                                       TStringBuf providerName, TContext& context)
    : TSetupRequest<TWebSearchByProviderResponse>(ConstructRequestId(TString{providerName} + "_web_search", request, TCgiParameters(), patchedQuery))
    , ProviderName(providerName)
    , Context(context)
    , Request(request)
    , PatchedQuery(patchedQuery)
{
}

NHttpFetcher::THandle::TRef TWebSearchByProviderHandle::Fetch(NHttpFetcher::IMultiRequest::TRef multiRequest) {
    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("numdoc"), SEARCH_ITEMS_COUNT_STRING);
    AddYaVideoAgeFilterParam(Context, cgi);
    if (Context.HasExpFlag(NAlice::NVideoCommon::FLAG_ANALYTICS_VIDEO_WEB_RESPONSES)) {
        NSerp::AddTemplateDataCgi(cgi);
    }

    return NSerp::PrepareSearchRequest(PatchedQuery, Context, cgi, NAlice::TWebSearchBuilder::EService::BassVideoHost,
                                       multiRequest)
        ->Fetch();
}

TResultValue TWebSearchByProviderHandle::Parse(NHttpFetcher::TResponse::TConstRef httpResponse, TWebSearchByProviderResponse* response, NSc::TValue* /*factorsData*/) {
    NSc::TValue searchResult;
    if (auto error = NSerp::ParseSearchResponse(httpResponse, &searchResult)) {
        TString errMsg = TStringBuilder() << "web search over video/" << ProviderName
                                          << " error: " << error->Msg;
        LOG(ERR) << errMsg << Endl;
        return TError(error->Type, errMsg);
    }

    if (Context.HasExpFlag(NAlice::NExperiments::TUNNELLER_ANALYTICS_INFO) &&
        searchResult.IsDict() && searchResult.Has(NAlice::NAnalyticsInfo::TUNNELLER_RAW_RESPONSE)) {
        Context.GetAnalyticsInfoBuilder().AddTunnellerRawResponse(
            TString{searchResult[NAlice::NAnalyticsInfo::TUNNELLER_RAW_RESPONSE].GetString()});
    }

    for (NSc::TValue& doc : searchResult["searchdata"]["docs"].GetArrayMutable()) {
        TVideoItem item;
        if (!ParseItemFromDoc(doc, &item))
            continue;

        ui64 relevance;
        if (TryFromString(doc["relevance"].GetString(), relevance))
            item->Relevance() = relevance;
        else
            LOG(WARNING) << "Failed to parse relevance for doc " << doc["url"].GetString() << Endl;

        double relevancePrediction;
        if (TryFromString(doc["markers"]["RelevPrediction"].GetString(), relevancePrediction))
            item->RelevancePrediction() = relevancePrediction;
        else
            LOG(WARNING) << "Failed to parse relevance prediction for doc " << doc["url"].GetString() << Endl;

        item->DebugInfo().WebPageUrl() = doc["url"].GetString();
        SetItemSource(item, NAlice::NVideoCommon::VIDEO_SOURCE_WEB);
        response->emplace_back(std::move(item));
    }

    return TResultValue();
}

} // namespace NBASS::NVideo

#pragma once

#include <alice/hollywood/library/scenarios/market/common/proto/search_info.pb.h>
#include "market_url_builder.h"
#include "report/client.h"
#include "report/response.h"
#include "types.h"

namespace NAlice::NHollywood::NMarket {

/*
    Содержит параметры поиска на маркете.
*/
struct TSearchInfo
{
    TSearchInfo(TStringBuf text, NGeobase::TId);
    TSearchInfo(const NProto::TSearchInfo& proto);

    TString Text;
    NGeobase::TId RegionId;
    TMaybe<TCategory> Category = Nothing();
    TCgiGlFilters GlFilters = {};
    TCgiRedirectParameters RedirectParams = {};
    TMaybe<EMarketGoodState> GoodState = Nothing();

    void ApplyRedirect(const TReportPrimeRedirect& redirect);
    TString CreateMarketUrl(const TMarketUrlBuilder& urlBuilder) const;
    TReportPrimeRequest CreateReportRequest(TReportClient& client) const;
    NProto::TSearchInfo ToProto() const;

private:
    TReportPrimeRequest CreateBaseReportRequest(TReportClient& client) const;
};

} // namespace NAlice::NHollywood::NMarket

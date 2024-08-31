#pragma once

#include "client.h"
#include "context.h"
#include "filter_worker.h"
#include "market_geo_support.h"

#include <library/cpp/scheme/scheme.h>

namespace NBASS {

namespace NMarket {

class TMarketHowMuchRequestImpl {
public:
    explicit TMarketHowMuchRequestImpl(TMarketContext& ctx);
    TResultValue Do();

private:
    TResultValue HandleResponseWithPossibleRedirect(const TReportResponse& response, size_t callCount = 0);
    TResultValue HandleModel(TModelId modelId);
    TResultValue HandleParametric(
        const TCategory& category,
        const TRedirectCgiParams& redirectParams,
        const TCgiGlFilters& filters,
        const TString& text,
        const TString& suggestText);

    TResultValue HandleOther(const TVector<TReportResponse::TResult>& results);
    TResultValue HandleEmptySerp();

    bool NeedToShowBeruAdv(const TVector<TReportResponse::TResult>& results) const;
    void FillAndRenderPopularGoods(const TVector<TReportResponse::TResult>& results);
    void FillResultUrls(const TString& uri);
    NSc::TValue GetPopularGoodPrices(const TVector<TReportResponse::TResult>& results);
    TString GetPopularGoodsCurrency(const TVector<TReportResponse::TResult>& results);

    TString CurrentRequest;
    TMarketContext& Ctx;
    TFilterWorker FilterWorker;
    TMarketGeoSupport GeoSupport;
};

} // namespace NMarket

} // namespace NBASS

#pragma once

#include "client.h"
#include "context.h"
#include "filter_worker.h"

namespace NBASS {

namespace NMarket {

class TBaseMarketChoice {
public:
    explicit TBaseMarketChoice(TMarketContext& ctx);
    TResultValue Do();

protected:
    void FillCtxFromParametricRedirect(const TReportResponse::TParametricRedirect& redirect);
    TString GetSearchResultUrl(TMaybe<EMarketType> optionalMarketType = Nothing()) const;
    TString GetCategoryResultUrl(
        TMaybe<EMarketType> optionalMarketType = Nothing(),
        const TMaybe<TCategory>& optionalCategory = Nothing()) const;
    TString GetResultUrl(TMaybe<EMarketType> optionalMarketType = Nothing());
    bool FillResult(const TVector<TReportResponse::TResult>& results);
    TReportResponse MakeFilterRequestWithRegionHandling(TMaybe<EMarketType> type = Nothing());
    TReportRequest MakeFilterRequestAsync(
        bool allowRedirects = false,
        TMaybe<EMarketType> optionalMarketType = Nothing(),
        const TMaybe<TCategory>& optionalCategory = Nothing(),
        const TMaybe<TVector<i64>>& fesh = Nothing());
    void AppendTextRedirect(const TStringBuf text);
    static bool HasIllegalWarnings(const TVector<TWarning>& warnings);
    TReportRequest MakeSearchRequestAsync(
        TStringBuf query,
        TMaybe<EMarketType> marketType,
        bool allowRedirects,
        const TRedirectCgiParams& redirectParams = TRedirectCgiParams());
    TReportResponse MakeSearchRequest(
        TStringBuf query,
        TMaybe<EMarketType> marketType = Nothing(),
        bool allowRedirects = true,
        const TRedirectCgiParams& redirectParams = TRedirectCgiParams());

    virtual TResultValue DoImpl() = 0;

protected:
    NSc::TValue GetResultDoc(const TReportResponse::TResult& result, ui32 galleryPosition);

    TMarketContext& Ctx;
    TFilterWorker FilterWorker;
};

} // namespace NMarket

} // namespace NBASS

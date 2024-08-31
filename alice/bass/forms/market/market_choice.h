#pragma once

#include "base_market_choice.h"
#include "client.h"
#include "context.h"
#include "filter_worker.h"
#include "forms.h"
#include "market_geo_support.h"
#include "state.h"

#include <alice/bass/forms/market/client/checkout.sc.h>
#include <alice/bass/forms/parallel_handler.h>

namespace NBASS {

namespace NMarket {

class TMarketChoice: public TBaseMarketChoice {
public:
    explicit TMarketChoice(TMarketContext& ctx, bool isParallelMode);

    IParallelHandler::TTryResult TryDo();

private:
    TResultValue DoImpl() override;

    IParallelHandler::TTryResult HandleSearch();
    TMaybe<TReportResponse> MakeSearchRequestWithRedirects();

    TResultValue HandleUnexpectedBehaviour();
    TResultValue HandleResponseError(const TReportResponse& response);
    TResultValue HandleProductOutdated();
    IParallelHandler::TTryResult TryHandleNative();
    IParallelHandler::TTryResult TryHandleNativeBeru();
    IParallelHandler::TTryResult TryHandleNativeAny();
    void CustomiseCtx();
    TResultValue HandleNative();
    TResultValue HandleNativeBeru();
    TResultValue HandleNativeAny();
    TResultValue HandleActivation(EMarketType marketType);
    TResultValue HandleMarketChoice();
    TResultValue HandleNumberFilter();
    TResultValue HandleProductDetailsExternal();
    TResultValue HandleProductDetailsDemand();
    TResultValue HandleStartChoiceAgain();
    TResultValue HandleBeruOrder();
    TResultValue HandleAddToCart();
    TResultValue HandleGoToShop();
    TResultValue HandleGarbage();
    TResultValue HandleSkuOfferDetails(ui64 sku);
    IParallelHandler::TTryResult HandleFilterSelection();
    IParallelHandler::TTryResult HandleNoneRedirection(const TReportResponse& response, TStringBuf query);
    IParallelHandler::TTryResult HandleParametricRedirection(const TReportResponse::TParametricRedirect& redirect);
    IParallelHandler::TTryResult RenderEmptyResult();
    IParallelHandler::TTryResult RenderGreenSuggestOrEmptyResult(const TReportResponse& greenResponse);
    bool TryRenderAskContinuePhraseFromMds(ui64 total);
    IParallelHandler::TTryResult RenderGalleryAndAskContinue(const TReportResponse& response);
    void AddNativeActivationUrls();
    bool IsPriceRequest() const;
    void TryHandlePriceRequest();
    bool TryUpdateFormalizedNumberFilter(const TFormalizedGlFilters& filters);
    NSc::TValue GetModelDetails(const TModel& model,
        const NSc::TValue& rawModel,
        const TCgiGlFilters& glFilters,
        const NBassApi::TReportDefaultOfferBlue<TBoolSchemeTraits>& blueDefaultOffer);
    NSc::TValue GetOfferDetails(const TOffer& offer, const NSc::TValue& rawOffer);
    NSc::TValue GetBlueOfferDetails(
        const TOffer& offer,
        const NBassApi::TReportDocumentConst<TBoolSchemeTraits>& rawOffer,
        const NSc::TValue& specs);
    void AddCartActionToDetails(ui64 sku, NSc::TValue details);
    void FillBlueOfferFields(const NBassApi::TReportDocumentConst<TBoolSchemeTraits>& offer, NSc::TValue& details);
    void FillShopOfferFields(const TMaybe<TOffer>& offer, NSc::TValue& details);
    TBeruOrderCardData GetBeruOrderData(
        ui64 sku,
        const NBassApi::TReportDocumentConst<TBoolSchemeTraits>& blueOffer);
    NSc::TValue GetRatingIcons() const;
    bool CheckExitDueAttemptsLimit();

    void SetState(EChoiceState state);
    TVector<TString> GetMarketPrefixes();

    bool CheckRedirectCategory(const TCategory& category) const;
    IParallelHandler::TTryResult CheckResultsCategory(const TVector<TReportResponse::TResult>& results) const;
    TResultValue FormalizeFilters();
    bool CheckMarketChoiceSupportedForGeoId() const;

private:
    EChoiceForm Form;
    EChoiceState State;
    TMarketGeoSupport GeoSupport;
    TNumberFilterWorker NumberFilterWorker;
    TQueryInfo InitialQueryInfo;
    bool IsParallelMode;
};

}

}

#pragma once

#include "base_market_choice.h"
#include "checkout_user_info.h"
#include "client/report_client.h"
#include "context.h"
#include "forms.h"
#include "number_filter_worker.h"
#include "state.h"
#include "market_geo_support.h"

#include <alice/bass/forms/market/client/checkout.sc.h>
#include <alice/bass/forms/parallel_handler.h>

namespace NBASS {

namespace NMarket {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class TMarketRecurringPurchase: public TBaseMarketChoice {
public:
    explicit TMarketRecurringPurchase(TMarketContext& ctx, bool isParallelMode);

    IParallelHandler::TTryResult TryDo();

public:
    using TRawOffer = NBassApi::TReportDocumentConst<TBoolSchemeTraits>;
    using TCartDeliveryScheme = NBassApi::TCartDelivery<TBoolSchemeTraits>;
    using TCartDeliveryOptionScheme = NBassApi::TCartDeliveryOption<TBoolSchemeTraits>;
    using TCartDeliveryOptionSchemeConst = NBassApi::TCartDeliveryOptionConst<TBoolSchemeTraits>;

private:
    TQueryInfo InitialQueryInfo;
    ERecurringPurchaseForm Form;
    ERecurringPurchaseState State;
    bool IsScreenless;
    bool IsFirstRequest;
    TCheckoutState CheckoutState;
    TCheckoutUserInfo UserInfo;
    TInstant StartTime;
    TNumberFilterWorker NumberFilterWorker;
    bool IsParallelMode;
    TMarketGeoSupport GeoSupport;

private:
    TResultValue DoImpl() override;

    ui64 GetSkuAvailableItemsNumber() const;
    TResultValue HandleGuest();
    TResultValue HandleNoEmail();
    TResultValue HandleRecurringPurchase();
    TResultValue HandleProductIndex();
    TResultValue HandleNumberFilter();
    TResultValue HandleChoice();
    TResultValue HandleCancel();
    TResultValue HandleGarbage();
    TResultValue HandleCheckoutStart();
    TResultValue HandleCheckoutStartScreenless();
    TResultValue HandleItemsNumber();
    TResultValue HandleSkuOutdated();
    TResultValue HandlePhone();
    TResultValue HandleAddress();
    TResultValue HandleConfirmOrder();
    TResultValue HandleCheckoutWaiting();
    TResultValue HandleWrongState();
    void HandleSuccessCheckout(i64 orderId, bool guestMode);

    bool TryUpdateFormalizedNumberFilter(const TFormalizedGlFilters& filters);
    void TryHandlePriceRequest();
    bool IsPriceRequest() const;
    bool TryFormatPhone(TString& phone);
    TResultValue ProcessItemsNumber();
    TResultValue ProcessPhone();
    TResultValue ProcessAddress();

    bool TryCheckOrderExists();
    TResultValue Checkout();
    TResultValue ManualCheckout();

    bool TryHandleDeliveryOptions(const TAddressSchemeConst& address);
    TReportResponse MakeDefinedDocsRequest(const TVector<ui64>& skus) const;
    TResultValue HandleParametricRedirection(const TReportResponse::TParametricRedirect& redirect);
    TResultValue HandleNoneRedirection(const TReportResponse& response);
    TResultValue HandleFilterSelection();
    TResultValue HandleSkuOfferDetails();
    TResultValue HandleModelDetails(TModelId id, const TCgiGlFilters& glFilters,
         const TRedirectCgiParams& redirectParams = TRedirectCgiParams());
    void FillDeliveryOptions(
        const TCartDeliveryOptionSchemeConst& deliveryOption,
        NDomSchemeRuntime::TArray<TBoolSchemeTraits, TDeliveryOptionScheme>& result);
    TResultValue HandleDeliveryOptions();
    TResultValue HandleDeliveryFirstOption();
    TResultValue HandleDeliveryOptions(TMaybe<i64> deliveryOptionIndex);
    TResultValue RenderAskItemsNumber(TStringBuf textCardName, ui32 availableItemsNumber);
    TResultValue RenderGalleryAndAskContinue(const TReportResponse& response);
    TResultValue RenderEmptyResult();
    TResultValue RenderHasNoOrders();
    TResultValue RenderDoNotCheckout();
    TResultValue RenderEmptyDeliveryOptions();
    TResultValue RenderGallery(const TReportResponse& response, const TVector<TSkuOrderItem>& skus);
    TResultValue FormalizeFilters();
    TMaybe<TReportResponse> MakeSearchRequestWithRedirects();
    bool FillResultFromOrders(
        const TVector<TReportResponse::TResult>& results,
        const TVector<TSkuOrderItem>& skus,
        TMaybe<ui64>& singleSku);

    bool CheckNoActivation();
    bool CheckExitDueAttemptsLimit();

    void ProcessSkuOfferDetails();

    NSc::TValue GetRatingIcons() const;
    NSc::TValue GetModelDetails(const TModel& model, const NSc::TValue& rawModel, const TCgiGlFilters& glFilters,
                                const TReportSearchResponse::TSchemeConst& blueDefaultOffer);
    NSc::TValue GetBlueOfferDetails(const TOffer& offer, const TRawOffer& rawOffer);
    TBeruOrderCardData GetBeruOrderData(ui64 sku, const TRawOffer& blueOffer);
    void FillBlueOfferFields(const TRawOffer& offer, NSc::TValue& details);

    void SetState(ERecurringPurchaseState state);

    void AddCancelSuggest();
    void DeleteCancelSuggest();
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace NMarket

} // namespace NBASS

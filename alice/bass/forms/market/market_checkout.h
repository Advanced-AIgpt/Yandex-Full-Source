#pragma once

#include "checkout_user_info.h"
#include "client.h"
#include "client/checkouter_client.h"
#include "client/report_client.h"
#include "context.h"
#include "delivery_intervals_worker.h"
#include "forms.h"
#include "market_geo_support.h"
#include "state.h"

#include <alice/bass/forms/market/client/checkout.sc.h>
#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/parallel_handler.h>
#include <library/cpp/scheme/domscheme_traits.h>

namespace NBASS {

namespace NMarket {

class TMarketCheckout {
public:
    explicit TMarketCheckout(TMarketContext& ctx);

    TResultValue Do();

    IParallelHandler::TTryResult TryDo();

    static bool TryFormatEmail(TString& email);
    static bool TryFormatPhone(TString& phone);

public:
    using TState = TCheckoutState;
    using TCartSchemeConst = NBassApi::TCartConst<TBoolSchemeTraits>;
    using TCartDeliveryScheme = NBassApi::TCartDelivery<TBoolSchemeTraits>;
    using TCartDeliveryOptionScheme = NBassApi::TCartDeliveryOption<TBoolSchemeTraits>;
    using TCartDeliveryOptionSchemeConst = NBassApi::TCartDeliveryOptionConst<TBoolSchemeTraits>;

private:
    void LogUnexpectedBehaviour() const;
    void HandleSkuOutdated();
    void HandleStart(TState& state);
    void HandleItemsNumber(TState& state);
    void HandleEmail(TState& state);
    void HandlePhone(TState& state);
    void HandleAddress(TState& state);
    void HandleDeliveryOptions(TState& state);
    void HandleDeliveryOptions(TState& state, const TMaybe<i64>& deliveryOptionIndex);
    void HandleConfirmOrder(TState& state);
    void HandleCheckoutWaiting(TState& state);
    void HandleWrongState();

    void ChangeStep(EChoiceState newStep);
    bool CheckExitDueAttemptsLimit();

    void ProcessItemsNumber(TState& state, const TCheckoutUserInfo& userInfo);
    void ProcessEmail(TState& state, const TCheckoutUserInfo& userInfo);
    void ProcessPhone(TState& state, const TCheckoutUserInfo& userInfo);
    void ProcessAddress(TState& state, const TCheckoutUserInfo& userInfo);
    void SetDeliverySuggests(TState& state, const TCheckoutUserInfo& userInfo);
    bool TryCheckOrderExists(const TState& state, const TCheckoutUserInfo& userInfo);
    void HandleSuccessCheckout(i64 orderId, bool guestMode);
    bool TryHandleDeliveryOptions(TState& state, const TCheckoutUserInfo& userInfo, const TDeliveryScheme& delivery);
    TResponseHandle<TCheckouterData> MakeAsyncCheckouterCartRequest(
        TState& state,
        const TCheckoutUserInfo& userInfo,
        const TDeliveryScheme& delivery);
    TMaybe<TCheckouterData> MakeCheckouterCartRequest(
        TState& state,
        const TCheckoutUserInfo& userInfo,
        const TDeliveryScheme& delivery);
    ui64 GetSkuAvailableItemsNumber(const TState& state) const;
    void RenderAskItemsNumber(
        TStringBuf textCardName,
        ui32 availableItemsNumber,
        const NSc::TValue& data = NSc::TValue());
    void RenderInvalidAddress(TState& state);
    void RenderEmptyDeliveryOptions(TState& state);
    void RenderAddressError(TStringBuf formName, TState& state);
    void RenderAskAddress(TStringBuf textCardName, const TState& state);
    void RenderCheckoutConfirmation(TState& state, const TCheckoutUserInfo& userInfo);
    bool TrySetPickupDeliveryAndOption(
        TState& state,
        const TCheckoutUserInfo& userInfo,
        const TDeliveryScheme& delivery);
    void HandleDeliverySuggestSelection(TState& state, const TCheckoutUserInfo& userInfo, size_t suggestIdx);
    void Checkout(TState& state);
    void ManualCheckout(TState& state);
    void SleepMaxAllowableTime() const;

    static TStringBuf StateToOldStep(EChoiceState state);
    static EChoiceState StateFromOldStep(TStringBuf step);

    TMarketContext& Ctx;
    EChoiceForm Form;
    EChoiceState State;
    TInstant StartTime;
    TDeliveryIntervalsWorker DeliveryIntervalsWorker;
    TMarketGeoSupport GeoSupport;
};

} // namespace NMarket

} // namespace NBASS

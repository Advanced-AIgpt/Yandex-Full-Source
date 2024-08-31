#pragma once

#include "bool_scheme_traits.h"
#include "base_client.h"
#include <alice/bass/forms/market/types.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <alice/bass/forms/market/checkout_user_info.h>
#include <alice/bass/forms/market/client/checkout.sc.h>
#include <alice/bass/forms/market/client/report.sc.h>

namespace NBASS {

namespace NMarket {

using TCheckouterRequest = TSchemeHolder<NBassApi::TCheckoutRequest<TBoolSchemeTraits>>;

class TCheckouterClient: public TBaseClient {
public:
    explicit TCheckouterClient(TMarketContext& context);

    THttpResponse<TCheckoutResponse> Checkout(
        const TStateCartScheme& cart,
        const TDeliveryScheme& delivery,
        const TDeliveryOptionScheme& deliveryOption,
        const TCheckoutUserInfo& userInfo,
        TStringBuf notes);

    TResponseHandle<TCheckouterData> CartAsync(
        const TStateCartScheme& cart,
        const TCheckoutUserInfo& userInfo,
        const TDeliveryScheme& delivery);
    THttpResponse<TCheckouterData> Cart(
        const TStateCartScheme & cart,
        const TCheckoutUserInfo& userInfo,
        const TDeliveryScheme& delivery);

    TResponseHandle<TCheckouterOrders> GetOrdersByUidAsync(
        TStringBuf uid,
        ui32 pageSize,
        TMaybe<TStringBuf> notesQuery = Nothing());
    THttpResponse<TCheckouterOrders> GetOrdersByUid(
        TStringBuf uid,
        ui32 pageSize,
        TMaybe<TStringBuf> notesQuery = Nothing());
    TVector<TCheckouterOrder> GetAllOrdersByUid(TStringBuf uid, ui32 pageSize);

    THttpResponse<TMuid> Auth(const TMaybe<TMuid>& muid, TStringBuf ip, TStringBuf userAgent);

private:
    i64 RegionId;
    i32 Clid;
    TStringBuf Uuid;
    TStringBuf Reqid;
    TStringBuf Ip;
    ECheckouterPlatform Platform;
    const TMarketExperiments& Experiments;

private:
    TCheckouterRequest CreateCheckoutBody(
        const TStateCartScheme& cart,
        const TDeliveryScheme& delivery,
        const TDeliveryOptionScheme& deliveryOption,
        const NBASS::NMarket::TCheckoutUserInfo& userInfo,
        TStringBuf notes) const;

    TCheckouterData CreateCartBody(
        const TStateCartScheme& cart,
        const TCheckoutUserInfo& userInfo,
        const TDeliveryScheme& delivery) const;

    TCgiParameters CreateCheckoutCgiParams(
        const TCheckoutUserInfo& userInfo,
        bool sandbox) const;

    THashMap<TString, TString> CreateCheckoutHeaders() const;

    void FillBuyer(NBassApi::TBuyer<TBoolSchemeTraits> buyer, const TCheckoutUserInfo& userInfo) const;

    void SetCheckoutItem(
        const TStateCartScheme& cart,
        NBassApi::TCart<TBoolSchemeTraits>& requestCart) const;
    void SetCheckoutDelivery(
        const TDeliveryScheme& delivery,
        const TDeliveryOptionScheme& deliveryOption,
        const TCheckoutUserInfo& userInfo,
        NBassApi::TCart<TBoolSchemeTraits>& cart) const;

    static ECheckouterPlatform GetCheckouterPlatform(const TClientInfo& clientInfo);

};

} // namespace NMarket

} // namespace NBASS

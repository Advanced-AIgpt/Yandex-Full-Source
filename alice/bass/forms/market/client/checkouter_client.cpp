#include "checkouter_client.h"

#include <alice/bass/forms/market/fake_users.h>
#include <alice/bass/forms/market/settings.h>
#include <alice/bass/util/error.h>

namespace NBASS {

namespace NMarket {

TCheckouterClient::TCheckouterClient(TMarketContext& ctx)
    : TBaseClient(ctx.GetSources(), ctx)
    , RegionId(ctx.UserRegion())
    , Clid(static_cast<i32>(ctx.GetMarketClid()))
    , Uuid(ctx.Meta().UUID())
    , Reqid(ctx.RequestId())
    , Ip(ctx.Meta().ClientIP())
    , Platform(GetCheckouterPlatform(ctx.MetaClientInfo()))
    , Experiments(ctx.GetExperiments())
{
}

THttpResponse<TCheckoutResponse> TCheckouterClient::Checkout(
    const TStateCartScheme& cart,
    const TDeliveryScheme& delivery,
    const TDeliveryOptionScheme& deliveryOption,
    const TCheckoutUserInfo& userInfo,
    TStringBuf notes)
{
    const auto& source = Sources.MarketCheckouter("/checkout");

    TCgiParameters cgi = CreateCheckoutCgiParams(userInfo, TFakeUsers::IsFakeCheckoutUser(userInfo.GetEmail()));

    const auto& headers = CreateCheckoutHeaders();
    const auto& body = CreateCheckoutBody(cart, delivery, deliveryOption, userInfo, notes);

    return Run<TCheckoutResponse>(source, cgi, body.Value(), headers).Wait();
}

TResponseHandle<TCheckouterData> TCheckouterClient::CartAsync(
    const TStateCartScheme& cart,
    const TCheckoutUserInfo& userInfo,
    const TDeliveryScheme& delivery)
{
    const auto& source = Sources.MarketCheckouterIntervals("/cart");

    TCgiParameters cgi = CreateCheckoutCgiParams(userInfo, false /* sandbox */);
    const auto& headers = CreateCheckoutHeaders();
    const auto& body = CreateCartBody(cart, userInfo, delivery);

    return Run<TCheckouterData>(source, cgi, body.Value(), headers);
}

THttpResponse<TCheckouterData> TCheckouterClient::Cart(
    const TStateCartScheme& cart,
    const TCheckoutUserInfo& userInfo,
    const TDeliveryScheme& delivery)
{
    auto result = CartAsync(cart, userInfo, delivery).Wait();
    return result;
}

TResponseHandle<TCheckouterOrders> TCheckouterClient::GetOrdersByUidAsync(
    TStringBuf uid,
    ui32 pageSize,
    TMaybe<TStringBuf> notesQuery)
{
    const auto& source = Sources.MarketCheckouterOrders(uid);

    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("rgb"), TStringBuf("BLUE"));
    if (notesQuery) {
        cgi.InsertEscaped(TStringBuf("notes"), *notesQuery);
    }
    cgi.InsertUnescaped(
        TStringBuf("fromDate"),
        (Now() - CHECKOUT_ORDERS_HISTORY_DEPTH).FormatLocalTime("%d-%m-%Y"));
    cgi.InsertUnescaped(TStringBuf("pageSize"), ToString(pageSize));
    cgi.InsertUnescaped(TStringBuf("archived"), TStringBuf("false"));

    return Run<TCheckouterOrders>(source, cgi, NSc::TValue(), CreateCheckoutHeaders());
}

THttpResponse<TCheckouterOrders> TCheckouterClient::GetOrdersByUid(
    TStringBuf uid,
    ui32 pageSize,
    TMaybe<TStringBuf> notesQuery)
{
    return GetOrdersByUidAsync(uid, pageSize, notesQuery).Wait();
}

THttpResponse<TMuid> TCheckouterClient::Auth(const TMaybe<TMuid>& muid, TStringBuf ip, TStringBuf userAgent)
{
    const auto& source = Sources.MarketCheckouterIntervals(TStringBuf("/auth/"));

    NSc::TValue body;
    body["ip"] = ip;
    body["userAgent"] = userAgent;

    TCgiParameters cgi;
    if (muid) {
       cgi.InsertUnescaped(TStringBuf("cookie"), muid->Cookie);
    }
    return Run<TMuid>(source, cgi, body, CreateCheckoutHeaders()).Wait();
}

TCheckouterRequest TCheckouterClient::CreateCheckoutBody(
    const TStateCartScheme& cart,
    const TDeliveryScheme& delivery,
    const TDeliveryOptionScheme& deliveryOption,
    const NBASS::NMarket::TCheckoutUserInfo& userInfo,
    TStringBuf notes) const
{
    TCheckouterRequest body;
    body->BuyerRegionId() = RegionId;
    body->BuyerCurrency() = "RUR";
    body->PaymentMethod() = "CASH_ON_DELIVERY";
    body->PaymentType() = "POSTPAID";

    FillBuyer(body->Buyer(), userInfo);

    auto order = body->Orders(0);
    order.Notes() = notes;
    SetCheckoutDelivery(delivery, deliveryOption, userInfo, order);
    SetCheckoutItem(cart, order);
    return body;
}

TCheckouterData TCheckouterClient::CreateCartBody(
    const TStateCartScheme& cart,
    const NBASS::NMarket::TCheckoutUserInfo& userInfo,
    const TDeliveryScheme& delivery) const
{
    TCheckouterData body;
    body->BuyerRegionId() = RegionId;
    body->BuyerCurrency() = "RUR";

    FillBuyer(body->Buyer(), userInfo);

    auto order = body->Carts(0);
    order.Delivery().RegionId() = delivery.Address().RegionId();
    if (delivery.Type() == TStringBuf("COURIER")) {
        order.Delivery().BuyerAddress() = delivery.Address();
    }

    SetCheckoutItem(cart, order);
    return body;
}

TCgiParameters TCheckouterClient::CreateCheckoutCgiParams(
    const TCheckoutUserInfo& userInfo,
    bool sandbox) const
{
    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("uid"), userInfo.GetUid());
    cgi.InsertUnescaped(TStringBuf("platform"), ToString(Platform));
    cgi.InsertUnescaped(TStringBuf("showHiddenPaymentOptions"), TStringBuf("true"));
    cgi.InsertUnescaped(TStringBuf("rgb"), TStringBuf("BLUE"));
    cgi.InsertUnescaped(
        TStringBuf("yandexPlus"),
        userInfo.HasYandexPlus() ? TStringBuf("true") : TStringBuf("false"));
    cgi.InsertUnescaped(TStringBuf("prime"), TStringBuf("false"));
    cgi.InsertUnescaped(TStringBuf("minifyOutlets"), TStringBuf("true"));
    if (sandbox) {
        cgi.InsertUnescaped(TStringBuf("sandbox"), TStringBuf("true"));
        cgi.InsertUnescaped(TStringBuf("context"), TStringBuf("PRODUCTION_TESTING"));
    }
    cgi.InsertUnescaped(TStringBuf("clid"), ToString(Clid));
    cgi.InsertUnescaped(TStringBuf("uuid"), Uuid);
    cgi.InsertUnescaped(TStringBuf("wprid"), Reqid);
    cgi.InsertUnescaped(TStringBuf("ip"), Ip);
    return cgi;
}

THashMap<TString, TString> TCheckouterClient::CreateCheckoutHeaders() const
{
    THashMap<TString, TString> headers;
    headers["X-Hit-Rate-Group"] = "UNLIMIT";
    return headers;
}

void TCheckouterClient::FillBuyer(NBassApi::TBuyer<TBoolSchemeTraits> buyer, const TCheckoutUserInfo& userInfo) const
{
    buyer.Email() = userInfo.GetEmail();
    buyer.Phone() = userInfo.GetPhone();
    buyer.FirstName() = userInfo.GetFirstName();
    buyer.LastName() = userInfo.GetLastName();
    buyer.DontCall() = true;
}

void TCheckouterClient::SetCheckoutItem(
    const TStateCartScheme& cart,
    NBassApi::TCart<TBoolSchemeTraits>& requestCart) const
{
    requestCart.ShopId() = cart.Offer().Shop().Id();

    auto item = requestCart.Items(requestCart.Items().Size());

    item.FeedId() = FromString<ui64>(cart.Offer().Shop().Feed().Id());
    item.OfferId() = cart.Offer().Shop().Feed().OfferId();
    item.BuyerPrice() = FromString<double>(cart.Offer().Prices().Value());
    item.ShowInfo() = cart.Offer().FeeShow();
    item.Count() = cart.ItemsNumber();
}

void TCheckouterClient::SetCheckoutDelivery(
    const TDeliveryScheme& delivery,
    const TDeliveryOptionScheme& deliveryOption,
    const TCheckoutUserInfo& userInfo,
    NBassApi::TCart<TBoolSchemeTraits>& cart) const
{
    auto checkoutDelivery = cart.Delivery();
    auto checkoutDeliveryDates = checkoutDelivery.Dates();
    checkoutDelivery.Id() = deliveryOption.DeliveryId();
    checkoutDelivery.RegionId() = delivery.Address().RegionId();
    checkoutDeliveryDates.IsDefault() = true;
    if (delivery.Type() == TStringBuf("PICKUP")) {
        checkoutDelivery.OutletId() = delivery.Outlet().Id();
    } else if (delivery.Type() == TStringBuf("COURIER")) {
        checkoutDelivery.BuyerAddress() = delivery.Address();
        checkoutDelivery.BuyerAddress().Recipient() = userInfo.GetRecipientName();
        checkoutDelivery.BuyerAddress().Phone() = userInfo.GetPhone();
        checkoutDeliveryDates.OptionId() = deliveryOption.OptionId();
    } else {
        ythrow TMarketException(TStringBuilder() << "Cannot checkout with unknown delivery type " << delivery.Type());
    }
}

ECheckouterPlatform TCheckouterClient::GetCheckouterPlatform(const TClientInfo& clientInfo)
{
    if (clientInfo.IsIOS()) {
        return ECheckouterPlatform::ALICE_SEARCH_IOS;
    }
    if (clientInfo.IsAndroid()) {
        return ECheckouterPlatform::ALICE_SEARCH_ANDROID;
    }
    if (clientInfo.IsSmartSpeaker()) {
        return ECheckouterPlatform::ALICE_STATION;
    }
    if (clientInfo.IsYaStroka()) {
        return ECheckouterPlatform::ALICE_WINDOWS;
    }
    if (clientInfo.IsYaBrowser()) {
        return ECheckouterPlatform::ALICE_BROWSER;
    }
    return ECheckouterPlatform::UNKNOWN;
}

TVector<TCheckouterOrder> TCheckouterClient::GetAllOrdersByUid(TStringBuf uid, ui32 pageSize)
{
    auto recentOrdersHandle = GetOrdersByUidAsync(uid, pageSize);
    const auto& recentOrders = recentOrdersHandle.Wait().GetResponse();

    return recentOrders.GetOrders();
}

} // namespace NMarket

} // namespace NBASS

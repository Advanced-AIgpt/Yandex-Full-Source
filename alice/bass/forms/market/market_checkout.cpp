#include "market_checkout.h"

#include "client/checkouter_client.h"
#include "client/geo_client.h"
#include "client/report_client.h"
#include "client/stock_storage_client.h"
#include "context.h"
#include "fake_users.h"
#include "forms.h"
#include "settings.h"
#include "settings.h"
#include "delivery_intervals_worker.h"
#include "market_exception.h"
#include "market_url_builder.h"
#include "util/report.h"
#include "util/serialize.h"
#include "util/string.h"
#include "util/suggests.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <regex>

namespace NBASS {

namespace NMarket {

namespace {

// взято отсюда: https://a.yandex-team.ru/arc/trunk/arcadia/crypta/graph/soup/identifiers/lib/email.h?rev=3904333
const std::regex EMAIL_REGEX("^[a-zа-я0-9!#$%&'*+/=?^_`{|}~-]+(?:\\.[a-zа-я0-9!#$%&'*+/=?^_`{|}~-]+)*"
                             "@(?:[a-zа-я0-9](?:[a-zа-я0-9-]*[a-zа-я0-9])?\\.)"
                             "+[a-zа-я0-9](?:[a-zа-я0-9-]*[a-zа-я0-9])?$");

const std::regex REPLACE_NON_DIGITS_REGEX("[^\\d]");

constexpr TStringBuf ITEMS_NUMBER_STEP = "items_number";
constexpr TStringBuf EMAIL_STEP = "email";
constexpr TStringBuf PHONE_STEP = "phone";
constexpr TStringBuf ADDRESS_STEP = "address";
constexpr TStringBuf DELIVERY_OPTIONS_STEP = "delivery_options";
constexpr TStringBuf CONFIRM_ORDER_STEP = "confirm_order";
constexpr TStringBuf CHECKOUT_WAITING_STEP = "checkout_waiting";
constexpr TStringBuf CHECKOUT_COMPLETE = "checkout_complete";

TMaybe<TString> GetStringSlotValue(const TMarketContext& ctx, TStringBuf slotName)
{
    const TSlot* slot = ctx.Ctx().GetSlot(slotName);
    if (slot && !slot->Value.IsNull()) {
        return slot->Value.ForceString();
    }
    return Nothing();
}

TMaybe<i64> GetIntegerSlotValue(const TMarketContext& ctx, TStringBuf slotName)
{
    const TSlot* slot = ctx.Ctx().GetSlot(slotName);
    if (slot && !slot->Value.IsNull()) {
        return slot->Value.ForceIntNumber();
    }
    return Nothing();
}

// todo change all usages to GetIntegerSlotValue(ctx, TStringBuf("index")) when MALISA-556 will be in prod
// intent market__checkout_index with slot "index" conficts with market__checkout_items_number with slot "items_number"
// so for now we will always try to get index from "index" or from "items_number" slots
TMaybe<i64> GetIndex(const TMarketContext& ctx)
{
    auto indexSlotValue = GetIntegerSlotValue(ctx, TStringBuf("index"));
    if (indexSlotValue.Defined()) {
        return indexSlotValue;
    }
    return GetIntegerSlotValue(ctx, TStringBuf("items_number"));
}

TString GetSlotOrUtterance(const TMarketContext& ctx, TStringBuf slotName)
{
    auto slotValue = GetStringSlotValue(ctx, slotName);
    return slotValue ? *slotValue : TString(ctx.Utterance());
}

void SetCourierDelivery(const TAddressSchemeConst& address, TDeliveryScheme delivery)
{
    delivery.Type() = "COURIER";
    delivery.Address() = address;
}

void SetPickupDelivery(
    const TAddressSchemeConst& address,
    i64 outletId,
    TStringBuf outletName,
    TDeliveryScheme delivery)
{
    delivery.Type() = "PICKUP";
    delivery.Address() = address;
    delivery.Outlet().Id() = outletId;
    delivery.Outlet().Name() = outletName;
}

bool HasCashOnDeliveryPaymentOption(TMarketCheckout::TCartDeliveryOptionSchemeConst deliveryOption) {
    for (auto paymentOption : deliveryOption.PaymentOptions()) {
        if (paymentOption.PaymentMethod() == TStringBuf("CASH_ON_DELIVERY")) {
            return true;
        }
    }
    return false;
}

TVector<TSchemeHolder<TDeliveryOptionScheme>> GetCourierDeliveryOptions(
    TMarketCheckout::TCartSchemeConst cart)
{
    TVector<TSchemeHolder<TDeliveryOptionScheme>> result;
    for (const auto& deliveryOption : cart.DeliveryOptions()) {
        if (deliveryOption.Type() == TStringBuf("DELIVERY") && HasCashOnDeliveryPaymentOption(deliveryOption)) {
            for (const auto& interval : deliveryOption.DeliveryIntervals(0).Intervals()) {
                if (result.size() >= MAX_DELIVERY_OPTIONS_COUNT) {
                    break;
                }
                auto resultOption = result.emplace_back().Scheme();
                resultOption.DeliveryId() = deliveryOption.Id();
                resultOption.OptionId() = deliveryOption.DeliveryOptionId();
                resultOption.Price() = deliveryOption.BuyerPrice();
                auto dates = resultOption.Dates();
                dates.FromDate() = deliveryOption.Dates().FromDate();
                dates.ToDate() = deliveryOption.Dates().ToDate();
                dates.FromTime() = interval.FromTime();
                dates.ToTime() = interval.ToTime();
            }
        }
    }
    return result;
}

TMaybe<TSchemeHolder<TDeliveryOptionScheme>> GetPickupDeliveryOption(
    TMarketCheckout::TCartSchemeConst cart,
    i64 outletId)
{
    TMaybe<TMarketCheckout::TCartDeliveryOptionSchemeConst> cheapestOption;
    for (auto deliveryOption : cart.DeliveryOptions()) {
        if (deliveryOption.Type() == TStringBuf("PICKUP") && HasCashOnDeliveryPaymentOption(deliveryOption)) {
            for (auto outlet : deliveryOption.Outlets()) {
                if (outlet.Id() == outletId) {
                    if (!cheapestOption.Defined() || cheapestOption->BuyerPrice() > deliveryOption.BuyerPrice()) {
                        cheapestOption = deliveryOption;
                    }
                    break;
                }
            }
        }
    }
    if (cheapestOption.Defined()) {
        TSchemeHolder<TDeliveryOptionScheme> option;
        option->DeliveryId() = cheapestOption->Id();
        option->Price() = cheapestOption->BuyerPrice();
        option->Dates().FromDate() = cheapestOption->Dates().FromDate();
        option->Dates().ToDate() = cheapestOption->Dates().ToDate();
        return option;
    }
    return Nothing();
}

TMaybe<TCheckouterData> GetCartResponse(const THttpResponse<TCheckouterData>& response) {
    if (!response.IsHttpOk() && !response.IsTimeout()) {
        // todo MALISA-240 /cart может вернуть 400, если мы неправильно указали регион, поэтому переспрашиваем адрес
        // нужно рассмотреть все варианты ошибок от чекаутера и правильно их обрабатывать
        LOG(ERR) << "/cart returned error: " << response.GetErrorText() << Endl;
        return Nothing();
    }

    const auto& responseData = response.GetResponse();

    if (responseData->Carts().Size() != 1) {
        // todo MALISA-240 если чекаутер ответит ошибкой, мы выдадим ему "/cart response should have exactly 1 cart".
        // Стоит добавить валидацию ответа от чекаутера
        LOG(ERR) << "/cart did not return a single cart: carts#" << responseData->Carts().Size() << Endl;
        return Nothing();
    }
    return responseData;
}

} // anonymous namespace

TMarketCheckout::TMarketCheckout(TMarketContext& ctx)
    : Ctx(ctx)
    , Form(FromString<EChoiceForm>(Ctx.FormName()))
    , State(Ctx.GetState(EChoiceState::Null))
    , StartTime(Now())
    , DeliveryIntervalsWorker(TDeliveryIntervalsWorker(ctx))
    , GeoSupport(Ctx.Ctx().GlobalCtx())
{
}

TResultValue TMarketCheckout::Do()
{
    LOG(DEBUG) << "Form name " << Ctx.FormName() << " state " << State << Endl;

    if (!GeoSupport.IsBeruSupportedForGeoId(Ctx.UserRegion(), Ctx.GetScenarioType())) {
        Ctx.RenderMarketNotSupportedInLocation(EMarketType::BLUE, GeoSupport.GetRegionName(Ctx.UserRegion()));
        Ctx.RenderEmptySerp();
        return TResultValue();
    }

    if (Form == EChoiceForm::ProductDetailsExternal_BeruOrder) {
        Form = EChoiceForm::Checkout;
        Ctx.SetResponseFormAndCopySlots(ToString(Form), {TStringBuf("sku"), TStringBuf("choice_market_type"),
            TStringBuf("market_clid"), TStringBuf("state"), TStringBuf("is_open")});
        AddCancelSuggest(Ctx);
    }

    TState state = Ctx.GetCheckoutState();
    if (State == EChoiceState::Null) {
        State = StateFromOldStep(state.Step());
    }
    switch (State) {
        case EChoiceState::Null:
        case EChoiceState::Activation:
        case EChoiceState::Choice:
        case EChoiceState::ProductDetails:
        case EChoiceState::ProductDetailsExternal:
        case EChoiceState::BeruProductDetails:
        case EChoiceState::MakeOrder:
        case EChoiceState::ActivationOpen:
        case EChoiceState::ChoiceOpen:
        case EChoiceState::ProductDetailsOpen:
        case EChoiceState::BeruProductDetailsOpen:
        case EChoiceState::MakeOrderOpen:
            HandleStart(state);
            break;
        case EChoiceState::CheckoutItemsNumber:
            HandleItemsNumber(state);
            break;
        case EChoiceState::CheckoutEmail:
            HandleEmail(state);
            break;
        case EChoiceState::CheckoutPhone:
            HandlePhone(state);
            break;
        case EChoiceState::CheckoutAddress:
            HandleAddress(state);
            break;
        case EChoiceState::CheckoutDeliveryOptions:
            HandleDeliveryOptions(state);
            break;
        case EChoiceState::CheckoutConfirmOrder:
            HandleConfirmOrder(state);
            break;
        case EChoiceState::CheckoutWaiting:
            HandleCheckoutWaiting(state);
            break;
        case EChoiceState::CheckoutComplete:
        case EChoiceState::Exit:
            HandleWrongState();
            break;
    }
    return TResultValue();
}

void TMarketCheckout::LogUnexpectedBehaviour() const
{
    LOG(ERR) << TStringBuf("Unexpected behaviour in TMarketCheckout: state - ") << State
             << TStringBuf(", form - ") << Form;
}

IParallelHandler::TTryResult TMarketCheckout::TryDo()
{
    const auto result = Do();
    if (result.Defined()) {
        return result.GetRef();
    }
    return IParallelHandler::ETryResult::Success;
}

ui64 TMarketCheckout::GetSkuAvailableItemsNumber(const TState& state) const
{
    const auto& offer = state.Cart().Offer();
    // check if we have BLUE offer (with supplier sku and hence we could ask StockStorage for items count)
    if (!offer.HasSupplierSku()) {
        LOG(DEBUG) << TStringBuf("Offer is WHITE and has no supplier sku, so we are taking items count from report response");
        // check if we get items count from report, otherwise return 0
        return offer.HasStockStoreCount() ? offer->StockStoreCount() : 0;
    };
    // in BLUE offer case ask StockStorage for items count
    LOG(DEBUG) << TStringBuf("Offer is BLUE and we ask StockStorage for items count");
    return TStockStorageClient(Ctx.GetSources(), Ctx).SkuItemsNumber(
        offer->SupplierSku(),
        offer->Supplier().Id(),
        offer->Supplier().WarehouseId()
    );
}

void TMarketCheckout::RenderAskItemsNumber(
    TStringBuf textCardName,
    ui32 availableItemsNumber,
    const NSc::TValue& data)
{
    Ctx.AddTextCardBlock(textCardName, data);
    AddNumberSuggests(Ctx, std::min(availableItemsNumber, MAX_ITEMS_NUMBER_SUGGESTS));
}

void TMarketCheckout::HandleSkuOutdated()
{
    Ctx.AddTextCardBlock(TStringBuf("market_checkout__beru_offer_outdated"));
    ChangeStep(EChoiceState::Choice);
}

void TMarketCheckout::HandleStart(TState& state)
{
    if (!Ctx.HasBeruOrderSku()) {
        Ctx.AddTextCardBlock(TStringBuf("market__beru_no_sku"));
        ChangeStep(EChoiceState::Choice);
        return;
    }

    ui64 sku = Ctx.GetBeruOrderSku();
    state.Sku() = sku;

    TCheckoutUserInfo userInfo(Ctx);
    userInfo.Init(true /* fullUpdate */);

    const auto& skuOffers = TReportClient(Ctx).GetSkuOffers(sku).GetResponse();

    const auto& results = skuOffers->Search().Results();
    if (results.Empty() || results[0].Offers().Items().Empty()) {
        HandleSkuOutdated();
        return;
    }
    const auto& offer = results[0].Offers().Items(0);
    state.Cart().Offer() = offer;
    state.Cart().ItemsNumber() = 1;

    ProcessItemsNumber(state, userInfo);
}

void TMarketCheckout::HandleItemsNumber(TState& state)
{
    ui64 availableItemsNumber = GetSkuAvailableItemsNumber(state);
    if (availableItemsNumber == 0) {
        HandleSkuOutdated();
        return;
    }

    i64 userItemsNumber;
    if (Form == EChoiceForm::CheckoutItemsNumber) {
        userItemsNumber = Ctx.GetItemsNumber();
    } else {
        userItemsNumber = GetIntegerSlotValue(Ctx, TStringBuf("index")).GetOrElse(0);
    }

    if (userItemsNumber <= 0) {
        if (!CheckExitDueAttemptsLimit()) {
            RenderAskItemsNumber(TStringBuf("market_checkout__invalid_items_number"), availableItemsNumber);
        }
        return;
    }
    if (availableItemsNumber < static_cast<ui64>(userItemsNumber)) {
        if (!CheckExitDueAttemptsLimit()) {
            NSc::TValue data;
            data["available_items_number"] = availableItemsNumber;
            RenderAskItemsNumber(
                TStringBuf("market_checkout__not_enough_items"),
                availableItemsNumber,
                data);
        }
        return;
    }

    state.Cart().ItemsNumber() = userItemsNumber;

    TCheckoutUserInfo userInfo(Ctx);
    userInfo.Init();
    ProcessEmail(state, userInfo);
}

void TMarketCheckout::HandleEmail(TState& state)
{
    TCheckoutUserInfo userInfo(Ctx);
    userInfo.Init(true /* fullUpdate */);

    if (!userInfo.HasEmail()) {
        auto email = GetSlotOrUtterance(Ctx, TStringBuf("email"));

        if (!TryFormatEmail(email)) {
            if (!CheckExitDueAttemptsLimit()) {
                // todo remove when malisa-462 will be in prod
                NSc::TValue data;
                data["ask_user_to_login"] = true;
                // end of todo
                Ctx.AddTextCardBlock(TStringBuf("market_checkout__invalid_email"), data);
                AddUserLoginedSuggest(Ctx);
            }
            return;
        }

        state.Email() = email;
    }

    ProcessPhone(state, userInfo);
}

void TMarketCheckout::HandlePhone(TState& state)
{
    TCheckoutUserInfo userInfo(Ctx);
    userInfo.Init();

    auto phone = GetSlotOrUtterance(Ctx, TStringBuf("phone"));
    if (!TryFormatPhone(phone)) {
        if (!CheckExitDueAttemptsLimit()) {
            Ctx.AddTextCardBlock(TStringBuf("market_checkout__invalid_phone"));
        }
        return;
    }

    state.Phone() = phone;

    ProcessAddress(state, userInfo);
}

void TMarketCheckout::RenderAskAddress(TStringBuf textCardName, const TState& state)
{
    NSc::TValue data;
    auto& deliverySuggests = data["delivery_suggests"].SetArray();
    for (const auto& suggest : state.DeliverySuggests()) {
        auto& data = deliverySuggests.Push();
        data["address"] = ToAddressString(suggest.Delivery().Address());
        data["type"] = suggest.Delivery().Type();
        data["prices"]["min"] = suggest.Prices().Min();
        data["prices"]["max"] = suggest.Prices().Max();
        if (suggest.Delivery().Type() == TStringBuf("PICKUP")) {
            data["outlet_name"] = suggest.Delivery().Outlet().Name();
        }
    }
    Ctx.AddTextCardBlock(textCardName, data);
    if (deliverySuggests.ArraySize() == 1) {
        AddConfirmSuggests(Ctx);
    } else if (deliverySuggests.ArraySize() > 1) {
        AddNumberSuggests(Ctx, deliverySuggests.ArraySize());
    }
}

void TMarketCheckout::HandleDeliverySuggestSelection(
    TState& state,
    const TCheckoutUserInfo& userInfo,
    size_t suggestIdx)
{
    const auto& selectedDelivery = state.DeliverySuggests(suggestIdx).Delivery();
    if (selectedDelivery.Type() == TStringBuf("PICKUP")) {
        if (TrySetPickupDeliveryAndOption(state, userInfo, selectedDelivery)) {
            RenderCheckoutConfirmation(state, userInfo);
        } else {
            state.DeliverySuggests().GetRawValue()->Delete(suggestIdx);
            RenderEmptyDeliveryOptions(state);
        }
    } else {
        if (!TryHandleDeliveryOptions(state, userInfo, selectedDelivery)) {
            state.DeliverySuggests().GetRawValue()->Delete(suggestIdx);
            RenderEmptyDeliveryOptions(state);
        }
    }
}

void TMarketCheckout::HandleAddress(TState& state)
{
    TCheckoutUserInfo userInfo(Ctx);
    userInfo.Init();

    if (Form == EChoiceForm::CheckoutYesOrNo && state.DeliverySuggests().Size() == 1) {
        TStringBuf confirmation = Ctx.GetConfirmation().GetString();
        if (confirmation == TStringBuf("no")) {
            state.DeliverySuggests().Clear();
            RenderAskAddress(TStringBuf("market_checkout__ask_address"), state);
            return;
        }

        Y_ASSERT(confirmation == TStringBuf("yes"));
        HandleDeliverySuggestSelection(state, userInfo, 0);
        return;
    } else if (Form == EChoiceForm::CheckoutIndex && state.DeliverySuggests().Size() > 1) {
        auto selectedIndex = GetIndex(Ctx).GetOrElse(0) - 1;
        if (selectedIndex < static_cast<i64>(state.DeliverySuggests().Size()) && selectedIndex >= 0) {
            HandleDeliverySuggestSelection(state, userInfo, static_cast<size_t>(selectedIndex));
            return;
        } else {
            RenderAddressError(TStringBuf("market_checkout__invalid_index"), state);
            return;
        }
    }

    TMaybe<TGeoPosition> userPosition;
    if (Ctx.Meta().HasLocation()) {
        userPosition = InitGeoPositionFromLocation(Ctx.Meta().Location());
    }

    auto address = GetSlotOrUtterance(Ctx, TStringBuf("address"));

    if (!address) {
        RenderInvalidAddress(state);
        return;
    }

    const auto& buyerAddress = RequestAddressResolution(Ctx, address, userPosition);

    if (buyerAddress) {
        TSchemeHolder<TDeliveryScheme> delivery;
        SetCourierDelivery(buyerAddress.GetRef().Scheme(), delivery.Scheme());
        if (!TryHandleDeliveryOptions(state, userInfo, delivery.Scheme())) {
            RenderEmptyDeliveryOptions(state);
        }
    } else {
        RenderInvalidAddress(state);
    }
}

bool TMarketCheckout::TrySetPickupDeliveryAndOption(
    TState& state,
    const TCheckoutUserInfo& userInfo,
    const TDeliveryScheme& delivery)
{
    const auto& cartResponseMaybe = MakeCheckouterCartRequest(state, userInfo, delivery);
    if (!cartResponseMaybe.Defined()) {
        return false;
    }

    auto optionMaybe = GetPickupDeliveryOption(cartResponseMaybe.GetRef()->Carts(0), delivery.Outlet().Id());
    if (optionMaybe.Defined()) {
        state.Delivery() = delivery;
        state.DeliveryOption() = optionMaybe.GetRef().Scheme();
        return true;
    }
    return false;
}

TResponseHandle<TCheckouterData> TMarketCheckout::MakeAsyncCheckouterCartRequest(
    TState& state,
    const TCheckoutUserInfo& userInfo,
    const TDeliveryScheme& delivery)
{
    return TCheckouterClient(Ctx).CartAsync(
        state.Cart(),
        userInfo,
        delivery);
}

TMaybe<TCheckouterData> TMarketCheckout::MakeCheckouterCartRequest(
    TState& state,
    const TCheckoutUserInfo& userInfo,
    const TDeliveryScheme& delivery)
{
    const auto& cartHttpResponse = TCheckouterClient(Ctx).Cart(
        state.Cart(),
        userInfo,
        delivery);
    return GetCartResponse(cartHttpResponse);
}

bool TMarketCheckout::TryHandleDeliveryOptions(
    TState& state,
    const TCheckoutUserInfo& userInfo,
    const TDeliveryScheme& delivery)
{
    const auto& cartResponseMaybe = MakeCheckouterCartRequest(state, userInfo, delivery);
    if (!cartResponseMaybe.Defined()) {
        return false;
    }
    auto stateDeliveryOptions = state.DeliveryOptions();

    for (const auto& deliveryOption : GetCourierDeliveryOptions(cartResponseMaybe.GetRef()->Carts(0))) {
        stateDeliveryOptions.Add() = deliveryOption.Scheme();
    }
    if (stateDeliveryOptions.Empty()) {
        return false;
    }
    state.Delivery() = delivery;
    ChangeStep(EChoiceState::CheckoutDeliveryOptions);

    if (stateDeliveryOptions.Size() == 1) {
        HandleDeliveryOptions(state, 1);
        return true;
    }

    Ctx.AddTextCardBlock(TStringBuf("market_checkout__ask_delivery_options"));
    AddNumberSuggests(Ctx, stateDeliveryOptions.Size());
    return true;
}

void TMarketCheckout::RenderInvalidAddress(TState& state)
{
    RenderAddressError(TStringBuf("market_checkout__invalid_address"), state);
}

void TMarketCheckout::RenderEmptyDeliveryOptions(TState& state)
{
    RenderAddressError(TStringBuf("market_checkout__empty_delivery_options"), state);
}

void TMarketCheckout::RenderAddressError(
    TStringBuf textCardName,
    TState& state)
{
    if (State != EChoiceState::CheckoutAddress) {
        state.Delivery().GetRawValue()->SetNull();
        RenderAskAddress(TStringBuf("market_checkout__ask_address"), state);
        ChangeStep(EChoiceState::CheckoutAddress);
    } else {
        if (!CheckExitDueAttemptsLimit()) {
            RenderAskAddress(textCardName, state);
        }
        return;
    }
}

void TMarketCheckout::HandleDeliveryOptions(TState& state)
{
    auto deliveryOptionIndex = Form == EChoiceForm::CheckoutDeliveryIntervals
                               ? DeliveryIntervalsWorker.Handle(state)
                               : GetIndex(Ctx);
    HandleDeliveryOptions(state, deliveryOptionIndex);
}

void TMarketCheckout::HandleDeliveryOptions(TState& state, const TMaybe<i64>& deliveryOptionIndex)
{
    if (!deliveryOptionIndex
        || *deliveryOptionIndex <= 0
        || (ui64) *deliveryOptionIndex > state.DeliveryOptions().Size()) {
        if (!CheckExitDueAttemptsLimit()) {
            Ctx.AddTextCardBlock("market_checkout__invalid_index");
            AddNumberSuggests(Ctx, state.DeliveryOptions().Size());
        }
        return;
    }

    state.DeliveryOption() = state.DeliveryOptions(*deliveryOptionIndex - 1);

    TCheckoutUserInfo userInfo(Ctx);
    userInfo.Init();
    RenderCheckoutConfirmation(state, userInfo);
}

void TMarketCheckout::RenderCheckoutConfirmation(TState& state, const TCheckoutUserInfo& userInfo)
{
    ChangeStep(EChoiceState::CheckoutConfirmOrder);
    // TODO: переделать на данные, которые передаются AddTextCardBlock,
    // когда это будет поддержано
    if (!state.HasEmail()) {
        state.Email() = userInfo.GetEmail();
    }
    if (!state.HasPhone()) {
        state.Phone() = userInfo.GetPhone();
    }

    NSc::TValue cardContext;
    cardContext["email"] = userInfo.GetEmail();
    cardContext["phone"] = userInfo.GetPhone();
    cardContext["address"] = ToAddressString(state.Delivery().Address());
    if (state.Delivery().Type() == "PICKUP") {
        cardContext["outlet_name"] = state.Delivery().Outlet().Name();
    }
    cardContext["delivery_type"] = state.Delivery().Type();
    cardContext["delivery_interval"] = *state.DeliveryOption().Dates().GetRawValue();

    ui64 offerPrice = FromString<ui64>(state.Cart().Offer().Prices().Value());
    ui64 deliveryPrice = state.DeliveryOption().Price();
    ui64 itemsNumber = state.Cart().ItemsNumber();
    cardContext["items_number"] = itemsNumber;
    cardContext["total_price"] = deliveryPrice + offerPrice * itemsNumber;
    cardContext["offer_price"] = offerPrice;
    cardContext["delivery_price"] = deliveryPrice;

    cardContext["offer_title"] = state.Cart().Offer().Titles().Raw();
    auto picture = TPicture::GetMostSuitablePicture(*state.Cart().Offer().GetRawValue());
    NBassApi::TPicture<TBoolSchemeTraits> pictureScheme(&cardContext["offer_picture"]);
    SerializePicture(picture, pictureScheme);

    AddConfirmCheckoutSuggests(Ctx);
    Ctx.AddDivCardBlock(TStringBuf("market_order_details"), cardContext);

    // todo remove when MALISA-337 will be in prod
    NSc::TValue textCardContext = cardContext;
    textCardContext["deliveryInterval"] = textCardContext["delivery_interval"];
    textCardContext["deliveryPrice"] = textCardContext["delivery_price"];
    textCardContext["offerPrice"] = textCardContext["offer_price"];
    // end of todo
    Ctx.AddTextCardBlock(TStringBuf("market_checkout__confirm"), textCardContext);
}

void TMarketCheckout::HandleConfirmOrder(TState& state)
{
    if (Form != EChoiceForm::CheckoutYesOrNo) {
        if (!CheckExitDueAttemptsLimit()) {
            AddConfirmCheckoutSuggests(Ctx);
            Ctx.AddTextCardBlock(TStringBuf("market_checkout__invalid_confirm"));
        }
        return;
    }

    auto confirmation = GetStringSlotValue(Ctx, TStringBuf("confirmation"));
    if (!confirmation) {
        ythrow TMarketException(TStringBuf("slot 'confirmation' is empty"));
    }

    if (*confirmation == TStringBuf("yes")) {
        Checkout(state);
    } else if (*confirmation == TStringBuf("no")) {
        ManualCheckout(state);
    } else {
        AddConfirmCheckoutSuggests(Ctx);
        Ctx.AddTextCardBlock(TStringBuf("market_checkout__invalid_confirm"));
    }
}

void TMarketCheckout::Checkout(TState& state)
{
    if (!Ctx.Meta().EndOfUtterance()) {
        return;
    }
    TCheckoutUserInfo userInfo(Ctx);
    userInfo.Init();

    if (!TFakeUsers::IsAllowedCheckout(state, userInfo)) {
        ChangeStep(EChoiceState::CheckoutComplete);
        NSc::TValue cardData;
        cardData["email"] = userInfo.GetEmail();
        Ctx.AddTextCardBlock(TStringBuf("market_common__checkout_is_not_allowed_for_this_user"), cardData);
        return;
    }

    const TString aliceId = TStringBuilder() << state.Cart().Offer().Model().Id() << ":" << StartTime.MicroSeconds();

    const TString notes = TStringBuilder() << TStringBuf("Заказ через Алису - ") << aliceId;

    state.Order().AliceId() = aliceId;
    state.Order().CheckoutedAtTimestamp() = StartTime.MicroSeconds();
    state.Order().Attempt() = 0;

    const auto& checkoutHttpResponse = TCheckouterClient(Ctx).Checkout(
        state.Cart(),
        state.Delivery(),
        state.DeliveryOption(),
        userInfo,
        notes);

    if (checkoutHttpResponse.IsTimeout()) {
        try {
            LOG(DEBUG)
                << "Checkout request is too long. Ask user to wait some more time.\n"
                << "Checkout request message: " << checkoutHttpResponse.GetErrorText() << Endl;
        } catch(...) {
        }
        ChangeStep(EChoiceState::CheckoutWaiting);
        Ctx.AddTextCardBlock(TStringBuf("market__checkout_wait"));
        AddCheckoutWaitSuggests(Ctx);
        return;
    }
    if (!checkoutHttpResponse.IsHttpOk()) {
        ChangeStep(EChoiceState::CheckoutComplete);
        Ctx.AddTextCardBlock(TStringBuf("market__checkout_failed"));
        return;
    }

    const auto& checkoutResponse = checkoutHttpResponse.GetResponse();

    if (!checkoutResponse.IsCheckedOut() || checkoutResponse.GetOrders().size() != 1) {
        ChangeStep(EChoiceState::CheckoutComplete);
        Ctx.AddTextCardBlock(TStringBuf("market__checkout_failed"));
        return;
    }

    auto orderId = checkoutResponse.GetOrders()[0].GetId();
    HandleSuccessCheckout(orderId, userInfo.IsGuest());
}

void TMarketCheckout::ManualCheckout(TState& state)
{
    TCheckoutUserInfo userInfo(Ctx);
    userInfo.Init();

    const auto& skuOffers = TReportClient(Ctx).GetSkuOffers(state.Sku()).GetResponse();

    const auto& results = skuOffers->Search().Results();
    if (results.Empty() || results[0].Offers().Items().Empty()) {
        ythrow TMarketException(TStringBuf("blue offers were not found"));
    }
    const auto& offer = results[0].Offers().Items(0);

    const TString url = TMarketUrlBuilder(Ctx).GetBeruCheckoutUrl(
        offer.WareId(),
        offer.FeeShow(),
        FromString(offer.Prices().Value().Get()));

    ChangeStep(EChoiceState::CheckoutComplete);
    Ctx.AddSuggest("market__manual_checkout", NSc::TValue(url));
    Ctx.AddTextCardBlock(TStringBuf("market__manual_checkout"));
}

void TMarketCheckout::HandleCheckoutWaiting(TState& state)
{
    TCheckoutUserInfo userInfo(Ctx);
    userInfo.Init();

    if (TryCheckOrderExists(state, userInfo)) {
        return;
    }

    auto waitUpperBound = TInstant::FromValue(state.Order().CheckoutedAtTimestamp()) + Ctx.GetConfig().Market().MaxCheckoutWaitDuration();

    if (StartTime > waitUpperBound) {
        ChangeStep(EChoiceState::CheckoutComplete);
        Ctx.AddTextCardBlock(TStringBuf("market__checkout_failed"));
        return;
    }

    state.Order().Attempt() += 1;
    if (state.Order().Attempt() >= MAX_CHECKOUT_ATTEMPTS) {
        ChangeStep(EChoiceState::CheckoutComplete);
        Ctx.AddTextCardBlock("market__checkout_failed");
        return;
    }
    Ctx.AddTextCardBlock("market__checkout_wait");
    AddCheckoutWaitSuggests(Ctx);
}

void TMarketCheckout::HandleWrongState()
{
    LOG(ERR) << "Invalid state " << State << TStringBuf(" for form ") << Form << Endl;
    ChangeStep(EChoiceState::CheckoutComplete);
}

EChoiceState TMarketCheckout::StateFromOldStep(TStringBuf step)
{
    if (step == ITEMS_NUMBER_STEP) {
        return EChoiceState::CheckoutItemsNumber;
    }
    if (step == EMAIL_STEP) {
        return EChoiceState::CheckoutEmail;
    }
    if (step == PHONE_STEP) {
        return EChoiceState::CheckoutPhone;
    }
    if (step == ADDRESS_STEP) {
        return EChoiceState::CheckoutAddress;
    }
    if (step == DELIVERY_OPTIONS_STEP) {
        return EChoiceState::CheckoutDeliveryOptions;
    }
    if (step == CONFIRM_ORDER_STEP) {
        return EChoiceState::CheckoutConfirmOrder;
    }
    if (step == CHECKOUT_WAITING_STEP) {
        return EChoiceState::CheckoutWaiting;
    }
    if (step == CHECKOUT_COMPLETE) {
        return EChoiceState::CheckoutComplete;
    }
    return EChoiceState::Null;
}

TStringBuf TMarketCheckout::StateToOldStep(EChoiceState state)
{
    switch (state) {
        case EChoiceState::Null:
        case EChoiceState::Activation:
        case EChoiceState::Choice:
        case EChoiceState::ProductDetails:
        case EChoiceState::ProductDetailsExternal:
        case EChoiceState::BeruProductDetails:
        case EChoiceState::MakeOrder:
        case EChoiceState::ActivationOpen:
        case EChoiceState::ChoiceOpen:
        case EChoiceState::ProductDetailsOpen:
        case EChoiceState::BeruProductDetailsOpen:
        case EChoiceState::MakeOrderOpen:
        case EChoiceState::Exit:
            return TStringBuf();
        case EChoiceState::CheckoutItemsNumber:
            return ITEMS_NUMBER_STEP;
        case EChoiceState::CheckoutEmail:
            return EMAIL_STEP;
        case EChoiceState::CheckoutPhone:
            return PHONE_STEP;
        case EChoiceState::CheckoutAddress:
            return ADDRESS_STEP;
        case EChoiceState::CheckoutDeliveryOptions:
            return DELIVERY_OPTIONS_STEP;
        case EChoiceState::CheckoutConfirmOrder:
            return CONFIRM_ORDER_STEP;
        case EChoiceState::CheckoutWaiting:
            return CHECKOUT_WAITING_STEP;
        case EChoiceState::CheckoutComplete:
            return CHECKOUT_COMPLETE;
    }
}

void TMarketCheckout::ChangeStep(EChoiceState state)
{
    State = state;
    Ctx.SetState(State);
    auto checkoutState = Ctx.GetCheckoutState();
    checkoutState.Step() = StateToOldStep(state);
    checkoutState.Attempt() = 0;
}

bool TMarketCheckout::CheckExitDueAttemptsLimit()
{
    auto state = Ctx.GetCheckoutState();
    state.Attempt() += 1;
    state.AttemptReminder() = (state.Attempt() + 1 == MAX_CHECKOUT_STEP_ATTEMPTS_COUNT);
    if (state.Attempt() >= MAX_CHECKOUT_STEP_ATTEMPTS_COUNT) {
        ChangeStep(EChoiceState::Exit);
        Ctx.RenderChoiceAttemptsLimit();
        return true;
    }
    return false;
}

void TMarketCheckout::ProcessItemsNumber(TState& state, const TCheckoutUserInfo& userInfo)
{
    ChangeStep(EChoiceState::CheckoutItemsNumber);

    ui64 availableItemsNumber = GetSkuAvailableItemsNumber(state);
    if (availableItemsNumber == 0) {
        HandleSkuOutdated();
        return;
    }
    if (availableItemsNumber > 1) {
        RenderAskItemsNumber(TStringBuf("market_checkout__ask_items_number"), availableItemsNumber);
    } else {
        state.Cart().ItemsNumber() = 1;
        ProcessEmail(state, userInfo);
    }
}

void TMarketCheckout::ProcessEmail(TState& state, const TCheckoutUserInfo& userInfo)
{
    ChangeStep(EChoiceState::CheckoutEmail);
    if (userInfo.HasEmail()) {
        ProcessPhone(state, userInfo);
        return;
    }

    Ctx.AddTextCardBlock(TStringBuf("market_checkout__ask_email"));
    AddUserLoginedSuggest(Ctx);
}

void TMarketCheckout::ProcessPhone(TState& state, const TCheckoutUserInfo& userInfo)
{
    ChangeStep(EChoiceState::CheckoutPhone);
    if (userInfo.HasPhone()) {
        ProcessAddress(state, userInfo);
        return;
    }

    NSc::TValue cardCtx;
    NBassApi::TCheckoutAskPhoneTextCardData<TBoolSchemeTraits> cardCtxScheme(&cardCtx);
    cardCtxScheme.IsGuest() = userInfo.IsGuest();
    Y_ASSERT(cardCtxScheme.Validate());
    Ctx.AddTextCardBlock(TStringBuf("market_checkout__ask_phone"), cardCtx);
}

void TMarketCheckout::SetDeliverySuggests(TState& state, const TCheckoutUserInfo& userInfo) {
    // create suggests
    TVector<TSchemeHolder<TDeliveryScheme>> deliveries;
    if (userInfo.HasLastAddressDelivery()) {
        SetCourierDelivery(userInfo.GetLastAddress(), deliveries.emplace_back().Scheme());
    }
    if (userInfo.HasLastPickupDelivery()) {
        SetPickupDelivery(
            userInfo.GetLastPickupAddress(),
            userInfo.GetLastPickupOutletId(),
            userInfo.GetLastPickupOutletName(),
            deliveries.emplace_back().Scheme());
    }

    // request /cart for every delivery
    TVector<TResponseHandle<TCheckouterData>> handles(Reserve(deliveries.size()));
    for (const auto& delivery : deliveries) {
        handles.push_back(MakeAsyncCheckouterCartRequest(state, userInfo, delivery.Scheme()));
    }

    // check deliveries are available and fill them
    for (size_t i = 0; i < deliveries.size(); i++) {
        const auto& delivery = deliveries[i];
        const auto& cartResponseMaybe = GetCartResponse(handles[i].Wait());
        if (!cartResponseMaybe.Defined()) {
            continue;
        }
        const auto& cart = cartResponseMaybe.GetRef()->Carts(0);

        if (delivery->Type() == TStringBuf("COURIER")) {
            const auto& options = GetCourierDeliveryOptions(cart);
            if (!options.empty()) {
                auto suggest = state.DeliverySuggests().Add();
                suggest.Delivery() = delivery.Scheme();

                const auto& cmp = [](
                    const TSchemeHolder<TDeliveryOptionScheme>& a,
                    const TSchemeHolder<TDeliveryOptionScheme>& b)
                {
                    return a->Price() < b->Price();
                };
                suggest.Prices().Min() = (*std::min_element(options.begin(), options.end(), cmp))->Price();
                suggest.Prices().Max() = (*std::max_element(options.begin(), options.end(), cmp))->Price();
            }
        } else if (delivery->Type() == TStringBuf("PICKUP")) {
            const auto& optionMaybe = GetPickupDeliveryOption(cart, delivery->Outlet().Id());
            if (optionMaybe.Defined()) {
                auto suggest = state.DeliverySuggests().Add();
                suggest.Delivery() = delivery.Scheme();
                auto price = optionMaybe.GetRef()->Price();
                suggest.Prices().Min() = price;
                suggest.Prices().Max() = price;
            }
        }
    }
}

void TMarketCheckout::ProcessAddress(TState& state, const TCheckoutUserInfo& userInfo)
{
    ChangeStep(EChoiceState::CheckoutAddress);
    if (userInfo.HasLastAddressDelivery() && !userInfo.HasLastPickupDelivery()) {
        TSchemeHolder<TDeliveryScheme> delivery;
        SetCourierDelivery(userInfo.GetLastAddress(), delivery.Scheme());
        if (!TryHandleDeliveryOptions(state, userInfo, delivery.Scheme())) {
            RenderAskAddress(TStringBuf("market_checkout__ask_address"), state);
        }
        return;
    }

    SetDeliverySuggests(state, userInfo);
    RenderAskAddress(TStringBuf("market_checkout__ask_address"), state);
}

bool TMarketCheckout::TryCheckOrderExists(const TState& state, const TCheckoutUserInfo& userInfo)
{
    const auto& orders = TCheckouterClient(Ctx)
        .GetOrdersByUid(userInfo.GetUid(), 1 /* pageSize */, state.Order().AliceId()).GetResponse();

    if (orders.HasOrders()) {
        Y_ASSERT(orders.GetOrders().size() == 1);
        HandleSuccessCheckout(orders.GetOrders()[0].GetId(), userInfo.IsGuest());
    }
    return orders.HasOrders();
}

void TMarketCheckout::HandleSuccessCheckout(i64 orderId, bool guestMode)
{
    ChangeStep(EChoiceState::CheckoutComplete);
    if (!guestMode) {
        Ctx.AddSuggest("market__checkout_complete", NSc::TValue(TMarketUrlBuilder(Ctx).GetBeruOrderUrl(orderId)));
    }
    NSc::TValue checkoutCompleteData;
    checkoutCompleteData["order_id"] = orderId;
    Ctx.AddTextCardBlock(TStringBuf("market__checkout_complete"), checkoutCompleteData);
}

void TMarketCheckout::SleepMaxAllowableTime() const
{
    SleepUntil(StartTime + MAX_REQUEST_HANDLE_DURATION);
}

bool TMarketCheckout::TryFormatEmail(TString& email)
{
    StripInPlace(email).to_lower();
    return std::regex_match(email.c_str(), EMAIL_REGEX);
}

bool TMarketCheckout::TryFormatPhone(TString& phone)
{
    TString formattedPhone;
    formattedPhone = std::regex_replace(phone.c_str(), REPLACE_NON_DIGITS_REGEX, "");
    const auto phoneSize = formattedPhone.size();
    if (phoneSize != 10 && phoneSize != 11) {
        return false;
    }
    phone.clear();
    phone.append(TStringBuf("+7")).append(formattedPhone.substr(phoneSize - 10));
    return true;
}

} // namespace NMarket

} // namespace NBASS

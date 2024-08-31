#include "market_recurring_purchase.h"

#include "client/checkouter_client.h"
#include "client/geo_client.h"
#include "client/stock_storage_client.h"
#include "delivery_builder.h"
#include "delivery_intervals_worker.h"
#include "dynamic_data.h"
#include "fake_users.h"
#include "market_exception.h"
#include "market_geo_support.h"
#include "market_url_builder.h"
#include "settings.h"
#include "util/report.h"
#include "util/serialize.h"
#include "util/string.h"
#include "util/suggests.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <util/string/cast.h>
#include <util/string/join.h>

#include <regex>

namespace NBASS {

namespace NMarket {

namespace {

const std::regex REPLACE_NON_DIGITS_REGEX("[^\\d]");

const ui32 MAX_STEP_ATTEMPTS_COUNT = 3;

const ui32 ORDERS_PAGE_SIZE = 100;

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

void AddWarningsToDetails(const TVector<TWarning>& warnings, NSc::TValue& details)
{
    if (!warnings.empty()) {
        TWarningsScheme warningsScheme(&details["warnings"]);
        SerializeWarnings(warnings, warningsScheme);
    }
}

void SetCourierDelivery(const TAddressSchemeConst& address, TDeliveryScheme delivery)
{
    delivery.Type() = "COURIER";
    delivery.Address() = address;
}

IParallelHandler::TTryResult ToTryResult(TResultValue res)
{
    if (res.Defined()) {
        return res.GetRef();
    }
    return IParallelHandler::ETryResult::Success;
}

}

TMarketRecurringPurchase::TMarketRecurringPurchase(TMarketContext& ctx, bool isParallelMode)
    : TBaseMarketChoice(ctx)
    , InitialQueryInfo(Ctx.GetTextRedirect(), Ctx.GetPrice(), Ctx.GetCgiGlFilters())
    , Form(FromString<ERecurringPurchaseForm>(Ctx.FormName()))
    , State(Ctx.GetState(ERecurringPurchaseState::Null))
    , IsScreenless(Ctx.IsScreenless())
    , IsFirstRequest(false)
    , CheckoutState(Ctx.GetCheckoutState())
    , UserInfo(Ctx)
    , StartTime(Now())
    , NumberFilterWorker(Ctx)
    , IsParallelMode(isParallelMode)
    , GeoSupport(Ctx.Ctx().GlobalCtx())
{
    UserInfo.Init(true);
    Ctx.SetChoiceMarketType(EMarketType::BLUE);
}

IParallelHandler::TTryResult TMarketRecurringPurchase::TryDo()
{
    if (Form == ERecurringPurchaseForm::Activation && IsParallelMode) {
        if (CheckNoActivation()) {
            return IParallelHandler::ETryResult::NonSuitable;
        }
        auto again = GetStringSlotValue(Ctx, TStringBuf("again"));
        if (!again || *again != TStringBuf("market_again")) {
            return IParallelHandler::ETryResult::NonSuitable;
        }
        if (Ctx.Request(true).empty()) {
            return IParallelHandler::ETryResult::NonSuitable;
        }
    }
    if (!GeoSupport.IsBeruSupportedForGeoId(Ctx.UserRegion(), Ctx.GetScenarioType())) {
        Ctx.RenderMarketNotSupportedInLocation(EMarketType::BLUE, GeoSupport.GetRegionName(Ctx.UserRegion()));
        return IParallelHandler::ETryResult::NonSuitable;
    }
    return ToTryResult(Do());
}

TResultValue TMarketRecurringPurchase::DoImpl()
{
    Ctx.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::BERU);
    if (CheckNoActivation()) {
        Ctx.AddTextCardBlock("recurring_purchase__no_activation");
        return TResultValue();
    }
    if (Form == ERecurringPurchaseForm::Cancel) {
        return HandleCancel();
    }
    if (UserInfo.IsGuest()) {
        return HandleGuest();
    }
    if (!UserInfo.HasEmail()) {
        return HandleNoEmail();
    }

    switch (Form) {
        case ERecurringPurchaseForm::Activation:
        case ERecurringPurchaseForm::Login:
            IsFirstRequest = true;
            return HandleRecurringPurchase();
        case ERecurringPurchaseForm::Ellipsis:
            return HandleRecurringPurchase();
        case ERecurringPurchaseForm::NumberFilter:
            return HandleNumberFilter();
        case ERecurringPurchaseForm::Cancel:
            return HandleCancel();
        case ERecurringPurchaseForm::Garbage:
            return HandleGarbage();
        case ERecurringPurchaseForm::ProductDetails:
            return HandleSkuOfferDetails();
        case ERecurringPurchaseForm::Repeat:
            return TResultValue();
        case ERecurringPurchaseForm::Checkout:
            return HandleCheckoutStart();
        case ERecurringPurchaseForm::CheckoutItemsNumber:
            return HandleItemsNumber();
        case ERecurringPurchaseForm::CheckoutIndex:
            switch (State) {
                case ERecurringPurchaseState::SelectProductIndex:
                    return HandleProductIndex();
                case ERecurringPurchaseState::CheckoutDeliveryOptions:
                    return HandleDeliveryOptions();
                default:
                    return HandleWrongState();
            }
        case ERecurringPurchaseForm::CheckoutDeliveryIntervals:
            return HandleDeliveryOptions();
        case ERecurringPurchaseForm::CheckoutYesOrNo:
            switch (State) {
                case ERecurringPurchaseState::ProductDetailsScreenless:
                    return HandleCheckoutStartScreenless();
                case ERecurringPurchaseState::CheckoutConfirmOrder:
                    return HandleConfirmOrder();
                case ERecurringPurchaseState::CheckoutDeliveryFirstOption:
                    return HandleDeliveryFirstOption();
                default:
                    return HandleWrongState();
            }
        case ERecurringPurchaseForm::CheckoutSuits:
            switch (State) {
                case ERecurringPurchaseState::CheckoutDeliveryFirstOption:
                    return HandleDeliveryFirstOption();
                default:
                    return HandleWrongState();
            }
        case ERecurringPurchaseForm::CheckoutEverything:
        {
            switch (State) {
                case ERecurringPurchaseState::Null:
                case ERecurringPurchaseState::CheckoutComplete:
                case ERecurringPurchaseState::Exit:
                    return HandleWrongState();
                case ERecurringPurchaseState::RecurringPurchase:
                case ERecurringPurchaseState::Choice:
                case ERecurringPurchaseState::ProductDetails:
                    return HandleGarbage();
                case ERecurringPurchaseState::ProductDetailsScreenless:
                    return HandleCheckoutStartScreenless();
                case ERecurringPurchaseState::SelectProductIndex:
                    return HandleProductIndex();
                case ERecurringPurchaseState::Login:
                    return HandleRecurringPurchase();
                case ERecurringPurchaseState::CheckoutItemsNumber:
                    return HandleItemsNumber();
                case ERecurringPurchaseState::CheckoutPhone:
                    return HandlePhone();
                case ERecurringPurchaseState::CheckoutAddress:
                    return HandleAddress();
                case ERecurringPurchaseState::CheckoutDeliveryOptions:
                    return HandleDeliveryOptions();
                case ERecurringPurchaseState::CheckoutDeliveryFirstOption:
                    return HandleDeliveryFirstOption();
                case ERecurringPurchaseState::CheckoutConfirmOrder:
                    return HandleConfirmOrder();
                case ERecurringPurchaseState::CheckoutWaiting:
                    return HandleCheckoutWaiting();
            }
        }
    }
}

ui64 TMarketRecurringPurchase::GetSkuAvailableItemsNumber() const
{
    const auto& offer = CheckoutState.Cart().Offer();
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

bool TMarketRecurringPurchase::CheckNoActivation()
{
    const auto& clientInfo = Ctx.MetaClientInfo();
    if (!Ctx.GetExperiments().RecurringPurchase()) {
        return true;
    }
    if (clientInfo.IsElariWatch()
        || clientInfo.IsNavigator()
        || clientInfo.IsYaAuto())
    {
        return true;
    }
    return false;
}

TResultValue TMarketRecurringPurchase::HandleGuest()
{
    SetState(ERecurringPurchaseState::Login);
    if (Form == ERecurringPurchaseForm::Login) {
        if (!CheckExitDueAttemptsLimit()) {
            Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__still_no_login"));
        }
    } else {
        Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__login"));
    }
    Ctx.AddAuthorizationSuggest();
    Ctx.AddSuggest(TStringBuf("recurring_purchase__user_logined"));
    return TResultValue();
}

TResultValue TMarketRecurringPurchase::HandleNoEmail()
{
    SetState(ERecurringPurchaseState::Login);
    Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__no_email"));
    Ctx.AddSuggest(TStringBuf("recurring_purchase__user_logined"));
    return TResultValue();
}

TResultValue TMarketRecurringPurchase::HandleWrongState()
{
    TStringBuilder errMessage;
    errMessage << "Wrong state " << State << TStringBuf(" for form ") << Form;
    LOG(ERR) << errMessage << Endl;
    SetState(ERecurringPurchaseState::Exit);
    DeleteCancelSuggest();
    return TError(TError::EType::MARKETERROR, errMessage);
}

TResultValue TMarketRecurringPurchase::HandleGarbage()
{
    if (!CheckExitDueAttemptsLimit()) {
        Ctx.AddTextCardBlock("recurring_purchase__garbage");
    }
    AddCancelSuggest();
    return TResultValue();
}

static TVector<ui64> GetSkus(const TVector<TSkuOrderItem>& skus)
{
    TVector<ui64> result;
    for (const auto& sku: skus) {
        result.push_back(sku.Sku);
    }
    return result;
}


TReportResponse TMarketRecurringPurchase::MakeDefinedDocsRequest(const TVector<ui64>& skus) const
{
    const auto& response = TMarketClient(Ctx).MakeDefinedDocsRequest(
        ToString(Ctx.GetTextRedirect()),
        skus,
        Ctx.GetPrice(),
        EMarketType::BLUE,
        true /* allowRedirects */);
    switch (response.GetRedirectType()) {
        case TReportResponse::ERedirectType::NONE:
            return response;
        case TReportResponse::ERedirectType::REGION:
            Ctx.SetUserRegion(response.GetRegionRedirect().GetUserRegion());
        case TReportResponse::ERedirectType::PARAMETRIC:
        case TReportResponse::ERedirectType::MODEL:
        case TReportResponse::ERedirectType::UNKNOWN:
            return TMarketClient(Ctx).MakeDefinedDocsRequest(
                ToString(Ctx.GetTextRedirect()),
                skus,
                Ctx.GetPrice(),
                EMarketType::BLUE,
                false /* allowRedirects */);
    }
}

TResultValue TMarketRecurringPurchase::HandleRecurringPurchase()
{
    SetState(IsScreenless ? ERecurringPurchaseState::SelectProductIndex : ERecurringPurchaseState::RecurringPurchase);
    AddCancelSuggest();
    if (TDynamicDataFacade::ContainsVulgarQuery(Ctx.Request())) {
        return RenderEmptyResult();
    }
    TryHandlePriceRequest();
    if (!IsPriceRequest()) {
        AppendTextRedirect(Ctx.Request());
    }
    const TStringBuf query = Ctx.GetTextRedirect();
    if (TUtf32String::FromUtf8(query).size() <= 1) {
        LOG(DEBUG) << "Request \"" << query << "\" is too short" << Endl;
        return RenderEmptyResult();
    }
    TCheckouterClient client { Ctx };

    const auto& orders = client.GetAllOrdersByUid(UserInfo.GetUid(), ORDERS_PAGE_SIZE);
    if (orders.empty()) {
        if (IsScreenless) {
            return RenderHasNoOrders();
        }
        return HandleChoice();
    }
    TVector<TSkuOrderItem> skus;
    for (const auto& order : orders) {
        skus.insert(skus.end(), order.GetOrderItems().begin(), order.GetOrderItems().end());
    }

    const auto& response = MakeDefinedDocsRequest(GetSkus(skus));
    if (response.HasError()) {
        return response.GetError();
    }
    if (response.GetTotal() == 0) {
        if (IsScreenless) {
            return RenderEmptyResult();
        }
        return HandleChoice();
    }
    return RenderGallery(response, skus);
}

TResultValue TMarketRecurringPurchase::HandleProductIndex()
{
    const NSc::TValue& documents = Ctx.GetResultModels();
    const TMaybe<i64> productIndex = GetIntegerSlotValue(Ctx, TStringBuf("index"));
    if (Form != ERecurringPurchaseForm::CheckoutIndex
        || !productIndex
        || *productIndex <= 0
        || (ui64)*productIndex > documents.ArraySize())
    {
        if (!CheckExitDueAttemptsLimit()) {
            Ctx.AddTextCardBlock("recurring_purchase__invalid_index");
            AddNumberSuggests(Ctx, documents.ArraySize());
        }
        return TResultValue();
    }

    const size_t index = *productIndex - 1;
    Y_ENSURE(documents.Has(index));

    const NSc::TValue& doc = documents.Get(index);
    const TStringBuf skuKey = "sku";
    Y_ENSURE(doc.Has(skuKey) && doc.Get(skuKey).IsIntNumber());
    const ui64 sku = static_cast<ui64>(doc.Get(skuKey).GetIntNumber());

    Ctx.SetBeruOrderSku(sku);
    return HandleCheckoutStart();
}

TResultValue TMarketRecurringPurchase::HandleNumberFilter()
{
    Y_ENSURE(!IsScreenless);
    if (!Ctx.DoesAnyAmountExist()) {
        LOG(WARNING) << "Classification error. number_filter intent without values. Handle as garbage intent" << Endl;
        return HandleGarbage();
    }
    return HandleRecurringPurchase();
}

TResultValue TMarketRecurringPurchase::HandleChoice()
{
    Y_ENSURE(!IsScreenless);
    SetState(ERecurringPurchaseState::Choice);

    TString originalText = ToString(Ctx.GetTextRedirect());
    auto response = MakeSearchRequestWithRedirects();
    if (!response) {
        Ctx.SetTextRedirect(originalText);
        return TResultValue();
    } else if (response->HasError()) {
        return response->GetError();
    }

    Y_ASSERT(response->GetRedirectType() == TReportResponse::ERedirectType::NONE);
    FilterWorker.UpdateFiltersDescription(response->GetFilters());

    const auto result = HandleNoneRedirection(*response);;

    Ctx.SetTextRedirect(originalText);
    return result;
}

TResultValue TMarketRecurringPurchase::FormalizeFilters()
{
    if (IsPriceRequest()) {
        return TResultValue();
    }

    Y_ASSERT(Ctx.Request() && Ctx.DoesCategoryExist());
    auto response = TMarketClient(Ctx).FormalizeFilterValues(Ctx.GetCategory().GetHid(), Ctx.Request());
    if (response.HasError()) {
        return TResultValue();
    }

    if (!TryUpdateFormalizedNumberFilter(response.GetFormalizedGlFilters())) {
        FilterWorker.AddFormalizedFilters(response.GetFormalizedGlFilters());
    }
    return TResultValue();
}

TMaybe<TReportResponse> TMarketRecurringPurchase::MakeSearchRequestWithRedirects()
{
    static const unsigned MAX_REDIRECT_COUNT = 5;

    auto makeRequest = [this](bool allowRedirects) -> TReportResponse {
        if (Ctx.DoesCategoryExist()) {
            return MakeFilterRequestAsync(allowRedirects).Wait();
        }
        return MakeSearchRequest(
            Ctx.GetTextRedirect(),
            Nothing() /* marketType */,
            allowRedirects,
            Ctx.GetRedirectCgiParams()
        );
    };

    bool formalizedFilters = false;
    for (unsigned redirectCount = 0; redirectCount < MAX_REDIRECT_COUNT; ++redirectCount) {
        if (!formalizedFilters && Ctx.Request() && Ctx.DoesCategoryExist()) {
            FormalizeFilters();
            formalizedFilters = true;
        }

        const TReportResponse response = makeRequest(true /* allowRedirects */);
        if (response.HasError()) {
            return response;
        }
        if (response.GetRedirectType() == TReportResponse::ERedirectType::NONE) {
            return response;
        }
        if (response.GetRedirectType() == TReportResponse::ERedirectType::MODEL) {
            break;
        }

        response.GetRedirect().FillCtx(Ctx);
    }
    return makeRequest(false /* allowRedirects */);
}

TResultValue TMarketRecurringPurchase::HandleCancel()
{
    SetState(ERecurringPurchaseState::Exit);
    Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__exit"));
    return TResultValue();
}

bool TMarketRecurringPurchase::CheckExitDueAttemptsLimit()
{
    const ui32 attempt = Ctx.GetAttempt() + 1;
    Ctx.SetAttempt(attempt);
    if (attempt >= MAX_STEP_ATTEMPTS_COUNT) {
        SetState(ERecurringPurchaseState::Exit);
        DeleteCancelSuggest();
        Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__exit"));
        return true;
    }
    if (attempt + 1 == MAX_STEP_ATTEMPTS_COUNT) {
        Ctx.SetAttemptReminder();
    }
    return false;
}

TResultValue TMarketRecurringPurchase::HandleCheckoutStartScreenless()
{
    auto confirmation = GetStringSlotValue(Ctx, TStringBuf("confirmation"));
    if (Form == ERecurringPurchaseForm::CheckoutYesOrNo && confirmation ) {
        if (*confirmation == TStringBuf("yes")) {
            return HandleCheckoutStart();
        } else {
            return RenderDoNotCheckout();
        }
    }
    if (!CheckExitDueAttemptsLimit()) {
        Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__invalid_confirm"));
        AddConfirmCheckoutSuggests(Ctx);
    }
    return TResultValue();
}

TResultValue TMarketRecurringPurchase::HandleCheckoutStart()
{
    AddCancelSuggest();

    if (!Ctx.HasBeruOrderSku()) {
        Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__beru_no_sku"));
        return TResultValue();
    }

    const ui64 sku = Ctx.GetBeruOrderSku();
    const auto& skuOffers = TReportClient(Ctx).GetSkuOffers(sku).GetResponse();

    const auto& results = skuOffers->Search().Results();
    if (results.Empty() || !results[0].Offers().HasItems() || results[0].Offers().Items().Empty()) {
        return HandleSkuOutdated();
    }
    const auto& offer = results[0].Offers().Items(0);
    CheckoutState.Cart().Offer() = offer;
    CheckoutState.Cart().ItemsNumber() = 1;
    CheckoutState.Sku() = sku;
    Y_ASSERT(UserInfo.HasEmail());

    return ProcessItemsNumber();
}

TResultValue TMarketRecurringPurchase::ProcessItemsNumber()
{
    Ctx.SetState(ERecurringPurchaseState::CheckoutItemsNumber);

    ui64 availableItemsNumber = GetSkuAvailableItemsNumber();
    if (availableItemsNumber == 0) {
        return HandleSkuOutdated();
    }
    if (availableItemsNumber > 1) {
        return RenderAskItemsNumber(TStringBuf("recurring_purchase__ask_items_number"), availableItemsNumber);
    } else {
        CheckoutState.Cart().ItemsNumber() = 1;
        return ProcessPhone();
    }
}

TResultValue TMarketRecurringPurchase::HandleSkuOutdated()
{
    Ctx.AddTextCardBlock(TStringBuf("market__beru_offer_outdated"));
    SetState(ERecurringPurchaseState::Exit);
    return TResultValue();
}

TResultValue TMarketRecurringPurchase::ProcessPhone()
{
    SetState(ERecurringPurchaseState::CheckoutPhone);
    if (UserInfo.HasPhone()) {
        return ProcessAddress();
    }
    Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__ask_phone"));
    return TResultValue();
}

bool TMarketRecurringPurchase::TryFormatPhone(TString& phone)
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

TResultValue TMarketRecurringPurchase::HandleItemsNumber() {
    AddCancelSuggest();

    ui64 availableItemsNumber = GetSkuAvailableItemsNumber();
    if (availableItemsNumber == 0) {
        return HandleSkuOutdated();
    }

    i64 userItemsNumber;
    if (Form == ERecurringPurchaseForm::CheckoutItemsNumber) {
        userItemsNumber = Ctx.GetItemsNumber();
    } else {
        userItemsNumber = GetIntegerSlotValue(Ctx, TStringBuf("index")).GetOrElse(0);
    }

    if (userItemsNumber <= 0) {
        if (!CheckExitDueAttemptsLimit()) {
            return RenderAskItemsNumber(TStringBuf("recurring_purchase__invalid_items_number"), availableItemsNumber);
        }
        return TResultValue();
    }
    if (availableItemsNumber < static_cast<ui64>(userItemsNumber)) {
        if (!CheckExitDueAttemptsLimit()) {
            return RenderAskItemsNumber(TStringBuf("recurring_purchase__not_enough_items"), availableItemsNumber);
        }
        return TResultValue();
    }

    CheckoutState.Cart().ItemsNumber() = userItemsNumber;

    return ProcessPhone();
}

TResultValue TMarketRecurringPurchase::RenderAskItemsNumber(TStringBuf textCardName, ui32 availableItemsNumber)
{
    NSc::TValue data;
    data["available_items_number"] = availableItemsNumber;
    Ctx.AddTextCardBlock(textCardName, data);
    AddNumberSuggests(Ctx, std::min(availableItemsNumber, MAX_ITEMS_NUMBER_SUGGESTS));
    return TResultValue();
}

TResultValue TMarketRecurringPurchase::HandlePhone()
{
    AddCancelSuggest();

    TString phone(Ctx.Utterance());
    if (!TryFormatPhone(phone)) {
        if (!CheckExitDueAttemptsLimit()) {
            Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__invalid_phone"));
        }
        return TResultValue();
    }

    CheckoutState.Phone() = phone;
    return ProcessAddress();
}

TResultValue TMarketRecurringPurchase::ProcessAddress()
{
    SetState(ERecurringPurchaseState::CheckoutAddress);
    if (UserInfo.HasAddress()) {
        bool hasDeliveryOptions = TryHandleDeliveryOptions(UserInfo.GetAddress());
        if (hasDeliveryOptions) {
            return TResultValue();
        }
    }
    Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__ask_address"));
    return TResultValue();
}

TResultValue TMarketRecurringPurchase::HandleAddress() {
    AddCancelSuggest();

    TMaybe<TGeoPosition> userPosition;
    if (Ctx.Meta().HasLocation()) {
        userPosition = InitGeoPositionFromLocation(Ctx.Meta().Location());
    }

    const auto& buyerAddress = RequestAddressResolution(Ctx, Ctx.Utterance(), userPosition);

    if (!buyerAddress) {
        if (!CheckExitDueAttemptsLimit()) {
            Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__invalid_address"));
        }
        return TResultValue();
    }

    bool hasDeliveryOptions = TryHandleDeliveryOptions(buyerAddress->Scheme());
    if (!hasDeliveryOptions) {
        RenderEmptyDeliveryOptions();
    }
    return TResultValue();
}

bool TMarketRecurringPurchase::TryHandleDeliveryOptions(const TAddressSchemeConst& address)
{
    TSchemeHolder<TDeliveryScheme> delivery;
    SetCourierDelivery(address, delivery.Scheme());
    const auto& cartHttpResponse = TCheckouterClient(Ctx).Cart(
        CheckoutState.Cart(),
        UserInfo,
        delivery.Scheme());

    if (!cartHttpResponse.IsHttpOk() && !cartHttpResponse.IsTimeout()) {
        // todo MALISA-240 /cart может вернуть 400, если мы неправильно указали регион, поэтому переспрашиваем адрес
        // нужно рассмотреть все варианты ошибок от чекаутера и правильно их обрабатывать
        LOG(ERR) << "/cart returned error: " << cartHttpResponse.GetErrorText() << Endl;
        return false;
    }

    const auto& cartResponse = cartHttpResponse.GetResponse();
    if (cartResponse->Carts().Size() != 1) {
        // todo MALISA-240 если чекаутер ответит ошибкой, мы выдадим ему "/cart response should have exactly 1 cart".
        // Стоит добавить валидацию ответа от чекаутера
        LOG(ERR) << "/cart did not return a single cart: carts#" << cartResponse->Carts().Size() << Endl;
        return false;
    }

    auto stateDeliveryOptions = CheckoutState.DeliveryOptions();
    const auto& deliveryOptions = cartResponse->Carts(0).DeliveryOptions();

    for (const auto& deliveryOption : deliveryOptions) {
        if (deliveryOption.Type() == TStringBuf("DELIVERY")) {
            FillDeliveryOptions(deliveryOption, stateDeliveryOptions);
        }
    }
    if (stateDeliveryOptions.Empty()) {
        LOG(INFO) << "/cart return empty delivery otions list" << deliveryOptions.Size() << Endl;
        return false;
    }
    CheckoutState.Delivery() = delivery.Scheme();
    if (stateDeliveryOptions.Size() == 1) {
        HandleDeliveryOptions(1);
        return true;
    }

    if (IsScreenless) {
        SetState(ERecurringPurchaseState::CheckoutDeliveryFirstOption);
        Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__ask_delivery_first_option"));
    } else {
        SetState(ERecurringPurchaseState::CheckoutDeliveryOptions);
        Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__ask_delivery_options"));
        AddNumberSuggests(Ctx, stateDeliveryOptions.Size());
    }
    return true;
}

void TMarketRecurringPurchase::FillDeliveryOptions(
    const TCartDeliveryOptionSchemeConst& deliveryOption,
    NDomSchemeRuntime::TArray<TBoolSchemeTraits, TDeliveryOptionScheme>& result)
{
    if (deliveryOption.DeliveryIntervals().Empty()) {
        return;
    }
    for (const auto& interval : deliveryOption.DeliveryIntervals(0).Intervals()) {
        if (result.Size() >= MAX_DELIVERY_OPTIONS_COUNT) {
            break;
        }
        auto resultOption = result.Add();
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

TResultValue TMarketRecurringPurchase::HandleDeliveryFirstOption()
{
    auto confirmation = GetStringSlotValue(Ctx, TStringBuf("confirmation"));
    if (Form == ERecurringPurchaseForm::CheckoutSuits
        && confirmation
        && *confirmation == TStringBuf("yes"))
    {
        return HandleDeliveryOptions(1);
    }
    SetState(ERecurringPurchaseState::CheckoutDeliveryOptions);
    Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__ask_delivery_options_screenless"));
    AddNumberSuggests(Ctx, CheckoutState.DeliveryOptions().Size());
    return TResultValue();
}

TResultValue TMarketRecurringPurchase::HandleDeliveryOptions()
{
    AddCancelSuggest();
    auto deliveryOptionIndex = Form == ERecurringPurchaseForm::CheckoutDeliveryIntervals
                               ? TDeliveryIntervalsWorker(Ctx).Handle(CheckoutState)
                               : GetIntegerSlotValue(Ctx, TStringBuf("index"));
    return HandleDeliveryOptions(deliveryOptionIndex);
}

TResultValue TMarketRecurringPurchase::HandleDeliveryOptions(TMaybe<i64> deliveryOptionIndex)
{
    if (!deliveryOptionIndex
            || *deliveryOptionIndex <= 0
            || (ui64)*deliveryOptionIndex > CheckoutState.DeliveryOptions().Size()) {
        if (!CheckExitDueAttemptsLimit()) {
            Ctx.AddTextCardBlock("recurring_purchase__invalid_index");
            AddNumberSuggests(Ctx, CheckoutState.DeliveryOptions().Size());
        }
        return TResultValue();
    }
    CheckoutState.DeliveryOption() = CheckoutState.DeliveryOptions(*deliveryOptionIndex - 1);

    // TODO: переделать на данные, которые передаются AddTextCardBlock,
    // когда это будет поддержано
    if (!CheckoutState.HasEmail()) {
        CheckoutState.Email() = UserInfo.GetEmail();
    }
    if (!CheckoutState.HasPhone()) {
        CheckoutState.Phone() = UserInfo.GetPhone();
    }

    NSc::TValue cardContext;
    cardContext["email"] = UserInfo.GetEmail();
    cardContext["phone"] = UserInfo.GetPhone();
    cardContext["address"] = ToAddressString(CheckoutState.Delivery().Address());
    cardContext["delivery_type"] = CheckoutState.Delivery().Type();
    cardContext["delivery_interval"] = *CheckoutState.DeliveryOption().Dates().GetRawValue();
    cardContext["offer_title"] = CheckoutState.Cart().Offer().Titles().Raw();
    ui64 deliveryPrice = CheckoutState.DeliveryOption().Price();
    ui64 offerPrice = FromString<ui64>(CheckoutState.Cart().Offer().Prices().Value());
    cardContext["delivery_price"] = deliveryPrice;
    cardContext["offer_price"] = offerPrice;
    cardContext["total_price"] = deliveryPrice + offerPrice * CheckoutState.Cart().ItemsNumber();
    cardContext["currency"] = CheckoutState.Cart().Offer().Prices().Currency();
    cardContext["items_number"] = CheckoutState.Cart().ItemsNumber();
    auto picture = TPicture::GetMostSuitablePicture(*CheckoutState.Cart().Offer().GetRawValue());
    NBassApi::TPicture<TBoolSchemeTraits> pictureScheme(&cardContext["offer_picture"]);
    SerializePicture(picture, pictureScheme);

    SetState(ERecurringPurchaseState::CheckoutConfirmOrder);
    if (IsScreenless) {
        Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__order_details_and_confirm"), cardContext);
    } else {
        Ctx.AddDivCardBlock(TStringBuf("recurring_purchase__order_details"), cardContext);
        Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__checkout_confirm"));
    }
    AddConfirmCheckoutSuggests(Ctx);
    return TResultValue();
}

TResultValue TMarketRecurringPurchase::HandleConfirmOrder()
{
    AddCancelSuggest();
    if (Form != ERecurringPurchaseForm::CheckoutYesOrNo) {
        if (!CheckExitDueAttemptsLimit()) {
            SetState(ERecurringPurchaseState::CheckoutConfirmOrder);
            Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__invalid_confirm"));
            AddConfirmCheckoutSuggests(Ctx);
        }
        return TResultValue();
    }

    auto confirmation = GetStringSlotValue(Ctx, TStringBuf("confirmation"));
    if (!confirmation) {
        ythrow TMarketException(TStringBuf("slot 'confirmation' is empty"));
    }

    if (*confirmation == TStringBuf("yes")) {
        Checkout();
    } else if (*confirmation == TStringBuf("no")) {
        if (IsScreenless) {
            return RenderDoNotCheckout();
        }
        ManualCheckout();
    } else {
        SetState(ERecurringPurchaseState::CheckoutConfirmOrder);
        AddConfirmCheckoutSuggests(Ctx);
        Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__invalid_confirm"));
    }
    return TResultValue();
}

TResultValue TMarketRecurringPurchase::Checkout()
{
    if (!TFakeUsers::IsAllowedCheckout(CheckoutState, UserInfo)) {
        SetState(ERecurringPurchaseState::CheckoutComplete);
        NSc::TValue cardData;
        cardData["email"] = UserInfo.GetEmail();
        Ctx.AddTextCardBlock(TStringBuf("market_common__checkout_is_not_allowed_for_this_user"), cardData);
        return TResultValue();
    }
    const TString aliceId = TStringBuilder() << CheckoutState.Cart().Offer().Model().Id() << ":" << StartTime.MicroSeconds();

    const TString notes = TStringBuilder() << TStringBuf("Заказ через Алису - ") << aliceId;

    const auto& checkoutHttpResponse = TCheckouterClient(Ctx).Checkout(
        CheckoutState.Cart(),
        CheckoutState.Delivery(),
        CheckoutState.DeliveryOption(),
        UserInfo,
        notes);

    if (checkoutHttpResponse.IsTimeout()) {
        LOG(DEBUG) << "Checkout request is too long. Ask user to wait some more time.\n"
                   << "Checkout request message: " << checkoutHttpResponse.GetErrorText() << Endl;

        CheckoutState.Order().AliceId() = aliceId;
        CheckoutState.Order().CheckoutedAtTimestamp() = StartTime.MicroSeconds();
        CheckoutState.Order().Attempt() = 0;
        SetState(ERecurringPurchaseState::CheckoutWaiting);
        Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__checkout_wait"));
        AddCheckoutWaitSuggests(Ctx);
        return TResultValue();
    }
    const auto& checkoutResponse = checkoutHttpResponse.GetResponse();

    if (!checkoutResponse.IsCheckedOut() || checkoutResponse.GetOrders().size() != 1) {
        SetState(ERecurringPurchaseState::CheckoutComplete);
        Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__checkout_failed"));
        return TResultValue();
    }

    auto orderId = checkoutResponse.GetOrders()[0].GetId();
    HandleSuccessCheckout(orderId, UserInfo.IsGuest());
    return TResultValue();
}

TResultValue TMarketRecurringPurchase::ManualCheckout()
{
    Y_ENSURE(!IsScreenless);

    const auto& skuOffers = TReportClient(Ctx).GetSkuOffers(CheckoutState.Sku()).GetResponse();

    const auto& results = skuOffers->Search().Results();
    if (results.Empty() || !results[0].Offers().HasItems() || results[0].Offers().Items().Empty()) {
        ythrow TMarketException(TStringBuf("blue offers were not found"));
    }
    const auto& offer = results[0].Offers().Items(0);

    const TString url = TMarketUrlBuilder(Ctx).GetBeruCheckoutUrl(
        offer.WareId(),
        offer.FeeShow(),
        FromString(offer.Prices().Value().Get()));

    SetState(ERecurringPurchaseState::CheckoutComplete);
    Ctx.AddSuggest("recurring_purchase__manual_checkout", NSc::TValue(url));
    Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__manual_checkout"));
    return TResultValue();
}

bool TMarketRecurringPurchase::TryCheckOrderExists() {
    const auto& orders = TCheckouterClient(Ctx).GetOrdersByUid(UserInfo.GetUid(), 1 /* pageSize */, CheckoutState.Order().AliceId()).GetResponse();

    if (orders.HasOrders()) {
        Y_ASSERT(orders.GetOrders().size() == 1);
        HandleSuccessCheckout(orders.GetOrders()[0].GetId(), UserInfo.IsGuest());
        return true;
    }
    return false;
}

void TMarketRecurringPurchase::HandleSuccessCheckout(i64 orderId, bool guestMode)
{
    SetState(ERecurringPurchaseState::CheckoutComplete);
    if (!guestMode) {
        Ctx.AddSuggest("recurring_purchase__checkout_complete", NSc::TValue(TMarketUrlBuilder(Ctx).GetBeruOrderUrl(orderId)));
    }
    NSc::TValue checkoutCompleteData;
    checkoutCompleteData["order_id"] = orderId;
    Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__checkout_complete"), checkoutCompleteData);
}

TResultValue TMarketRecurringPurchase::HandleCheckoutWaiting()
{
    if (TryCheckOrderExists()) {
        return TResultValue();
    }

    auto waitUpperBound = TInstant::FromValue(CheckoutState.Order().CheckoutedAtTimestamp()) + Ctx.GetConfig().Market().MaxCheckoutWaitDuration();

    if (StartTime > waitUpperBound) {
        SetState(ERecurringPurchaseState::CheckoutComplete);
        Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__checkout_failed"));
        return TResultValue();
    }

    CheckoutState.Order().Attempt() += 1;
    if (CheckoutState.Order().Attempt() >= MAX_CHECKOUT_ATTEMPTS) {
        SetState(ERecurringPurchaseState::CheckoutComplete);
        Ctx.AddTextCardBlock("recurring_purchase__checkout_failed");
        return TResultValue();
    }
    Ctx.AddTextCardBlock("recurring_purchase__checkout_wait");
    AddCheckoutWaitSuggests(Ctx);
    return TResultValue();
}

void TMarketRecurringPurchase::ProcessSkuOfferDetails()
{
    SetState(IsScreenless ? ERecurringPurchaseState::ProductDetailsScreenless : ERecurringPurchaseState::ProductDetails);

    const ui64 sku = Ctx.GetBeruOrderSku();
    const auto& skuOffers = TReportClient(Ctx).GetSkuOffers(sku).GetResponse();
    const auto& results = skuOffers->Search().Results();
    if (results.Empty() || !results[0].Offers().HasItems() || results[0].Offers().Items().Empty()) {
        Ctx.RenderChoiceBeruOfferOutdated();
        return;
    }
    const auto& rawOffer = results[0].Offers().Items(0);
    TOffer offer = TReportResponse::TResult(*rawOffer.GetRawValue()).GetOffer();
    if (HasIllegalWarnings(offer.GetWarnings())) {
        Ctx.RenderChoiceBeruOfferOutdated();
        return;
    }
    if (IsScreenless) {
        Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__sku_offer"), GetBlueOfferDetails(offer, rawOffer));
    } else {
        Ctx.RenderChoiceBeruProductDetailsCard(GetBlueOfferDetails(offer, rawOffer));
    }
    Ctx.AddBeruProductDetailsCardSuggest();
}

TResultValue TMarketRecurringPurchase::HandleSkuOfferDetails()
{
    Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__product_details"));
    ProcessSkuOfferDetails();
    AddCancelSuggest();
    return TResultValue();
}

NSc::TValue TMarketRecurringPurchase::GetModelDetails(
    const TModel& model,
    const NSc::TValue& rawModel,
    const TCgiGlFilters& glFilters,
    const TReportSearchResponse::TSchemeConst& blueDefaultOffer)
{
    NSc::TValue details;
    details["type"] = TStringBuf("model");
    details["model_id"] = model.GetId();
    details["prices"] = SerializeModelPrices(model);
    details["prices"]["currency"] = rawModel["prices"]["currency"].GetString();
    details["title"] = model.GetTitle();
    NBassApi::TPicture<TBoolSchemeTraits> pictureScheme(&details["picture"]);
    SerializePicture(TPicture::GetMostSuitablePicture(rawModel, glFilters), pictureScheme);
    details["market_url"] = TMarketUrlBuilder(Ctx).GetMarketModelUrl(
        model.GetId(),
        model.GetSlug(),
        Ctx.UserRegion(),
        Ctx.GetProductGalleryNumber(),
        Ctx.GetGalleryPosition(),
        glFilters
    );
    details["specs"] = rawModel["specs"]["friendly"];
    details["filters"] = GetFiltersForDetails(rawModel["filters"]);
    details["rating"] = rawModel["rating"];
    details["rating_icons"] = GetRatingIcons();
    AddWarningsToDetails(model.GetWarnings(), details);

    if (!blueDefaultOffer.Search().Results().Empty()) {
        const auto& blueOffer = blueDefaultOffer.Search().Results(0);
        FillBlueOfferFields(blueOffer, details);
    }
    return details;
}

NSc::TValue TMarketRecurringPurchase::GetBlueOfferDetails(const TOffer& offer, const TRawOffer& rawOffer)
{
    TMarketUrlBuilder urlBuilder(Ctx);

    NSc::TValue details;
    details["type"] = TStringBuf("offer");
    details["prices"] = SerializeBlueOfferPrices(rawOffer);
    const auto price = rawOffer.Prices();
    details["prices"]["currency"] = price.Currency();

    details["title"] = rawOffer.Titles().Raw().Get();
    NBassApi::TPicture<TBoolSchemeTraits> pictureScheme(&details["picture"]);
    SerializePicture(TPicture::GetMostSuitablePicture(*rawOffer.GetRawValue()), pictureScheme);
    details["filters"] = GetFiltersForDetails((*rawOffer.GetRawValue())["filters"]);
    AddWarningsToDetails(offer.GetWarnings(), details);
    details["urls"]["terms_of_use"] = urlBuilder.GetBeruTermsOfUseUrl();
    details["urls"]["model"] = urlBuilder.GetBeruModelUrl(
        rawOffer.MarketSku(),
        rawOffer.Slug(),
        Ctx.UserRegion(),
        Ctx.GetProductGalleryNumber(),
        Ctx.GetGalleryPosition()
    );
    details["urls"]["supplier"] = urlBuilder.GetBeruSupplierUrl(rawOffer.WareId());
    const auto sku = FromString<ui64>(rawOffer.MarketSku().Get());
    details["action"]["form_update"] = Ctx.GetCheckoutFormUpdate(sku).Value();
    Ctx.SetBeruOrderSku(sku);
    NBassApi::TOutputDelivery<TBoolSchemeTraits> delivery(&details["delivery"]);
    TDeliveryBuilder::FillBlueDelivery(rawOffer, delivery);

    return details;
}

void TMarketRecurringPurchase::FillBlueOfferFields(const TRawOffer& offer, NSc::TValue& details)
{
    const auto price = offer.Prices();
    NSc::TValue& beruPrice = details["beru"]["prices"];
    beruPrice["value"] = price.Value();
    beruPrice["currency"] = price.Currency();

    const auto sku = FromString<ui64>(offer.MarketSku().Get());
    details["beru"]["action"]["form_update"] = Ctx.GetCheckoutFormUpdate(sku).Value();

    Ctx.SetBeruOrderSku(sku);
}

TBeruOrderCardData TMarketRecurringPurchase::GetBeruOrderData(ui64 sku, const TRawOffer& blueOffer)
{
    TMarketUrlBuilder urlBuilder(Ctx);

    TBeruOrderCardData data;
    data->Title() = blueOffer.Titles().Raw();
    data->Prices().Value() = blueOffer.Prices().Value();
    data->Prices().Currency() = blueOffer.Prices().Currency();
    SerializePicture(TPicture::GetMostSuitablePicture(*blueOffer.GetRawValue()), data->Picture());
    data->Urls().TermsOfUse() = urlBuilder.GetBeruTermsOfUseUrl();
    data->Urls().Model() = urlBuilder.GetBeruModelUrl(
        blueOffer.MarketSku(),
        blueOffer.Slug(),
        Ctx.UserRegion(),
        Ctx.GetProductGalleryNumber(),
        Ctx.GetGalleryPosition()
    );
    data->Urls().Supplier() = urlBuilder.GetBeruSupplierUrl(blueOffer.WareId());
    const auto formUpdate = Ctx.GetCheckoutFormUpdate(sku);
    data->Action().FormUpdate() = formUpdate.Scheme();

    TDeliveryBuilder::FillBlueDelivery(blueOffer, data->Delivery());
    return data;
}

TResultValue TMarketRecurringPurchase::HandleModelDetails(
    TModelId id,
    const TCgiGlFilters& glFilters,
    const TRedirectCgiParams& redirectParams)
{
    auto defaultOfferHandle = TReportClient(Ctx).GetDefaultOfferAsync(id, glFilters, EMarketType::BLUE);

    const auto& response = TMarketClient(Ctx).MakeSearchModelRequest(id, glFilters, redirectParams, true /* withSpecs */);
    if (response.HasError()) {
        return response.GetError();
    }
    const auto& defaultOfferResponse = defaultOfferHandle.Wait().GetResponse();

    const auto& results = response.GetResults();
    if (results.empty()) {
        Ctx.RenderChoiceProductOutdated();
        return TResultValue();
    }

    auto result = results[0];
    auto model = result.GetModel();
    if (HasIllegalWarnings(model.GetWarnings())) {
        Ctx.RenderChoiceProductOutdated();
        return TResultValue();
    }
    Ctx.RenderChoiceProductDetailsCard(GetModelDetails(
        model,
        result.GetRawData(),
        glFilters,
        defaultOfferResponse.Scheme()));
    Ctx.AddProductDetailsCardSuggests(!defaultOfferResponse->Search().Results().Empty(), false);
    return TResultValue();
}

TResultValue TMarketRecurringPurchase::HandleParametricRedirection(const TReportResponse::TParametricRedirect& redirect)
{
    FillCtxFromParametricRedirect(redirect);
    LOG(DEBUG) << "Category confirmed. Ask filters" << Endl;
    return HandleFilterSelection();
}

TResultValue TMarketRecurringPurchase::HandleFilterSelection()
{
    Ctx.SetCurrentResult(GetResultUrl());

    const TReportResponse response = MakeFilterRequestWithRegionHandling();
    if (response.HasError()) {
        return response.GetError();
    }

    FilterWorker.UpdateFiltersDescription(response.GetFilters());

    if (response.GetTotal() == 0) {
        return RenderEmptyResult();
    }
    return RenderGalleryAndAskContinue(response);
}

bool TMarketRecurringPurchase::TryUpdateFormalizedNumberFilter(const TFormalizedGlFilters& filters)
{
    if (Form != ERecurringPurchaseForm::NumberFilter) {
        return false;
    }
    const auto& formalizedFilters = filters->Filters();
    if (formalizedFilters.Size() != 1) {
        return false;
    }
    const auto& kv = formalizedFilters.begin();
    if (kv.Value()->Type() != TStringBuf("number")) {
        return false;
    }
    NumberFilterWorker.UpdateFilter(ToString(kv.Key()));
    return true;
}

void TMarketRecurringPurchase::TryHandlePriceRequest()
{
    if (IsPriceRequest() || Form != ERecurringPurchaseForm::NumberFilter && Ctx.DoesAnyAmountExist()) {
        NSc::TValue price = Ctx.GetPrice();
        NSc::TValue amountFrom = price["from"];
        NSc::TValue amountTo = price["to"];
        NumberFilterWorker.GetAmountInterval(amountFrom, amountTo, true /* needRange */);
        Ctx.SetPrice(amountFrom, amountTo);
    }
}

bool TMarketRecurringPurchase::IsPriceRequest() const
{
    // Запрос ценовой, если выполняются условия:
    // 1) запрос числовой
    // 2) единицы измерения или "рубли", или не указаны
    // 3) параметр или "цена", или не указан
    return Form == ERecurringPurchaseForm::NumberFilter
        && Ctx.DoesAnyAmountExist()
        && EqualToOneOf(Ctx.GetUnit(), TStringBuf("rur"), TStringBuf(""))
        && EqualToOneOf(Ctx.GetParameter(), TStringBuf("price"), TStringBuf(""));
}

TResultValue TMarketRecurringPurchase::HandleNoneRedirection(const TReportResponse& response)
{
    if (response.GetTotal() == 0) {
        return RenderEmptyResult();
    }
    return RenderGalleryAndAskContinue(response);
}

TResultValue TMarketRecurringPurchase::RenderEmptyDeliveryOptions()
{
    Ctx.AddTextCardBlock(TStringBuf("recurring_purchase__empty_delivery_options"));
    SetState(ERecurringPurchaseState::CheckoutComplete);
    DeleteCancelSuggest();
    return TResultValue();
}

TResultValue TMarketRecurringPurchase::RenderEmptyResult()
{
    if (IsFirstRequest) {
        SetState(ERecurringPurchaseState::Exit);
        Ctx.AddTextCardBlock(IsScreenless
            ? TStringBuf("recurring_purchase__no_such_goods_screenless")
            : TStringBuf("recurring_purchase__no_such_goods"));
        DeleteCancelSuggest();
    } else {
        Ctx.AddTextCardBlock("market__empty_result");
        Ctx.SetQueryInfo(InitialQueryInfo);
    }
    return TResultValue();
}

TResultValue TMarketRecurringPurchase::RenderHasNoOrders()
{
    SetState(ERecurringPurchaseState::Exit);
    Ctx.AddTextCardBlock("recurring_purchase__has_no_orders");
    DeleteCancelSuggest();
    return TResultValue();
}

TResultValue TMarketRecurringPurchase::RenderDoNotCheckout()
{
    SetState(ERecurringPurchaseState::Exit);
    Ctx.AddTextCardBlock("recurring_purchase__do_not_checkout");
    DeleteCancelSuggest();
    return TResultValue();
}

bool TMarketRecurringPurchase::FillResultFromOrders(const TVector<TReportResponse::TResult>& results, const TVector<TSkuOrderItem>& skus, TMaybe<ui64>& singleSku)
{
    auto& documents = Ctx.CreateResultModels();

    THashMap<ui64, NSc::TValue> docBySku;
    for (size_t i = 0; i < results.size() && documents.ArraySize() < MAX_MARKET_RESULTS; i++) {
        const auto& result = results[i];
        if (HasIllegalWarnings(result.GetWarnings())) {
            continue;
        }
        if (result.GetType() != TReportResponse::TResult::EType::MODEL) {
            continue;
        }
        const auto& offers = result.GetModelOffers();
        for (const auto& offer : offers) {
            const ui64 sku = offer.GetSku().GetRef();
            const NSc::TValue doc = GetResultDoc(result, documents.ArraySize() + 1);
            if (doc.IsNull()) {
                continue;
            }
            const ui64 price = doc["prices"]["value"];
            auto [it, hasThisSku] = docBySku.emplace(sku, doc);
            if (!hasThisSku) {
                const ui64 oldPrice = it->second["prices"]["value"];
                if (oldPrice > price) {
                    it->second = doc;
                }
            }
        }
    }

    THashSet<ui64> pocessedSkus;
    for (const auto& item : skus) {
        if (pocessedSkus.contains(item.Sku)) {
            continue;
        }
        const auto it = docBySku.find(item.Sku);
        if (it != docBySku.end()) {
            pocessedSkus.emplace(item.Sku);
            documents.Push(it->second);
        }
    }
    if (pocessedSkus.size() == 1) {
        singleSku = *pocessedSkus.begin();
    }
    return !documents.ArrayEmpty();
}

TResultValue TMarketRecurringPurchase::RenderGallery(const TReportResponse& response, const TVector<TSkuOrderItem>& skus)
{
    Ctx.ClearResult();
    TMaybe<ui64> singleSku;
    if (FillResultFromOrders(response.GetResults(), skus, singleSku)) {
        if (IsScreenless) {
            if (!singleSku.Empty()) {
                Ctx.SetBeruOrderSku(singleSku.GetRef());
                ProcessSkuOfferDetails();
                return TResultValue();
            }
            Ctx.AddTextCardBlock("recurring_purchase__select_product");
            return TResultValue();
        } else {
            NSc::TValue data;
            data["utterance"] = Ctx.Utterance();
            Ctx.AddTextCardBlock("recurring_purchase__ask_continue", data);
            if (!singleSku.Empty()) {
                Ctx.SetBeruOrderSku(singleSku.GetRef());
                ProcessSkuOfferDetails();
                return TResultValue();
            }
            Ctx.AddDivCardBlock("recurring_purchase_models", NSc::TValue());
            Ctx.Ctx().AddStopListeningBlock();
        }
    } else {
        if (IsScreenless) {
            return RenderEmptyResult();
        }
        return HandleChoice();
    }
    return TResultValue();
}


TResultValue TMarketRecurringPurchase::RenderGalleryAndAskContinue(const TReportResponse& response)
{
    Ctx.ClearResult();
    Ctx.AddTotal(response.GetTotal());
    Ctx.SetFirstRequest(false);
    if (FillResult(response.GetResults())) {
        Ctx.RenderChoiceGallery();
        Ctx.AddTextCardBlock("market__ask_continue");
    } else {
        return RenderEmptyResult();
    }
    return TResultValue();
}

NSc::TValue TMarketRecurringPurchase::GetRatingIcons() const
{
    static TStringBuf iconNames[] = {
        TStringBuf("Fill"),
        TStringBuf("Half"),
        TStringBuf("None")};

    NSc::TValue result;
    for (const auto iconName : iconNames) {
        const auto iconUrl = Ctx.GetAvatarPictureUrl(TStringBuf("poi"), iconName);
        if (iconUrl) {
            result[iconName] = *iconUrl;
        }
    }

    return result;
}

void TMarketRecurringPurchase::SetState(ERecurringPurchaseState state)
{
    if (state != State) {
        Ctx.SetAttempt(0);
    }
    State = state;
    Ctx.SetState(State);
}

void TMarketRecurringPurchase::AddCancelSuggest()
{
    if (State != ERecurringPurchaseState::CheckoutComplete
        && State != ERecurringPurchaseState::Exit)
    {
        Ctx.AddSuggest(TStringBuf("market__cancel"));
    }
}

void TMarketRecurringPurchase::DeleteCancelSuggest()
{
    Ctx.DeleteSuggest(TStringBuf("market__cancel"));
}

} // namespace NMarket

} // namespace NBASS

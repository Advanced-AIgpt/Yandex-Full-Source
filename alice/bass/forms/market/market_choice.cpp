#include "market_choice.h"

#include "dynamic_data.h"
#include "delivery_builder.h"
#include "client/report_client.h"
#include "util/report.h"
#include "util/serialize.h"
#include "util/suggests.h"
#include "forms.h"
#include "market_exception.h"
#include "market_url_builder.h"
#include "clear_request.h"
#include "product_offers_card.h"

#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <library/cpp/resource/resource.h>

namespace NBASS {

namespace NMarket {

namespace {

const ui32 MAX_STEP_ATTEMPTS_COUNT = 4;

void AddWarningsToDetails(const TVector<TWarning>& warnings, NSc::TValue& details)
{
    if (!warnings.empty()) {
        TWarningsScheme warningsScheme(&details["warnings"]);
        SerializeWarnings(warnings, warningsScheme);
    }
}

IParallelHandler::TTryResult ToTryResult(TResultValue res)
{
    if (res.Defined()) {
        return res.GetRef();
    }
    return IParallelHandler::ETryResult::Success;
}

TResultValue FromTryResult(IParallelHandler::TTryResult res)
{
    if (auto tryError = std::get_if<TError>(&res)) {
        return *tryError;
    }
    return TResultValue();
}

}

TMarketChoice::TMarketChoice(TMarketContext& ctx, bool isParallelMode)
    : TBaseMarketChoice(ctx)
    , Form(FromString<EChoiceForm>(Ctx.FormName()))
    , State(Ctx.GetState(EChoiceState::Activation))
    , GeoSupport(Ctx.Ctx().GlobalCtx())
    , NumberFilterWorker(Ctx)
    , InitialQueryInfo(Ctx.GetTextRedirect(), Ctx.GetPrice(), Ctx.GetCgiGlFilters())
    , IsParallelMode(isParallelMode)
{
}

IParallelHandler::TTryResult TMarketChoice::TryDo()
{
    if (!CheckMarketChoiceSupportedForGeoId()) {
        if (!IsParallelMode) {
            Ctx.RenderEmptySerp();
            Ctx.AddSearchSuggest();
        }
        return IParallelHandler::ETryResult::NonSuitable;
    }

    if (IsParallelMode) {
        if (Form == EChoiceForm::Native) {
            return TryHandleNative();
        } else if (Form == EChoiceForm::NativeBeru) {
            return TryHandleNativeBeru();
        }
    }

    return ToTryResult(Do());
}

TResultValue TMarketChoice::DoImpl()
{
    LOG(DEBUG) << "Form " << Form << " state " << State << Endl;

    switch (Form) {
        case EChoiceForm::Native:
            return HandleNative();
        case EChoiceForm::NativeBeru:
            return HandleNativeBeru();
        case EChoiceForm::Activation:
            return HandleActivation(Ctx.GetChoiceMarketType(EMarketType::GREEN));
        case EChoiceForm::BeruActivation:
            return HandleActivation(EMarketType::BLUE);
        case EChoiceForm::Cancel:
            SetState(EChoiceState::Exit);
            Ctx.RenderCancel();
            return TResultValue();
        case EChoiceForm::Garbage:
            return HandleGarbage();
        case EChoiceForm::Repeat:
            return TResultValue();
        case EChoiceForm::ShowMore:
            return TResultValue();
        case EChoiceForm::StartChoiceAgain:
            return HandleStartChoiceAgain();
        case EChoiceForm::MarketChoice:
        case EChoiceForm::MarketChoiceEllipsis:
        case EChoiceForm::MarketParams:
            return HandleMarketChoice();
        case EChoiceForm::MarketNumberFilter:
            return HandleNumberFilter();
        case EChoiceForm::ProductDetails:
            return HandleProductDetailsDemand();
        case EChoiceForm::ProductDetailsExternal:
            return HandleProductDetailsExternal();
        case EChoiceForm::BeruOrder:
            return HandleBeruOrder();
        case EChoiceForm::AddToCart:
            return HandleAddToCart();
        case EChoiceForm::GoToShop:
            return HandleGoToShop();
        case EChoiceForm::Checkout:
        case EChoiceForm::CheckoutItemsNumber:
        case EChoiceForm::CheckoutEmail:
        case EChoiceForm::CheckoutPhone:
        case EChoiceForm::CheckoutAddress:
        case EChoiceForm::CheckoutIndex:
        case EChoiceForm::CheckoutDeliveryIntervals:
        case EChoiceForm::CheckoutYesOrNo:
        case EChoiceForm::CheckoutEverything:
        case EChoiceForm::ProductDetailsExternal_BeruOrder:
            return HandleUnexpectedBehaviour();
    }
}

TResultValue TMarketChoice::HandleUnexpectedBehaviour()
{
    TString errMsg = TStringBuilder() << TStringBuf("Unexpected behaviour in TMarketChoice: state - ") << State
                                      << TStringBuf(", form - ") << Form;
    LOG(ERR) << errMsg << Endl;
    ythrow TMarketException(errMsg);
}

TResultValue TMarketChoice::HandleProductOutdated()
{
    SetState(Ctx.IsOpen() ? EChoiceState::ChoiceOpen : EChoiceState::Choice);
    Ctx.RenderChoiceProductOutdated();
    return TResultValue();
}

TResultValue TMarketChoice::HandleResponseError(const TReportResponse& response)
{
    SetState(Ctx.IsOpen() ? EChoiceState::ChoiceOpen : EChoiceState::Choice);
    return response.GetError();
}

void TMarketChoice::CustomiseCtx()
{
    Ctx.SetNative(true);

    if (Ctx.GetChoiceMarketType() == EMarketType::GREEN && Ctx.GetExperiments().MarketNativeOpen()) {
        Ctx.SetOpen(true);
        SetState(EChoiceState::ChoiceOpen);
    } else if (Ctx.GetChoiceMarketType() == EMarketType::BLUE && Ctx.GetExperiments().MarketNativeBlueOpen()) {
        Ctx.SetOpen(true);
        SetState(EChoiceState::ChoiceOpen);
    } else {
        SetState(EChoiceState::Choice);
    }

    Ctx.AddSearchSuggest();
    AppendTextRedirect(Ctx.Request());
}

IParallelHandler::TTryResult TMarketChoice::TryHandleNativeAny()
{
    CustomiseCtx();
    if (Ctx.GetChoiceMarketType() == EMarketType::GREEN
            && !GeoSupport.IsMarketNativeSupportedForGeoId(Ctx.UserRegion(), Ctx.GetScenarioType()))
    {
        return IParallelHandler::ETryResult::NonSuitable;
    }
    if (TDynamicDataFacade::ContainsVulgarQuery(Ctx.Request())) {
        return IParallelHandler::ETryResult::NonSuitable;
    }
    if (!Ctx.GetExperiments().MarketBeru() && Ctx.GetChoiceMarketType() == EMarketType::BLUE) {
        return IParallelHandler::ETryResult::NonSuitable;
    }
    if (!IsPriceRequest()) {
        AppendTextRedirect(Ctx.Request());
    }

    TString originalText = ToString(Ctx.GetTextRedirect());
    try {
        auto result = HandleSearch();
        Ctx.SetTextRedirect(originalText);
        return result;
    } catch (...) {
        Ctx.SetTextRedirect(originalText);
        throw;
    }
}

TResultValue TMarketChoice::HandleNativeAny()
{
    CustomiseCtx();
    return HandleMarketChoice();
}

IParallelHandler::TTryResult TMarketChoice::TryHandleNative()
{
    if (!Ctx.GetSlot("bad_shop").StringEmpty() || !Ctx.GetSlot("is_used").StringEmpty()) {
        return IParallelHandler::ETryResult::NonSuitable;
    }

    bool isOnMarket = Ctx.GetChoiceMarketType() == EMarketType::GREEN && Ctx.HasChoiceMarketType() && Ctx.GetExperiments().MarketNativeOnMarket();
    const TStringBuf actionString = Ctx.GetStringSlot("action");
    bool hasNotActionFlag = (actionString.empty()
                             || (actionString == TStringBuf("where_buy") && !Ctx.GetExperiments().MarketNativeAllowWhereBuy())
                             || (actionString == TStringBuf("choose") && !Ctx.GetExperiments().MarketNativeAllowChoose())
                             || (actionString == TStringBuf("where_search") && !Ctx.GetExperiments().MarketNativeAllowWhereSearch())
                             || (actionString == TStringBuf("search") && !Ctx.GetExperiments().MarketNativeAllowSearch())
                             || (actionString == TStringBuf("wish") && !Ctx.GetExperiments().MarketNativeAllowWish())
                             || (actionString == TStringBuf("choose_to_buy") && Ctx.GetExperiments().MarketNativeDenyChooseToBuy())
                            );

    if (Ctx.GetChoiceMarketType() == EMarketType::GREEN
        && hasNotActionFlag
        && !isOnMarket
       )
    {
        LOG(DEBUG) << "Slot 'action' is '" << actionString << "'. Fallback to search" << Endl;
        return IParallelHandler::ETryResult::NonSuitable;
    }

    Ctx.SetResponseFormAndCopySlots(ToString(EChoiceForm::MarketChoice), {TStringBuf("request"), TStringBuf("amount"),
        TStringBuf("amount_to"), TStringBuf("amount_from"), TStringBuf("amount_need_range"), TStringBuf("unit"),
        TStringBuf("choice_market_type")});
    TryHandlePriceRequest();
    return TryHandleNativeAny();
}

IParallelHandler::TTryResult TMarketChoice::TryHandleNativeBeru()
{
    auto marketType = Ctx.GetChoiceMarketType();
    if (marketType == EMarketType::BLUE
        || (Ctx.GetExperiments().MarketNativeOnMarket() && marketType == EMarketType::GREEN && Ctx.HasChoiceMarketType())
       )
    {
        Ctx.SetResponseFormAndCopySlots(ToString(EChoiceForm::MarketChoice), {TStringBuf("request"), TStringBuf("choice_market_type")});
        return TryHandleNativeAny();
    } else {
        return IParallelHandler::ETryResult::NonSuitable;
    }
}

TResultValue TMarketChoice::HandleNative()
{
    Ctx.SetResponseFormAndCopySlots(ToString(EChoiceForm::MarketChoice), {TStringBuf("request"), TStringBuf("amount"),
        TStringBuf("amount_to"), TStringBuf("amount_from"), TStringBuf("amount_need_range"), TStringBuf("unit"),
        TStringBuf("choice_market_type")});

    return HandleNativeAny();
}

TResultValue TMarketChoice::HandleNativeBeru()
{
    auto marketType = Ctx.GetChoiceMarketType();
    if (marketType == EMarketType::BLUE
        || (Ctx.GetExperiments().MarketNativeOnMarket() && marketType == EMarketType::GREEN && Ctx.HasChoiceMarketType())
       )
    {
        Ctx.SetResponseFormAndCopySlots(ToString(EChoiceForm::MarketChoice), {TStringBuf("request"), TStringBuf("choice_market_type")});

        return HandleNativeAny();
    } else {
        return Ctx.RunSearchResponse();
    }
}

TResultValue TMarketChoice::HandleActivation(EMarketType marketType)
{
    if (marketType == EMarketType::BLUE) {
        Ctx.SetResponseForm(ToString(EChoiceForm::Activation));
        Ctx.SetChoiceMarketType(marketType);
    } else {
        Ctx.SetChoiceMarketType(marketType);
    }
    Ctx.RenderActivationPhrase(TDynamicDataFacade::IsFreeDeliveryDate(Now()));
    SetState(Ctx.IsOpen() ? EChoiceState::ChoiceOpen : EChoiceState::Choice);
    return TResultValue();
}

TResultValue TMarketChoice::HandleMarketChoice()
{
    SetState(Ctx.IsOpen() ? EChoiceState::ChoiceOpen : EChoiceState::Choice);
    if (TDynamicDataFacade::ContainsVulgarQuery(Ctx.Request())) {
        return FromTryResult(RenderEmptyResult());
    }
    if (Form == EChoiceForm::MarketChoice) {
        Ctx.ClearAllSlots();
    }
    TryHandlePriceRequest();

    Ctx.ClearPreviousSearchSlots();
    Ctx.ClearFilterExamples();

    if (!IsPriceRequest()) {
        AppendTextRedirect(Ctx.Request());
    }
    TString originalText = ToString(Ctx.GetTextRedirect());
    try {
        auto result = FromTryResult(HandleSearch());
        Ctx.SetTextRedirect(originalText);
        return result;
    } catch (...) {
        Ctx.SetTextRedirect(originalText);
        throw;
    }
}

IParallelHandler::TTryResult TMarketChoice::HandleSearch()
{
    TStringBuf query = Ctx.GetTextRedirect();
    if (TUtf32String::FromUtf8(query).size() <= 1) {
        LOG(DEBUG) << "Request \"" << query << "\" is too short" << Endl;
        Ctx.RenderChoiceEmptyResult();
        return IsParallelMode ? IParallelHandler::ETryResult::NonSuitable : IParallelHandler::ETryResult::Success;
    }

    const TMaybe<TReportResponse> response = MakeSearchRequestWithRedirects();
    if (response.Empty() || response->HasError()) {
        return IsParallelMode || response.Empty()
               ? IParallelHandler::TTryResult(IParallelHandler::ETryResult::NonSuitable)
               : IParallelHandler::TTryResult(response->GetError());
    }

    Y_ASSERT(response->GetRedirectType() == TReportResponse::ERedirectType::NONE);
    FilterWorker.UpdateFiltersDescription(response->GetFilters());

    if (response->GetTotal() == 0) {
        switch (Ctx.GetChoiceMarketType()) {
            case EMarketType::GREEN: {
                return RenderEmptyResult();
            }
            case EMarketType::BLUE: {
                // green and blue nids are different, so here we need to skip blue nid
                const auto& greenResponse = (Ctx.DoesCategoryExist())
                        ? MakeFilterRequestAsync(
                              false /* allowRedirects */,
                              EMarketType::GREEN,
                              TCategory(Ctx.GetCategory().GetHid()),
                              Ctx.GetFesh()
                          ).Wait()
                       : MakeSearchRequestAsync(
                              Ctx.GetTextRedirect(),
                              EMarketType::GREEN,
                              false /* allowRedirects */
                          ).Wait();

                if (greenResponse.HasError()) {
                    return IsParallelMode
                        ? IParallelHandler::TTryResult(IParallelHandler::ETryResult::NonSuitable)
                        : IParallelHandler::TTryResult(greenResponse.GetError());
                }
                return RenderGreenSuggestOrEmptyResult(greenResponse);
            }
        }
    }

    if (!Ctx.WereFilterExamplesShown()) {
        FilterWorker.SetFilterExamples(response->GetFilters());
        Ctx.SetFilterExamplesShown();
    }

    return RenderGalleryAndAskContinue(*response);
}

TResultValue TMarketChoice::FormalizeFilters()
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

TMaybe<TReportResponse> TMarketChoice::MakeSearchRequestWithRedirects()
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

        if (response.GetRedirect().HasCategory()
            && !CheckRedirectCategory(response.GetRedirect().GetCategory()))
        {
            return Nothing();
        }

        response.GetRedirect().FillCtx(Ctx);
    }
    return makeRequest(false /* allowRedirects */);
}

TResultValue TMarketChoice::HandleNumberFilter()
{
    if (!Ctx.DoesAnyAmountExist()) {
        LOG(WARNING) << "Classification error. number_filter intent without values. Handle as garbage intent" << Endl;
        return HandleGarbage();
    }
    return HandleMarketChoice();
}

TResultValue TMarketChoice::HandleProductDetailsDemand()
{
    const auto& product = Ctx.GetProduct();
    if (product->Type() == TStringBuf("sku")) {
        return HandleSkuOfferDetails(product->Id());
    } else if (product->Type() == TStringBuf("model")) {
        TCgiGlFilters glFilters;
        for (const auto kv : product->GlFilters()) {
            TVector<TString> values(Reserve(kv.Value().Size()));
            for (const auto& value : kv.Value()) {
                values.push_back(ToString(value));
            }
            glFilters[kv.Key()] = values;
        }
        auto hidMaybe = product->HasHid() ? MakeMaybe(product->Hid()) : Nothing();
        auto wareMaybe = product->HasWareId() ? MakeMaybe(product->WareId()) : Nothing();
        return TProductOffersCardHandler(Ctx).Do(product->Id(), wareMaybe, glFilters, product->Price(), hidMaybe);
    } else if (product->Type() == TStringBuf("offer")) {
        const auto& response = TMarketClient(Ctx).MakeSearchOfferRequest(product->WareId());
        if (response.HasError()) {
            return HandleResponseError(response);
        }
        const auto& results = response.GetResults();
        if (results.empty()) {
            return HandleProductOutdated();
        }

        auto result = results[0];
        auto offer = result.GetOffer();
        if (HasIllegalWarnings(offer.GetWarnings())) {
            return HandleProductOutdated();
        }

        SetState(Ctx.IsOpen() ? EChoiceState::ProductDetailsOpen : EChoiceState::ProductDetails);
        auto details = GetOfferDetails(offer, result.GetRawData());
        Ctx.RenderChoiceProductDetailsCard(details);
        Ctx.AddProductDetailsCardSuggests(false, details.Has(TStringBuf("shop_url")));
        return TResultValue();
    }

    SetState(Ctx.IsOpen() ? EChoiceState::ChoiceOpen : EChoiceState::Choice);
    LOG(ERR) << "Got unexpected product type - " << product->Type() << Endl;
    Y_ASSERT(false);
    return TError(TError::EType::MARKETERROR, TStringBuf("unexpected_behaviour"));
}

TResultValue TMarketChoice::HandleProductDetailsExternal()
{
    SetState(EChoiceState::ProductDetailsExternal);
    bool need_greeting = true;
    if (!Ctx.HasMarketClid()) {
        Ctx.SetMarketClid(EClids::PRODUCT_DETAILS);
    }
    if (Ctx.GetMarketClid() == EClids::SEARCH_BY_PICTURE_IN_ALICE
        || Ctx.GetBoolSlot(TStringBuf("from_alice")))
    {
        need_greeting = false;
    }
    const auto& product = Ctx.GetProduct();
    if (product->Type() == TStringBuf("model")) {
        TCgiGlFilters glFilters;
        for (const auto kv : product->GlFilters()) {
            TVector<TString> values(Reserve(kv.Value().Size()));
            for (const auto& value : kv.Value()) {
                values.push_back(ToString(value));
            }
            glFilters[kv.Key()] = values;
        }
        auto hidMaybe = product->HasHid() ? MakeMaybe(product->Hid()) : Nothing();
        NSc::TValue data;
        data["need_greeting"] = need_greeting;
        return TProductOffersCardHandler(Ctx).Do(
            product->Id(),
            product->HasWareId() ? MakeMaybe(product->WareId()) : Nothing(),
            glFilters,
            product->Price(),
            hidMaybe,
            TRedirectCgiParams(),
            false /* increaseGalleryNumber */,
            TStringBuf("market__product_details_external"),
            data);
    } else if (product->Type() == TStringBuf("offer")) {
        const auto& response = TMarketClient(Ctx).MakeSearchOfferRequest(product->WareId());
        if (response.HasError()) {
            return HandleResponseError(response);
        }
        const auto& results = response.GetResults();
        if (results.empty()) {
            return HandleProductOutdated();
        }

        auto result = results[0];
        auto offer = result.GetOffer();
        if (HasIllegalWarnings(offer.GetWarnings())) {
            return HandleProductOutdated();
        }

        SetState(EChoiceState::ProductDetailsExternal);
        auto details = GetOfferDetails(offer, result.GetRawData());
        NSc::TValue data;
        data["need_greeting"] = need_greeting;
        Ctx.AddTextCardBlock(TStringBuf("market__product_details_external"), data);
        Ctx.AddDivCardBlock("market_product_details_external", details);
        Ctx.Ctx().AddStopListeningBlock();
        return TResultValue();
    }
    LOG(ERR) << "Got unexpected product type - " << product->Type() << Endl;
    Y_ASSERT(false);
    return TError(TError::EType::MARKETERROR, TStringBuf("unexpected_behaviour"));
}

TResultValue TMarketChoice::HandleGoToShop()
{
    if (!Ctx.HasProduct()) {
        return HandleGarbage();
    }
    const auto& product = Ctx.GetProduct();
    const TStringBuf shopUrl = product->ShopUrl();
    if (shopUrl.empty()) {
        return HandleGarbage();
    }
    SetState(Ctx.IsOpen() ? EChoiceState::ChoiceOpen : EChoiceState::Choice);
    NSc::TValue data;
    data["uri"] = shopUrl;
    Ctx.Ctx().AddCommand<TMarketGoToShopDirective>(TStringBuf("open_uri"), data);
    Ctx.AddTextCardBlock(TStringBuf("market__go_to_shop"));
    return TResultValue();
}

TResultValue TMarketChoice::HandleStartChoiceAgain()
{
    bool isClosed = false;
    if (Ctx.IsOpen() && !Ctx.GetExperiments().MarketStartChoiceAgainOpen()) {
        Ctx.SetOpen(false);
        isClosed = true;
    }
    SetState(EChoiceState::Choice);
    Ctx.RenderChoiceStartChoiceAgain(isClosed);
    return TResultValue();
}

TResultValue TMarketChoice::HandleGarbage()
{
    if (CheckExitDueAttemptsLimit()) {
        return TResultValue();
    }
    SetState(Ctx.IsOpen() ? EChoiceState::ChoiceOpen : EChoiceState::Choice);
    Ctx.RenderChoiceGarbage();
    return TResultValue();
}

TResultValue TMarketChoice::HandleSkuOfferDetails(ui64 sku)
{
    const auto& skuOffers = TReportClient(Ctx).GetSkuOffers(sku).GetResponse();

    const auto& results = skuOffers->Search().Results();
    if (results.Empty() || results[0].Offers().Items().Empty()) {
        SetState(Ctx.IsOpen() ? EChoiceState::ChoiceOpen : EChoiceState::Choice);
        Ctx.RenderChoiceBeruOfferOutdated();
        return TResultValue();
    }

    const auto& rawOffer = results[0].Offers().Items(0);
    const auto& specs = results[0].Specs().Friendly();
    NSc::TValue specsTValue;
    auto& specsArr = specsTValue.SetArray();

    for (const auto& spec : specs) {
        specsArr.Push(spec);
    }

    TOffer offer = TReportResponse::TResult(*rawOffer.GetRawValue()).GetOffer();
    if (HasIllegalWarnings(offer.GetWarnings())) {
        SetState(Ctx.IsOpen() ? EChoiceState::ChoiceOpen : EChoiceState::Choice);
        Ctx.RenderChoiceBeruOfferOutdated();
        return TResultValue();
    }
    SetState(Ctx.IsOpen() ? EChoiceState::BeruProductDetailsOpen : EChoiceState::BeruProductDetails);
    Ctx.RenderChoiceBeruProductDetailsCard(GetBlueOfferDetails(offer, rawOffer, specsTValue));
    Ctx.AddBeruProductDetailsCardSuggest();

    return TResultValue();
}

NSc::TValue TMarketChoice::GetModelDetails(
    const TModel& model,
    const NSc::TValue& rawModel,
    const TCgiGlFilters& glFilters,
    const NBassApi::TReportDefaultOfferBlue<TBoolSchemeTraits>& blueDefaultOffer)
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

    FillShopOfferFields(model.GetDefaultOffer(), details);

    if (!blueDefaultOffer.Search().Results().Empty()) {
        const auto& blueOffer = blueDefaultOffer.Search().Results(0);
        FillBlueOfferFields(blueOffer, details);
    }
    return details;
}

NSc::TValue TMarketChoice::GetOfferDetails(const TOffer& offer, const NSc::TValue& rawOffer)
{
    NSc::TValue details;
    details["type"] = TStringBuf("offer");
    details["prices"] = SerializeOfferPrices(offer);
    details["prices"]["currency"] = rawOffer["prices"]["currency"].GetString();
    details["title"] = offer.GetTitle();
    NBassApi::TPicture<TBoolSchemeTraits> pictureScheme(&details["picture"]);
    SerializePicture(offer.GetPicture(), pictureScheme);

    details["market_url"] = TMarketUrlBuilder(Ctx).GetMarketOfferUrl(
        offer.GetWareId(),
        offer.GetCpc(),
        Ctx.UserRegion(),
        Ctx.GetProductGalleryNumber(),
        Ctx.GetGalleryPosition()
    );
    details["filters"] = GetFiltersForDetails(rawOffer["filters"]);
    FillShopOfferFields(offer, details);
    AddWarningsToDetails(offer.GetWarnings(), details);
    return details;
}

NSc::TValue TMarketChoice::GetBlueOfferDetails(
    const TOffer& offer,
    const NBassApi::TReportDocumentConst<TBoolSchemeTraits>& rawOffer,
    const NSc::TValue& specs)
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
    details["specs"] = specs;
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
    AddCartActionToDetails(sku, details);
    if (TDynamicDataFacade::IsFreeDeliveryDate(Now())) {
        details["free_delivery"] = true;
    }

    return details;
}

void TMarketChoice::AddCartActionToDetails(ui64 sku, NSc::TValue details)
{
    if (Ctx.GetExperiments().AddToCart()) {
        details["add_to_cart"]["form_update"] = Ctx.GetAddToCartFormUpdate(sku).Value();
        AddCartSuggest(Ctx, sku);
    }
}

void TMarketChoice::FillShopOfferFields(const TMaybe<TOffer>& offer, NSc::TValue& details)
{
    if (!offer) {
        return;
    }
    if (!Ctx.HasExpFlag("market_direct_shop")) {
        return;
    }
    if (!offer->GetShopUrl().empty()) {
        details["shop_url"] = offer->GetShopUrl();
        details["shop_name"] = offer->GetShop();
    }
}

void TMarketChoice::FillBlueOfferFields(
    const NBassApi::TReportDocumentConst<TBoolSchemeTraits>& offer,
    NSc::TValue& details)
{
    const auto price = offer.Prices();
    NSc::TValue& beruPrice = details["beru"]["prices"];
    beruPrice["value"] = price.Value();
    beruPrice["currency"] = price.Currency();

    const auto sku = FromString<ui64>(offer.MarketSku().Get());
    details["beru"]["action"]["form_update"] = Ctx.GetBeruOrderFormUpdate(
        sku,
        Ctx.GetProductGalleryNumber(),
        Ctx.GetGalleryPosition()
    ).Value();

    Ctx.SetBeruOrderSku(sku);
    AddCartActionToDetails(sku, details);
}

TResultValue TMarketChoice::HandleBeruOrder()
{
    // todo MALISA-635
    const TMaybe<ui64> sku = Ctx.GetBeruOrderSku();

    if (!sku) {
        SetState(Ctx.IsOpen() ? EChoiceState::ChoiceOpen : EChoiceState::Choice);
        Ctx.RenderChoiceBeruNoSku();
        return TResultValue();
    }

    auto skuOffers = TReportClient(Ctx).GetSkuOffers(*sku).GetResponse();
    const auto& results = skuOffers->Search().Results();
    if (results.Empty() || results[0].Offers().Items().Empty()) {
        SetState(Ctx.IsOpen() ? EChoiceState::ChoiceOpen : EChoiceState::Choice);
        Ctx.RenderChoiceBeruOfferOutdated();
    } else {
        SetState(Ctx.IsOpen() ? EChoiceState::MakeOrderOpen : EChoiceState::MakeOrder);
        const auto& offer = results[0].Offers().Items(0);
        Ctx.RenderChoiceBeruOrderCard(GetBeruOrderData(*sku, offer));
        Ctx.AddBeruOrderCardSuggests();
    }

    return TResultValue();
}

TResultValue TMarketChoice::HandleAddToCart()
{
    if (!Ctx.HasBeruOrderSku()) {
        SetState(EChoiceState::Choice);
        Ctx.RenderChoiceNoSkuForCart();
        return TResultValue();
    }

    const ui64 sku = Ctx.GetBeruOrderSku();
    const auto& skuOffers = TReportClient(Ctx).GetSkuOffers(sku).GetResponse();
    const auto& results = skuOffers->Search().Results();
    if (results.Empty() || results[0].Offers().Items().Empty()) {
        SetState(Ctx.IsOpen() ? EChoiceState::ChoiceOpen : EChoiceState::Choice);
        Ctx.RenderChoiceBeruOfferOutdated();
    } else {
        const auto& offer = results[0].Offers().Items(0);
        SetState(EChoiceState::CheckoutComplete);
        Ctx.RenderChoiceAddToCart();
        Ctx.OpenBundleUrl(offer.Urls().Cpa());
    }

    return TResultValue();
}

TBeruOrderCardData TMarketChoice::GetBeruOrderData(
    ui64 sku,
    const NBassApi::TReportDocumentConst<TBoolSchemeTraits>& blueOffer)
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
    data->FreeDelivery() = TDynamicDataFacade::IsFreeDeliveryDate(Now());
    TDeliveryBuilder::FillBlueDelivery(blueOffer, data->Delivery());
    return data;
}

IParallelHandler::TTryResult TMarketChoice::HandleParametricRedirection(const TReportResponse::TParametricRedirect& redirect)
{
    FillCtxFromParametricRedirect(redirect);
    LOG(DEBUG) << "Category confirmed. Ask filters" << Endl;
    return HandleFilterSelection();
}

bool TMarketChoice::TryUpdateFormalizedNumberFilter(const TFormalizedGlFilters& filters)
{
    if (Form != EChoiceForm::MarketNumberFilter) {
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

void TMarketChoice::TryHandlePriceRequest()
{
    if (IsPriceRequest() || Form != EChoiceForm::MarketNumberFilter && Ctx.DoesAnyAmountExist()) {
        NSc::TValue price = Ctx.GetPrice();
        NSc::TValue amountFrom = price["from"];
        NSc::TValue amountTo = price["to"];
        NumberFilterWorker.GetAmountInterval(amountFrom, amountTo, true /* needRange */);
        Ctx.SetPrice(amountFrom, amountTo);
    }
}

bool TMarketChoice::IsPriceRequest() const
{
    // Запрос ценовой, если выполняются условия:
    // 1) запрос числовой
    // 2) единицы измерения или "рубли", или не указаны
    // 3) параметр или "цена", или не указан
    return Form == EChoiceForm::MarketNumberFilter && Ctx.DoesAnyAmountExist() && EqualToOneOf(Ctx.GetUnit(), TStringBuf("rur"), TStringBuf("")) && EqualToOneOf(Ctx.GetParameter(), TStringBuf("price"), TStringBuf(""));
}

IParallelHandler::TTryResult TMarketChoice::HandleFilterSelection()
{
    Ctx.SetCurrentResult(GetResultUrl());

    auto response = MakeFilterRequestWithRegionHandling();
    if (response.HasError()) {
        return IsParallelMode
            ? IParallelHandler::TTryResult(IParallelHandler::ETryResult::NonSuitable)
            : IParallelHandler::TTryResult(response.GetError());
    }

    FilterWorker.UpdateFiltersDescription(response.GetFilters());

    if (response.GetTotal() == 0) {
        switch (Ctx.GetChoiceMarketType()) {
            case EMarketType::GREEN: {
                return RenderEmptyResult();
            }
            case EMarketType::BLUE: {
                // green and blue nids are different, so here we need to skip blue nid
                const auto& greenResponse = MakeFilterRequestAsync(
                    false /* allowRedirects */,
                    EMarketType::GREEN,
                    TCategory(Ctx.GetCategory().GetHid()),
                    Ctx.GetFesh()
                ).Wait();
                if (greenResponse.HasError()) {
                    return IsParallelMode
                        ? IParallelHandler::TTryResult(IParallelHandler::ETryResult::NonSuitable)
                        : IParallelHandler::TTryResult(greenResponse.GetError());
                }
                return RenderGreenSuggestOrEmptyResult(greenResponse);
            }
        }
    }
    if (!Ctx.WereFilterExamplesShown()) {
        FilterWorker.SetFilterExamples(response.GetFilters());
        Ctx.SetFilterExamplesShown();
    }
    return RenderGalleryAndAskContinue(response);
}

IParallelHandler::TTryResult TMarketChoice::HandleNoneRedirection(const TReportResponse& response, TStringBuf query)
{
    if (response.GetTotal() == 0) {
        switch (Ctx.GetChoiceMarketType()) {
            case EMarketType::GREEN:
                return RenderEmptyResult();
            case EMarketType::BLUE: {
                const auto& greenResponse = MakeSearchRequest(
                    query,
                    EMarketType::GREEN,
                    false /* allowRedirects */
                );
                if (greenResponse.HasError()) {
                    return IsParallelMode
                        ? IParallelHandler::TTryResult(IParallelHandler::ETryResult::NonSuitable)
                        : IParallelHandler::TTryResult(greenResponse.GetError());
                }
                return RenderGreenSuggestOrEmptyResult(greenResponse);
            }
        }
    }
    return RenderGalleryAndAskContinue(response);
}

IParallelHandler::TTryResult TMarketChoice::RenderGreenSuggestOrEmptyResult(const TReportResponse& greenResponse)
{
    if (greenResponse.GetTotal() == 0) {
        return RenderEmptyResult();
    }
    TString resultUrl = Ctx.DoesCategoryExist()
        // green and blue nids are different, so here we need to skip blue nid
        ? GetCategoryResultUrl(EMarketType::GREEN, TCategory(Ctx.GetCategory().GetHid()))
        : GetSearchResultUrl(EMarketType::GREEN);
    Ctx.RenderChoiceSuggestGreenSearch(resultUrl, greenResponse.GetTotal());
    Ctx.SetQueryInfo(InitialQueryInfo);
    return IParallelHandler::ETryResult::Success;
}

IParallelHandler::TTryResult TMarketChoice::RenderEmptyResult()
{
    if (Ctx.IsNative() && !IsParallelMode) {
        return ToTryResult(Ctx.RunSearchResponse());
    }
    Ctx.RenderChoiceEmptyResult();
    Ctx.SetQueryInfo(InitialQueryInfo);
    return IParallelHandler::ETryResult::NonSuitable;
}

bool TMarketChoice::TryRenderAskContinuePhraseFromMds(ui64 total)
{
    if (!Ctx.GetExperiments().MdsPhrases()) {
        return false;
    }
    if (Ctx.GetChoiceMarketType() != EMarketType::GREEN) {
        return false;
    }

    TString regionName;
    TString regionPrepcase;
    bool gotRegionNames = Ctx.TryGetUserRegionNames(&regionName, &regionPrepcase);
    if (!gotRegionNames) {
        return false;
    }

    const auto& phraseVariants = TDynamicDataFacade::GetPhraseVariants(
        Ctx.IsNative()
            ? (Ctx.IsOpen() ? TStringBuf("ask_continue_native_open") : TStringBuf("ask_continue_native"))
            : (Ctx.UserRegionWasChanged() ? TStringBuf("ask_continue_with_region") : TStringBuf("ask_continue")),
        Ctx.GetExperiments().ExpVersion()
    );

    if (phraseVariants.empty()) {
        return false;
    }
    auto& rng = Ctx.GetRng();
    Ctx.RenderChoiceAskContinueCustom(
        phraseVariants[rng.RandomInteger(phraseVariants.size())],
        total,
        regionName,
        regionPrepcase);
    return true;
}

IParallelHandler::TTryResult TMarketChoice::RenderGalleryAndAskContinue(const TReportResponse& response)
{
    Ctx.ClearResult();
    Ctx.SetFirstRequest(false);
    if (FillResult(response.GetResults())) {
        Ctx.AddTotal(response.GetTotal());
        Ctx.RenderChoiceGallery(Ctx.GetChoiceMarketType() == EMarketType::GREEN);
        bool addedCustomPhrase = TryRenderAskContinuePhraseFromMds(response.GetTotal());
        if (!addedCustomPhrase) {
            Ctx.RenderChoiceAskContinue(response.GetTotal());
        }
        if (Ctx.IsNative() && Ctx.GetChoiceMarketType() != EMarketType::BLUE && Ctx.GetExperiments().ActivationUrls()) {
            AddNativeActivationUrls();
        }
        return CheckResultsCategory(response.GetResults());
    } else {
        return RenderEmptyResult();
    }
}

void TMarketChoice::AddNativeActivationUrls()
{
    static const TVector<TString> marketPrefixPhrases = GetMarketPrefixes();

    const auto& marketSearchRequest = NMarket::ClearRequests(
        Ctx.Utterance(),
        marketPrefixPhrases,
        TVector<std::pair<TString, TString>>()
    );


    Ctx.AddSuggest(
        TStringBuf("market__market_search"),
        NSc::TValue(TMarketUrlBuilder(Ctx).GetMarketSearchUrl(
            DEFAULT_MARKET_TYPE, marketSearchRequest, Ctx.UserRegion(), EMarketGoodState::NEW, true /* redirect */))
    );


    if (Ctx.GetExperiments().AdsUrl()) {
        const auto& searchUri = GenerateSearchAdsUri(&Ctx.Ctx(), Ctx.Utterance());
        Ctx.AddSuggest(TStringBuf("market__yandex_search_ads"), NSc::TValue(searchUri));
    } else {
        const auto& searchUri = GenerateSearchUri(&Ctx.Ctx(), Ctx.Utterance());
        Ctx.AddSuggest(TStringBuf("market__yandex_search"), NSc::TValue(searchUri));
    }
}

NSc::TValue TMarketChoice::GetRatingIcons() const
{
    static TStringBuf iconNames[] = {
        TStringBuf("Fill"),
        TStringBuf("Half"),
        TStringBuf("None")
    };

    NSc::TValue result;
    for (const auto iconName : iconNames) {
        const auto iconUrl = Ctx.GetAvatarPictureUrl(TStringBuf("poi"), iconName);
        if (iconUrl) {
            result[iconName] = *iconUrl;
        }
    }

    return result;
}

bool TMarketChoice::CheckExitDueAttemptsLimit()
{
    ui32 attempt = Ctx.GetAttempt() + 1;
    if (attempt >= MAX_STEP_ATTEMPTS_COUNT) {
        SetState(EChoiceState::Exit);
        Ctx.RenderChoiceAttemptsLimit();
        return true;
    }
    if (attempt + 1 == MAX_STEP_ATTEMPTS_COUNT) {
        Ctx.SetAttemptReminder();
    }
    Ctx.SetAttempt(attempt);
    return false;
}

void TMarketChoice::SetState(EChoiceState state)
{
    State = state;
    Ctx.SetState(State);
}

TVector<TString> TMarketChoice::GetMarketPrefixes()
{
    TVector<TString> marketPrefixPhrases;
    Split(NResource::Find("market_prefix_phrases.txt"), "\n", marketPrefixPhrases);
    return marketPrefixPhrases;
}

bool TMarketChoice::CheckRedirectCategory(const TCategory& category) const
{
    if (!IsParallelMode) {
        return true;
    }
    bool allowWhiteList = Ctx.IsNativeOnMarket()
        ? Ctx.GetExperiments().AllowWhiteListMarket()
        : Ctx.GetExperiments().AllowWhiteList();
    bool allowBlackList = Ctx.IsNativeOnMarket()
        ? Ctx.GetExperiments().AllowBlackListMarket()
        : Ctx.GetExperiments().AllowBlackList();

    return !TDynamicDataFacade::IsDeniedCategory(
        category.GetHid(),
        Ctx.GetChoiceMarketType(),
        allowWhiteList,
        allowBlackList,
        Ctx.IsNativeOnMarket(),
        Ctx.GetExperiments().ExpVersion());
}

IParallelHandler::TTryResult TMarketChoice::CheckResultsCategory(const TVector<TReportResponse::TResult>& results) const
{
    for (const auto& result : results) {
        if (HasIllegalWarnings(result.GetWarnings())) {
            continue;
        }
        bool allowWhiteList = Ctx.IsNativeOnMarket()
            ? Ctx.GetExperiments().AllowWhiteListMarket()
            : Ctx.GetExperiments().AllowWhiteList();
        bool allowBlackList = Ctx.IsNativeOnMarket()
            ? Ctx.GetExperiments().AllowBlackListMarket()
            : Ctx.GetExperiments().AllowBlackList();

        if (TDynamicDataFacade::IsDeniedCategory(
            result.GetCategories(),
            Ctx.GetChoiceMarketType(),
            allowWhiteList,
            allowBlackList,
            Ctx.IsNativeOnMarket(),
            Ctx.GetExperiments().ExpVersion()))
        {
            return IParallelHandler::ETryResult::NonSuitable;
        }
    }
    return IParallelHandler::ETryResult::Success;
}

bool TMarketChoice::CheckMarketChoiceSupportedForGeoId() const
{
    bool isSupported = true;
    TMaybe<EMarketType> marketType = Ctx.GetChoiceMarketTypeMaybe();
    if (!marketType.Defined()) {
        marketType = Ctx.GetPreviousChoiceMarketType();
    }
    if (*marketType == EMarketType::GREEN) {
        if (!GeoSupport.IsMarketSupportedForGeoId(Ctx.UserRegion(), Ctx.GetScenarioType())) {
            isSupported = false;
        }
    } else {
        Y_ASSERT(*marketType != EMarketType::GREEN);
        if (!GeoSupport.IsBeruSupportedForGeoId(Ctx.UserRegion(), Ctx.GetScenarioType())) {
            isSupported = false;
        }
    }
    return isSupported;
}

} // namespace NMarket

} // namespace NBASS

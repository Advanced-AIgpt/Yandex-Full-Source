#pragma once

#include "experiments.h"
#include "state.h"
#include "types.h"
#include "types/model.h"
#include "types/offer.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/market/client/checkout.sc.h>
#include <alice/bass/util/error.h>
#include <alice/library/util/rng.h>
#include <util/generic/lazy_value.h>

namespace NBASS {

namespace NMarket {

using THowMuchScenarioContextScheme = NBassApi::THowMuchScenarioContextSlot<TBoolSchemeTraits>;

struct TExtendedGalleryOpts {
    enum class EActionType {
        MARKET,
        DETAILS_CARD,
        SHOP,
    };
    enum class EDetailsCardButton {
        GRAY,
        VIOLET,
        NONE,
    };
    EActionType CardAction = EActionType::DETAILS_CARD;
    EDetailsCardButton DetailsCardButton = EDetailsCardButton::GRAY;
    EActionType ShopNameAction = EActionType::SHOP;
    bool RenderShopName = true;
    bool RenderRating = false;
    bool RenderAdviserPercentage = false;
    ui8 ReasonsToBuyMaxSize = 0;
    bool EnableVoicePurchase = true;

    NSc::TValue ToJson() const;
    bool RenderVoicePurchaseMark() const;
};

struct TProductOffersCardOpts {
    enum class EOfferActionType {
        SHOP,
        MARKET,
    };
    bool EnableVoicePurchase = true;
    EOfferActionType OfferAction = EOfferActionType::SHOP;

    NSc::TValue ToJson() const;
};

class TMarketContext {
public:
    TMarketContext(TContext& ctx, EScenarioType scenarioType = EScenarioType::OTHER);

    TContext& Ctx() const { return *CtxPtr; }

    const TContext::TMeta& Meta() const { return Ctx().Meta(); }
    const TClientInfo& MetaClientInfo() const { return Ctx().MetaClientInfo(); }
    const TConfig& GetConfig() const { return Ctx().GetConfig(); }
    bool HasExpFlag(TStringBuf name) const { return Ctx().HasExpFlag(name); }
    TMaybe<TStringBuf> ExpFlag(TStringBuf name) const { return Ctx().ExpFlag(name); }
    const TString& ReqId() const { return Ctx().ReqId(); }
    NAlice::IRng& GetRng() const { return Ctx().GetRng(); }
    EScenarioType GetScenarioType() const { return ScenarioType; }
    TStringBuf RequestId() const { return Meta().RequestId(); }
    TStringBuf BaseMarketRequestId() const { return MarketReqId; };
    TString MarketRequestId();
    bool IsScreenless() const;
    // TODO(bas1330): MEGAMIND-1486 use ClientFeatures method instead
    bool CanOpenUri() const;

    TSourcesRequestFactory GetSources() const;
    const TMarketExperiments& GetExperiments() const;
    const TString& FormName() const;
    TStringBuf Utterance() const;
    TStringBuf Request(bool onlyRequestSlot = false) const;
    void SetRequest(TStringBuf request);
    i64 UserRegion() const;
    void SetUserRegion(i64);
    bool TryGetUserRegionNames(TString* name, TString* namePrepcase) const;
    bool UserRegionWasChanged() const;
    TMaybe<TString> GetAvatarPictureUrl(TStringBuf ns, TStringBuf name) const;

    bool HasUid() const;
    TStringBuf GetUid() const;

    TStringBuf GetState() const;
    void SetState(TStringBuf state);

    bool HasSlot(TStringBuf name) const;
    TStringBuf GetStringSlot(TStringBuf name) const;
    void SetStringSlot(TStringBuf name, TStringBuf value);
    bool GetBoolSlot(TStringBuf name, bool defaultValue = false) const;
    void SetBoolSlot(TStringBuf name, bool value);
    NSc::TValue GetSlot(TStringBuf name) const;
    void SetSlot(TStringBuf name, NSc::TValue value);
    TStringBuf GetSlotSource(TStringBuf name) const;


    template <typename StateEnum>
    StateEnum GetState(StateEnum defaultState) const
    {
        return GetState().empty() ? defaultState : FromString<StateEnum>(GetState());
    }
    template <typename StateEnum>
    void SetState(StateEnum state)
    {
        SetState(TStringBuf(ToString(state)));
    }

    bool IsCheckoutStep() const;
    TCheckoutState GetCheckoutState() const;

    EMarketType GetChoiceMarketType(TMaybe<EMarketType> default_ = Nothing()) const;
    TMaybe<EMarketType> GetChoiceMarketTypeMaybe() const;
    TMaybe<EMarketType> GetProductMarketType() const;
    EMarketType GetPreviousChoiceMarketType() const;
    void SetChoiceMarketType(EMarketType type);
    bool HasChoiceMarketType() const;

    EClids GetMarketClid() const;
    void SetMarketClid(EClids type);
    bool HasMarketClid() const;

    bool HasBeruOrderSku() const;
    ui64 GetBeruOrderSku() const;
    void SetBeruOrderSku(ui64 sku);

    void SetCategory(const TCategory& category);
    const TCategory& GetCategory() const;
    void SetFesh(const TVector<i64>& fesh);
    const TVector<i64>& GetFesh() const;
    void ClearFesh();
    void AddTotal(i64 total);
    void AddHowMuchTotal(i64 total);
    bool DoesCategoryExist() const;
    void ClearCategory();

    TGlFilters GetGlFilters() const;
    NSc::TValue GetRawGlFilters() const;
    TCgiGlFilters GetCgiGlFilters() const;
    void AddGlFilter(const TGlFilter& filter);
    void AddGlFilter(const TStringBuf id, const TVector<TString>& values);
    void ClearGlFilter(const TStringBuf id);

    void ClearGlFilters();
    void ClearAllSlots();
    void ClearPreviousSearchSlots();
    void SetGlFilterDescription(const TStringBuf description);

    void SetPrice(const NSc::TValue& from, const NSc::TValue& to);
    bool DoesPriceExist() const;
    const NSc::TValue& GetPrice() const;
    NSlots::TChoicePriceSchemeConst GetPriceScheme() const;
    void ClearPrice();

    void SetFilterExamples(const TVector<NSc::TValue>& filterExamples);
    void ClearFilterExamples();

    void SetFirstRequest(bool firstRequest);
    bool WereFilterExamplesShown() const;
    void SetFilterExamplesShown();

    TVector<int> GetIndices() const;
    TVector<int> GetExclusiveIndices() const;

    void SetModel(const TModel& model);
    void AddModelSnippet(const TModel& model);
    void ClearModel();

    bool HasMuid() const;
    TMaybe<TMuid> GetMuid() const;
    void SetMuid(const TMuid& muid);
    void DeleteMuid();

    void SetPopularGoodCategoryName(TStringBuf name);
    void SetPopularGoodPrices(const NSc::TValue& prices);
    void AddPopularGoodOffer(const TOffer& offer, ui32 galleryPosition, bool isVoicePurchase=false);
    void AddPopularGoodModel(const TModel& model, ui32 galleryPosition, bool isVoicePurchase=false);
    void SetPopularGoodUrl(TStringBuf url);
    void ClearPopularGood();
    THowMuchScenarioContextScheme GetHowMuchScenarioCtx();

    void SetCurrentResult(const TString& result);
    void SetResult(const TString& result);
    void ClearResult();

    const NSc::TValue& GetResultModels();
    NSc::TValue& CreateResultModels();

    i64 GetItemsNumber() const;

    NSc::TValue CreateJsonedModel(
        const TModel& model,
        ui32 galleryPosition,
        bool isVoicePurchase = false) const;

    NSc::TValue CreateJsonedBlueOffer(
        const TModel& model,
        ui32 galleryPosition,
        bool isVoicePurchase = false) const;

    NSc::TValue CreateJsonedOffer(
        const TOffer& offer,
        ui32 galleryPosition,
        bool isVoicePurchase = false) const;

    TStringBuf GetTextRedirect() const;
    void SetTextRedirect(TStringBuf result);

    TStringBuf GetSuggestTextRedirect() const;
    void SetSuggestTextRedirect(TStringBuf result);

    TRedirectCgiParams GetRedirectCgiParams() const;
    void SetRedirectCgiParams(const TRedirectCgiParams& result);
    void ClearRedirectCgiParams();

    double GetAmount() const;
    double GetAmountFrom() const;
    double GetAmountTo() const;
    const TStringBuf GetUnit() const;
    const TStringBuf GetParameter() const;
    bool DoesAmountExist() const;
    bool DoesAmountNeedRange() const;
    bool DoesAmountFromExist() const;
    bool DoesAmountToExist() const;
    bool DoesAnyAmountExist() const;
    void ClearAmounts();

    bool IsCalledDirectly() const;
    TStringBuf GetCalledFrom() const;
    void SetCalledFrom(TStringBuf calledFrom);

    bool IsNative() const;
    void SetNative(bool value);
    bool IsNativeOnMarket();

    bool IsOpen() const;
    void SetOpen(bool value);

    const NSc::TValue& GetConfirmation() const;
    void Log(const TStringBuf log);

    bool IsDebugMode() { return Ctx().HasExpFlag(TStringBuf("market_debug")); }
    void AddDebugTraces();

    bool HasProduct() const;
    NSlots::TProduct GetProduct() const;
    void SetProduct(NSlots::TProduct);

    void SetQueryInfo(const TQueryInfo& info);

    void SetVulgar();
    void SetCurrency(TStringBuf currency);

    ui32 GetAttempt() const;
    void SetAttempt(ui32 value);
    void SetAttemptReminder();

    void SetGalleryNumber(ui32 galleryNumber);
    ui32 GetGalleryNumber() const;
    void SetProductGalleryNumber(ui32 galleryNumber);
    ui32 GetProductGalleryNumber() const;
    ui32 GetGalleryPosition() const;

    void RenderHowMuchEmptySerp();
    void RenderEmptySerp();
    void RenderDebugInfo();

    void RenderHowMuchModel();
    void RenderHowMuchPopularGoods(TStringBuf currentRequest, bool showBeruAdv = false);

    void RenderChoiceEmptyResult();
    void RenderChoiceAskContinue(ui64 total);
    void RenderChoiceAskContinueCustom(TStringBuf phrase, ui64 total, TStringBuf region, TStringBuf regionPrepcase);
    void RenderChoiceGallery(bool useExtendedGallery=false);
    void RenderExtendedGallery(
        const NSc::TArray& docs,
        TStringBuf resultUrl,
        EMarketType resultUrlType,
        ui64 totalDocs,
        TExtendedGalleryOpts = {});
    void RenderChoiceProductOutdated();
    void RenderChoiceBeruOfferOutdated();
    void RenderChoiceBeruNoSku();
    void RenderChoiceNoSkuForCart();
    void RenderChoiceAddToCart();
    void OpenBundleUrl(TStringBuf url);
    void RenderChoiceProductDetailsCard(const NSc::TValue& details);
    void RenderChoiceProductOffersCard(const NSc::TValue& data, const TProductOffersCardOpts& opts = {});
    void RenderChoiceBeruProductDetailsCard(const NSc::TValue& details);
    void RenderChoiceBeruOrderCard(const TBeruOrderCardData& data);
    void RenderChoiceSuggestGreenSearch(TStringBuf greenUrl, ui64 greenTotal);
    void RenderActivationPhrase(bool freeDelivery);
    void RenderNoActivationPhrase();
    void RenderCancel();
    void RenderChoiceGarbage();
    void RenderChoiceStartChoiceAgain(bool isClosed);
    void RenderChoiceAttemptsLimit();
    void RenderMarketNotSupportedInLocation(EMarketType rgb, TStringBuf geoLocation);
    void RenderYandexSearch();

    void SetResponseForm(TStringBuf formName);
    void SetResponseFormAndCopySlots(TStringBuf formName, std::initializer_list<TStringBuf> slotNames);
    void AddMarketSuggests();
    void AddProductDetailsCardSuggests(bool isBlueOffer, bool hasShopUrl);
    void AddBeruProductDetailsCardSuggest();
    void AddBeruOrderCardSuggests();

    NSc::TValue GetProductDetailsFormUpdate(
        const NSlots::TProduct& product,
        ui32 galleryNumber,
        ui32 galleryPosition) const;
    TFormUpdate GetBeruOrderFormUpdate(ui64 sku, ui32 galleryNumber, ui32 galleryPosition) const;
    TFormUpdate GetCheckoutFormUpdate(ui64 sku) const;
    TFormUpdate GetAddToCartFormUpdate(ui64 sku) const;
    static TFormUpdate CreateSkuFormUpdate(ui64 sku, TStringBuf formName);

    TResultValue RunSearchResponse();

    TContext::TBlock* AddSuggest(TStringBuf type, NSc::TValue data = NSc::Null(), NSc::TValue formUpdate = NSc::Null());
    NSc::TValue DeleteSuggest(TStringBuf type);
    void AddSearchSuggest(TStringBuf query = TStringBuf());
    bool AddAuthorizationSuggest();
    void AddOnboardingSuggest();
    void AddOpenBlueSuggest();

    TContext::TBlock* AddTextCardBlock(TStringBuf id, NSc::TValue data = NSc::TValue());
    TContext::TBlock* AddDivCardBlock(TStringBuf type, NSc::TValue data);

private:
    static constexpr TStringBuf STATE_SLOT_NAME = "market__state";
    static constexpr TStringBuf STATE_SLOT_TYPE = "string";

    TLazyValue<TCategory> InitCategory();
    NSc::TValue& GetJsonedCategory() const;
    NSc::TValue CreateJsonedCategory(const TCategory& category) const;
    void AddGlFilter(const TStringBuf id, const NSc::TValue& value);
    void AddOpenSerpSearchUriCommand(TStringBuf uri);
    TMaybe<EMarketType> GetMarketTypeSlotValueMaybe(TStringBuf slotName) const;
    EMarketType GetMarketTypeSlotValue(
        TStringBuf slotName,
        TMaybe<EMarketType> default_ = Nothing()) const;
    void AddBeruActivationSuggest(TStringBuf modelName, bool attachToCard, EReferer referer);

    template <class TValue>
    static void AddSlotToFormUpdate(
        TStringBuf slotName,
        TStringBuf slotType,
        const TValue& slotValue,
        TFormUpdate& formUpdate)
    {
        TSlot slot(slotName, slotType);
        slot.Value = slotValue;
        formUpdate->Slots().Add()->CopyFrom(slot.ToJson(nullptr));
    }

    template <class EScenarioState>
    static void AddStateToFormUpdate(
        EScenarioState state,
        TFormUpdate& formUpdate)
    {
        AddStateValueToFormUpdate(ToString(state), formUpdate);
    }

    static void AddStateValueToFormUpdate(
        TStringBuf stateValue,
        TFormUpdate& formUpdate)
    {
        AddSlotToFormUpdate(STATE_SLOT_NAME, STATE_SLOT_TYPE, stateValue, formUpdate);
    }

    bool CheckValidMarketReqId(TStringBuf id) const;

private:
    TContext::TPtr CtxPtr;
    EScenarioType ScenarioType;
    TMarketExperiments Experiments;
    TLazyValue<TCategory> Category;
    TString MarketReqId;
    ui32 SubReqCounter;
    TVector<i64> Fesh;
    bool RegionWasChanged = false;
};

} // namespace NMarket

} // namespace NBASS

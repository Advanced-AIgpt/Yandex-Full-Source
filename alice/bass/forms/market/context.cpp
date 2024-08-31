#include "context.h"

#include "client.h"
#include "dynamic_data.h"
#include "forms.h"
#include "market_exception.h"
#include "market_url_builder.h"
#include "settings.h"
#include "types/filter.h"
#include "util/serialize.h"
#include "util/suggests.h"
#include "util/amount.h"

#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/search/search.h>
#include <alice/bass/forms/urls_builder.h>
#include <util/charset/utf8.h>

#include <alice/bass/libs/avatars/avatars.h>

namespace NBASS {

namespace NMarket {

namespace {

const NSc::TValue null;

const TStringBuf MuidSlotName = "muid";

class TGlFilterSerializer {
public:
    explicit TGlFilterSerializer(NSc::TValue& rawGlFilters)
        : RawGlFilters(rawGlFilters)
    {
    }
    void operator()(const TBoolGlFilter& filter)
    {
        auto& rawFilter = GetFilterWithBaseInfo(filter);
        rawFilter["type"] = TStringBuf("boolean");
        rawFilter["value"] = filter.Value;
    }
    void operator()(const TNumberGlFilter& filter)
    {
        auto& rawFilter = GetFilterWithBaseInfo(filter);
        rawFilter["type"] = TStringBuf("number");
        if (filter.Min.Defined()) {
            rawFilter["min"] = filter.Min.GetRef();
        }
        if (filter.Max.Defined()) {
            rawFilter["max"] = filter.Max.GetRef();
        }
        rawFilter["unit"] = filter.Unit;
    }
    void operator()(const TEnumGlFilter& filter)
    {
        auto& rawFilter = GetFilterWithBaseInfo(filter);
        rawFilter["type"] = TStringBuf("enum");
        auto& rawValues = rawFilter["values"];
        for (const auto& [id, value] : filter.Values) {
            NSc::TValue rawValue;
            rawValue["name"] = value.Name;
            rawValues[id] = rawValue;
        }
    }
    void operator()(const TRawGlFilter& filter)
    {
        auto& rawFilter = RawGlFilters[filter.Id];
        rawFilter["type"] = TStringBuf("raw");
        for (const auto& value : filter.Values) {
            rawFilter["values"].Push(value);
        }
    }

private:
    NSc::TValue& RawGlFilters;

    template <class TFilter>
    NSc::TValue& GetFilterWithBaseInfo(const TFilter& filter)
    {
        auto& rawFilter = RawGlFilters[filter.Id];
        rawFilter = NSc::TValue();
        rawFilter["name"] = filter.Name;
        return rawFilter;
    }
};

class TCgiValuesExtractor {
public:
    explicit TCgiValuesExtractor(TCgiGlFilters& filters)
        : Filters(filters)
    {
    }
    void operator()(const TBoolGlFilter& filter) const
    {
        TVector<TString> values;
        values.push_back(filter.Value ? "1" : "0");
        Filters[filter.Id] = values;
    }
    void operator()(const TNumberGlFilter& filter) const
    {
        TVector<TString> values;

        TStringBuilder value;
        if (filter.Min) {
            value << filter.Min.GetRef();
        }
        value << '~';
        if (filter.Max) {
            value << filter.Max.GetRef();
        }
        values.push_back(value);

        Filters[filter.Id] = values;
    }
    void operator()(const TEnumGlFilter& filter) const
    {
        TVector<TString> values;

        for (const auto& kv : filter.Values) {
            values.push_back(ToString(kv.first));
        }

        Filters[filter.Id] = values;
    }
    void operator()(const TRawGlFilter& filter) const
    {
        Filters[filter.Id] = filter.Values;
    }

private:
    TCgiGlFilters& Filters;
};

NSc::TValue CreateBaseJsonedDoc(
    TStringBuf type,
    TStringBuf title,
    const TPicture& picture,
    const NSc::TValue& prices,
    bool isVoicePurchase)
{
    NSc::TValue doc;
    doc["type"] = type;
    doc["title"] = title;
    SerializePicture(picture, doc["picture"]);
    doc["prices"] = prices;
    if (isVoicePurchase) {
        doc["voice_purchase"] = true;
    }
    return doc;
}

} // namespace

NSc::TValue TExtendedGalleryOpts::ToJson() const
{
    NSc::TValue result;
    result["card_action"] = ToString(CardAction);
    result["details_card_button"] = ToString(DetailsCardButton);
    result["shop_name_action"] = ToString(ShopNameAction);
    result["render_voice_purchase_mark"] = RenderVoicePurchaseMark();
    result["render_shop_name"] = RenderShopName;
    result["render_rating"] = RenderRating;
    result["render_adviser_percentage"] = RenderAdviserPercentage;
    result["reasons_to_buy_max_size"] = ReasonsToBuyMaxSize;
    return result;
}

bool TExtendedGalleryOpts::RenderVoicePurchaseMark() const
{
    if (!EnableVoicePurchase) {
        return false;
    }
    return CardAction == EActionType::DETAILS_CARD || DetailsCardButton != EDetailsCardButton::NONE;
}

NSc::TValue TProductOffersCardOpts::ToJson() const
{
    NSc::TValue result;
    result["enable_voice_purchase"] = EnableVoicePurchase;
    result["offer_action"] = ToString(OfferAction);
    return result;
}

bool TMarketContext::CheckValidMarketReqId(TStringBuf id) const
{
    return !id.Empty();
}

TMarketContext::TMarketContext(TContext& ctx, EScenarioType scenarioType)
    : CtxPtr(&ctx)
    , ScenarioType(scenarioType)
    , Experiments(TMarketExperiments(ctx))
    , Category(InitCategory())
    , SubReqCounter(0)
{
    if (CheckValidMarketReqId(ctx.MarketReqId())) {           // try to pick one up from headers
        MarketReqId = ctx.MarketReqId();
        LOG(DEBUG) << "Picked a valid MarketReqId from request headers." << Endl;
    } else {                                                  // generate new otherwise
        MarketReqId = TStringBuilder() << ctx.Now().MilliSeconds() << TStringBuf("/") << RequestId();
        LOG(DEBUG) << "Generated a fresh new MarketReqId from request data." << Endl;
    }
    LOG(INFO) << "X-Market-Req-ID: " << MarketReqId << Endl;
}

TString TMarketContext::MarketRequestId()
{
    return TStringBuilder() << MarketReqId << "/" << (++SubReqCounter);
}

bool TMarketContext::IsScreenless() const
{
    return !Ctx().ClientFeatures().SupportsDivCards() || GetExperiments().Screenless();
}

bool TMarketContext::CanOpenUri() const
{
    if (GetExperiments().Screenless()) {
        return false;
    }
    const auto& clientFeatures = Ctx().ClientFeatures();
    return clientFeatures.IsSearchApp()
        || clientFeatures.IsYaBrowser()
        || clientFeatures.IsYaLauncher();
}

TMaybe<EMarketType> TMarketContext::GetMarketTypeSlotValueMaybe(TStringBuf slotName) const {
    const TSlot* const slot = Ctx().GetSlot(slotName);
    if (!IsSlotEmpty(slot)) {
        EMarketType type;
        TStringBuf typeStr = slot->Value.GetString();
        if (TryFromString<EMarketType>(typeStr, type)) {
            return type;
        }
        LOG(DEBUG) << "Got unexpected market type \"" << typeStr << "\"" << Endl;
        Y_ASSERT(false);
    }
    return Nothing();
}

EMarketType TMarketContext::GetMarketTypeSlotValue(
    TStringBuf slotName,
    TMaybe<EMarketType> default_) const
{
    if (TMaybe<EMarketType> result = GetMarketTypeSlotValueMaybe(slotName)) {
        return *result;
    }

    if (default_.Defined()) {
        return default_.GetRef();
    }
    switch (ScenarioType) {
        case EScenarioType::HOW_MUCH:
            return DEFAULT_MARKET_TYPE;
        case EScenarioType::CHOICE:
            return DEFAULT_MARKET_TYPE;
        case EScenarioType::RECURRING_PURCHASE:
            return EMarketType::BLUE;
        case EScenarioType::BERU_BONUSES:
            return EMarketType::BLUE;
        case EScenarioType::ORDERS_STATUS:
            return EMarketType::BLUE;
        case EScenarioType::SHOPPING_LIST:
            return EMarketType::BLUE;
        case EScenarioType::OTHER:
            return DEFAULT_MARKET_TYPE;
    }
}

TSourcesRequestFactory TMarketContext::GetSources() const
{
    return Ctx().GetSources();
}

const TMarketExperiments& TMarketContext::GetExperiments() const
{
    return Experiments;
}

EMarketType TMarketContext::GetChoiceMarketType(TMaybe<EMarketType> default_) const
{
    return GetMarketTypeSlotValue(TStringBuf("choice_market_type"), default_);
}

TMaybe<EMarketType> TMarketContext::GetChoiceMarketTypeMaybe() const
{
    return GetMarketTypeSlotValueMaybe(TStringBuf("choice_market_type"));
}

TMaybe<EMarketType> TMarketContext::GetProductMarketType() const
{
    const TSlot* const slot = Ctx().GetSlot("product_market_type");
    if (!IsSlotEmpty(slot)) {
        EMarketType type;
        if (TryFromString<EMarketType>(slot->Value.GetString(), type)) {
            return type;
        }
        LOG(DEBUG) << "Got unexpected product market type slot value: " << slot->Value.ToJsonPretty() << Endl;
        Y_ASSERT(false);
    }
    return Nothing();
}

EMarketType TMarketContext::GetPreviousChoiceMarketType() const
{
    if (auto productType = GetProductMarketType(); productType.Defined()) {
        return *productType;
    }
    // Повторяем логику определения цвета сценария из market_choice.cpp, потому что нужно
    // уметь определять его до вызова TMarketChoice::Do
    switch (FromString<EChoiceForm>(FormName())) {
        case EChoiceForm::Native:
        case EChoiceForm::Activation:
            return EMarketType::GREEN;
        case EChoiceForm::BeruActivation:
            return EMarketType::BLUE;
        case EChoiceForm::NativeBeru:
            return GetChoiceMarketType(EMarketType::BLUE);
        default:
            return GetChoiceMarketType(EMarketType::GREEN);
    }
}

void TMarketContext::SetChoiceMarketType(EMarketType type)
{
    NSc::TValue value;
    value.SetString(ToString(type));
    Ctx().CreateSlot(TStringBuf("choice_market_type"), TStringBuf("market_type"), true /* optional */, value);
}

bool TMarketContext::HasChoiceMarketType() const
{
    return !IsSlotEmpty(Ctx().GetSlot("choice_market_type"));
}

EClids TMarketContext::GetMarketClid() const
{
    const TSlot* const slot = Ctx().GetSlot(TStringBuf("market_clid"));
    if (!IsSlotEmpty(slot)) {
        EClids clid;
        TStringBuf clidStr = slot->Value.GetString();
        if (TryFromString<EClids>(clidStr, clid)) {
            return clid;
        }
        LOG(DEBUG) << "Got unexpected market clid \"" << clidStr << "\"" << Endl;
        Y_ASSERT(false);
    }
    switch (ScenarioType) {
        case EScenarioType::HOW_MUCH:
            return EClids::HOW_MUCH;
        case EScenarioType::CHOICE:
            switch (GetChoiceMarketType()) {
                case EMarketType::BLUE:
                    return EClids::CHOICE_BLUE;
                case EMarketType::GREEN:
                    return EClids::CHOICE_GREEN;
            }
        case EScenarioType::RECURRING_PURCHASE:
            return EClids::RECURRING_PURCHASE;
        case EScenarioType::ORDERS_STATUS:
            return EClids::ORDERS_STATUS;
        case EScenarioType::BERU_BONUSES:
            return EClids::BERU_BONUSES;
        case EScenarioType::SHOPPING_LIST:
            return EClids::SHOPPING_LIST;
        case EScenarioType::OTHER:
            return EClids::OTHER;
    }
}

void TMarketContext::SetMarketClid(EClids type)
{
    NSc::TValue value;
    value.SetString(ToString(type));
    Ctx().CreateSlot(TStringBuf("market_clid"), TStringBuf("clid_type"), true /* optional */, value);
}

bool TMarketContext::HasMarketClid() const
{
    return !IsSlotEmpty(Ctx().GetSlot("market_clid"));
}

const TString& TMarketContext::FormName() const
{
    return Ctx().FormName();
}

TStringBuf TMarketContext::Utterance() const
{
    TStringBuf asrUtterance = Meta()->AsrUtterance();
    auto utterance = asrUtterance.empty() ? Meta()->Utterance() : asrUtterance;
    if (utterance.EndsWith('.')) {
        utterance.Chop(1);
    }
    return utterance;
}

TStringBuf TMarketContext::Request(bool onlyRequestSlot) const
{
    // фраза при активации маркетного сценария
     const TSlot* const slot = Ctx().GetSlot("request");
     if (!IsSlotEmpty(slot)) {
        return slot->Value.GetString();
     }

    if (onlyRequestSlot) {
        return TStringBuf("");
    }

    // фраза при активации по колдунщику
    const TSlot* const serp = Ctx().GetSlot("query");
    if (!IsSlotEmpty(serp)) {
        return serp->Value.GetString();
    }

    // если совсем все плохо берем полностью всю исходную фразу
    // может содержать символы в произвольном регистре при наборе с клавиатуры
    return Utterance();
}

void TMarketContext::SetRequest(TStringBuf request)
{
    Ctx().CreateSlot(TStringBuf("request"), TStringBuf("string"), true, request);
}

i64 TMarketContext::UserRegion() const
{
    const auto slot = Ctx().GetSlot(TStringBuf("region_id"), TStringBuf("region_id"));
    if (!IsSlotEmpty(slot)) {
        return slot->Value.GetIntNumber();
    }
    return Ctx().UserRegion();
}

bool TMarketContext::TryGetUserRegionNames(TString* name, TString* namePrepcase) const
{
    NAlice::GeoIdToNames(Ctx().GlobalCtx().GeobaseLookup(), UserRegion(), "ru", name, namePrepcase);
    if (!name || name->Empty()) {
        return false;
    }
    if (!namePrepcase || namePrepcase->Empty()) {
        return false;
    }
    return true;
}

void TMarketContext::SetUserRegion(i64 regionId)
{
    Ctx().CreateSlot(TStringBuf("region_id"), TStringBuf("region_id"), true, NSc::TValue().SetIntNumber(regionId));
    RegionWasChanged = true;
}

bool TMarketContext::UserRegionWasChanged() const
{
    return RegionWasChanged;
}

TMaybe<TString> TMarketContext::GetAvatarPictureUrl(TStringBuf ns, TStringBuf name) const
{
    const auto* avatar = Ctx().Avatar(ns, name);
    if (avatar) {
        return avatar->Https;
    }
    return Nothing();
}

bool TMarketContext::HasUid() const
{
    const TSlot* const slot = Ctx().GetSlot("uid");
    return !IsSlotEmpty(slot);
}

TStringBuf TMarketContext::GetUid() const
{
    const TSlot* const slot = Ctx().GetSlot("uid");
    if (!IsSlotEmpty(slot)) {
        return slot->Value.GetString();
    }
    return TStringBuf();
}

TStringBuf TMarketContext::GetState() const
{
    const TSlot* const stateSlot = Ctx().GetSlot(STATE_SLOT_NAME);
    if (!IsSlotEmpty(stateSlot)) {
        return stateSlot->Value.GetString();
    }
    return TStringBuf();
}

void TMarketContext::SetState(TStringBuf state)
{
    auto slot = Ctx().GetOrCreateSlot(STATE_SLOT_NAME, STATE_SLOT_TYPE);
    Y_ASSERT(slot);
    slot->Value.SetString(state);
}

bool TMarketContext::HasSlot(TStringBuf name) const
{
    const TSlot* const stateSlot = Ctx().GetSlot(name);
    return !IsSlotEmpty(stateSlot);
}

TStringBuf TMarketContext::GetStringSlot(TStringBuf name) const
{
    const TSlot* const stateSlot = Ctx().GetSlot(name);
    if (!IsSlotEmpty(stateSlot)) {
        return stateSlot->Value.GetString();
    }
    return TStringBuf();
}

void TMarketContext::SetStringSlot(TStringBuf name, TStringBuf value)
{
    auto slot = Ctx().GetOrCreateSlot(name, "string");
    Y_ASSERT(slot);
    slot->Value.SetString(value);
}

bool TMarketContext::GetBoolSlot(TStringBuf name, bool defaultValue /*= false*/) const
{
    const TSlot* const stateSlot = Ctx().GetSlot(name);
    if (!IsSlotEmpty(stateSlot)) {
        return stateSlot->Value.GetBool(defaultValue);
    }
    return defaultValue;
}

void TMarketContext::SetBoolSlot(TStringBuf name, bool value)
{
    auto slot = Ctx().GetOrCreateSlot(name, "bool");
    Y_ASSERT(slot);
    slot->Value.SetBool(value);
}

NSc::TValue TMarketContext::GetSlot(TStringBuf name) const
{
    const TSlot* const stateSlot = Ctx().GetSlot(name);
    if (!IsSlotEmpty(stateSlot)) {
        return stateSlot->Value;
    }
    return TStringBuf();
}

void TMarketContext::SetSlot(TStringBuf name, NSc::TValue value)
{
    auto slot = Ctx().GetOrCreateSlot(name, "string");
    Y_ASSERT(slot);
    slot->Value = value;
}

TStringBuf TMarketContext::GetSlotSource(TStringBuf name) const
{
    const TSlot* const stateSlot = Ctx().GetSlot(name);
    if (!IsSlotEmpty(stateSlot)) {
        return stateSlot->SourceText.GetString();
    }
    return TStringBuf();
}

bool TMarketContext::IsCheckoutStep() const
{
    switch (ScenarioType) {
        case EScenarioType::HOW_MUCH:
            return false;
        case EScenarioType::RECURRING_PURCHASE:
            return false;
        case EScenarioType::CHOICE:
            switch (FromString<EChoiceForm>(FormName())) {
                case EChoiceForm::Native:
                case EChoiceForm::NativeBeru:
                case EChoiceForm::Activation:
                case EChoiceForm::BeruActivation:
                case EChoiceForm::Cancel:
                case EChoiceForm::Garbage:
                case EChoiceForm::Repeat:
                case EChoiceForm::ShowMore:
                case EChoiceForm::StartChoiceAgain:
                case EChoiceForm::MarketChoice:
                case EChoiceForm::MarketChoiceEllipsis:
                case EChoiceForm::MarketParams:
                case EChoiceForm::MarketNumberFilter:
                case EChoiceForm::ProductDetails:
                case EChoiceForm::ProductDetailsExternal:
                case EChoiceForm::BeruOrder:
                case EChoiceForm::AddToCart:
                case EChoiceForm::GoToShop:
                    return false;
                case EChoiceForm::ProductDetailsExternal_BeruOrder:
                case EChoiceForm::Checkout:
                case EChoiceForm::CheckoutItemsNumber:
                case EChoiceForm::CheckoutEmail:
                case EChoiceForm::CheckoutPhone:
                case EChoiceForm::CheckoutAddress:
                case EChoiceForm::CheckoutIndex:
                case EChoiceForm::CheckoutDeliveryIntervals:
                case EChoiceForm::CheckoutYesOrNo:
                case EChoiceForm::CheckoutEverything:
                    return true;
            }
        case EScenarioType::BERU_BONUSES:
            return false;
        case EScenarioType::ORDERS_STATUS:
            return false;
        case EScenarioType::SHOPPING_LIST:
            return false;
        case EScenarioType::OTHER:
            return false;
    }
}

TCheckoutState TMarketContext::GetCheckoutState() const
{
    auto* stateSlot = Ctx().GetOrCreateSlot(TStringBuf("state"), TStringBuf("checkout_state"));
    TCheckoutState state(&stateSlot->Value);
    return state;
}

ui64 TMarketContext::GetBeruOrderSku() const
{
    const TSlot* const skuSlot = Ctx().GetSlot("sku");
    if (IsSlotEmpty(skuSlot)) {
        ythrow TMarketException(TStringBuf("SKU expected but doesn't exists"));
    }
    return skuSlot->Value.GetIntNumber();
}

bool TMarketContext::HasBeruOrderSku() const
{
    const TSlot* const skuSlot = Ctx().GetSlot("sku");
    return !IsSlotEmpty(skuSlot) && skuSlot->Value.IsIntNumber();
}

void TMarketContext::SetBeruOrderSku(ui64 sku)
{
    auto slot = Ctx().GetOrCreateSlot("sku", "number");
    slot->Value = sku;
}

void TMarketContext::SetResult(const TString& resultUrl)
{
    NSc::TValue& result = Ctx().GetOrCreateSlot(TStringBuf("result"), TStringBuf("result"))->Value;
    result["url"] = resultUrl;
    result["market_type"] = ToString(GetChoiceMarketType());
}

void TMarketContext::ClearResult()
{
    Ctx().CreateSlot(
        "result",
        "result",
        true, /* optional */
        null
    );
}

void TMarketContext::SetModel(const TModel& model)
{
    Ctx().CreateSlot(
        "model",
        "model",
        true, /* optional */
        CreateJsonedModel(model, 0 /* galleryPosition */)
    );
}

bool TMarketContext::HasMuid() const
{
    return !IsSlotEmpty(Ctx().GetSlot(MuidSlotName));
}

TMaybe<TMuid> TMarketContext::GetMuid() const
{
    const auto muidSlot = Ctx().GetSlot(MuidSlotName);
    if (IsSlotEmpty(muidSlot)) {
        return Nothing();
    }
    return TMuid(muidSlot->Value);
}

void TMarketContext::SetMuid(const TMuid& muid)
{
    Ctx().CreateSlot(MuidSlotName, TStringBuf("muid"), true, muid.ToJson());
}

void TMarketContext::DeleteMuid()
{

    Ctx().CreateSlot(MuidSlotName, TStringBuf("muid"), true, null);
}

const NSc::TValue& TMarketContext::GetResultModels()
{
    NSc::TValue& result = Ctx().GetOrCreateSlot("result", "result")->Value["models"];
    if (!result.IsArray()) {
        result.SetArray();
    }
    return result;
}

NSc::TValue& TMarketContext::CreateResultModels()
{
    return Ctx().GetOrCreateSlot("result", "result")->Value["models"].SetArray();
}

i64 TMarketContext::GetItemsNumber() const {
    const TSlot* const itemsNumberSlot = Ctx().GetSlot("items_number");
    if (IsSlotEmpty(itemsNumberSlot)) {
        LOG(ERR) << TStringBuf("items_number slot value expected but doesn't exist") << Endl;
        return 0;
    }
    return itemsNumberSlot->Value.GetIntNumber(0);
}

NSc::TValue TMarketContext::CreateJsonedModel(
    const TModel& model,
    ui32 galleryPosition,
    bool isVoicePurchase) const
{
    TMarketUrlBuilder urlBuilder(*this);

    auto getUrl = [this, &urlBuilder, &model, galleryPosition] (TMarketUrlBuilder::EMarketModelTab tab) {
        // todo почему не заполняем фильтры и цены?
        return urlBuilder.GetMarketModelUrl(
            model.GetId(),
            model.GetSlug(),
            UserRegion(),
            GetGalleryNumber() + 1,
            galleryPosition,
            TCgiGlFilters(),
            -1 /* priceFrom */,
            -1 /* priceTo */,
            TRedirectCgiParams(),
            tab);
    };
    NSc::TValue jsoned = CreateBaseJsonedDoc(
        TStringBuf("model"),
        model.GetTitle(),
        model.GetPicture(),
        SerializeModelPrices(model),
        isVoicePurchase
    );
    jsoned["id"] = model.GetId();
    if (model.HasRating()) {
        jsoned["rating"]["icon_url"] = urlBuilder.GetRatingIconUrl(model.GetRating());
        if (model.HasReviewCount()) {
            jsoned["rating"]["review_count"] = model.GetReviewCount();
            jsoned["rating"]["reviews_url"] = getUrl(TMarketUrlBuilder::EMarketModelTab::Reviews);
        }
    }
    if (model.HasAdviserPercentage()) {
        jsoned["adviser_percentage"] = model.GetAdviserPercentage();
    }
    if (!model.GetAdvantages().empty()) {
        const auto& advantages = model.GetAdvantages();
        for (size_t i = 0; i < std::min(MAX_GALLERY_MODEL_ADVANTAGES, advantages.size()); i++) {
            jsoned["reasons_to_buy"].Push() = advantages[i];
        }
    }

    NSlots::TProduct product;
    product->Id() = model.GetId();
    product->Type() = TStringBuf("model");
    if (model.GetDefaultOffer()) {
        product->WareId() = model.GetDefaultOffer()->GetWareId();
        product->ShopUrl() = model.GetDefaultOffer()->GetShopUrl();
        jsoned["shop_url"] = model.GetDefaultOffer()->GetShopUrl();
        jsoned["shop_name"] = model.GetDefaultOffer()->GetShop();
    }
    auto productFilters = product->GlFilters();
    for (const auto& kv : model.GetGlFilters()) {
        auto values = productFilters[kv.first];
        for (const auto& value : kv.second) {
            values.Add() = value;
        }
    }
    product->Price() = GetPriceScheme();
    if (DoesCategoryExist()) {
        product->Hid() = GetCategory().GetHid();
    }
    Y_ASSERT(product->Validate());
    jsoned["form_update"] = GetProductDetailsFormUpdate(product, GetGalleryNumber() + 1, galleryPosition);
    jsoned["url"] = getUrl(TMarketUrlBuilder::EMarketModelTab::Main);

    return jsoned;
}

NSc::TValue TMarketContext::CreateJsonedBlueOffer(
    const TModel& model,
    ui32 galleryPosition,
    bool isVoicePurchase) const
{
    TMarketUrlBuilder urlBuilder(*this);

    const auto& offerMaybe = model.GetDefaultOffer();
    if (offerMaybe.Empty() || offerMaybe.GetRef().GetSku().Empty()) {
        LOG(ERR) << "Blue model doesn't have offers. model_id: " << model.GetId() << Endl;
        Y_ASSERT(false);
        return NSc::TValue();
    }

    const auto& offer = offerMaybe.GetRef();
    NSc::TValue jsoned = CreateBaseJsonedDoc(
        TStringBuf("offer"),
        offer.GetTitle(),
        offer.GetPicture(),
        SerializeOfferPrices(offer),
        isVoicePurchase
    );
    jsoned["modelid"] = model.GetId();
    jsoned["sku"] = offer.GetSku().GetRef();

    NSlots::TProduct product;
    product->Id() = offer.GetSku().GetRef();
    product->WareId() = offer.GetWareId();
    product->ShopUrl() = offer.GetShopUrl();
    product->Type() = TStringBuf("sku");
    Y_ASSERT(product->Validate());
    jsoned["form_update"] = GetProductDetailsFormUpdate(product, GetGalleryNumber() + 1, galleryPosition);
    jsoned["shop_url"] = urlBuilder.GetBeruModelUrl(
        ToString(offer.GetSku().GetRef()),
        model.GetSlug(),
        UserRegion(),
        GetProductGalleryNumber(),
        galleryPosition);
    jsoned["shop_name"] = "БЕРУ";

    return jsoned;
}

NSc::TValue TMarketContext::CreateJsonedOffer(
    const TOffer& offer,
    ui32 galleryPosition,
    bool isVoicePurchase) const
{
    TMarketUrlBuilder urlBuilder(*this);

    NSc::TValue jsoned = CreateBaseJsonedDoc(
        TStringBuf("offer"),
        offer.GetTitle(),
        offer.GetPicture(),
        SerializeOfferPrices(offer),
        isVoicePurchase
    );
    jsoned["ware_id"] = offer.GetWareId();
    jsoned["shop"] = offer.GetShop();

    NSlots::TProduct product;
    if (ScenarioType == EScenarioType::RECURRING_PURCHASE
        && !offer.GetSku().Empty() && offer.GetSku().GetRef() != 0)
    {
        product->Id() = offer.GetSku().GetRef();
        product->WareId() = offer.GetWareId();
        product->Type() = TStringBuf("sku");
    } else {
        product->WareId() = offer.GetWareId();
        product->Type() = TStringBuf("offer");
    }
    product->ShopUrl() = offer.GetShopUrl();
    Y_ASSERT(product->Validate());
    jsoned["form_update"] = GetProductDetailsFormUpdate(product, GetGalleryNumber() + 1, galleryPosition);
    jsoned["shop_url"] = offer.GetShopUrl();
    jsoned["shop_name"] = offer.GetShop();

    jsoned["url"] = urlBuilder.GetMarketOfferUrl(
        offer.GetWareId(),
        offer.GetCpc(),
        UserRegion(),
        GetGalleryNumber() + 1,
        galleryPosition
    );

    return jsoned;
}

void TMarketContext::AddModelSnippet(const TModel& model)
{
    /* Используется только в сценарии "сколько стоит", поэтому отключаю карточку с детальной информацией о товаре
    */
    auto slot = Ctx().GetOrCreateSlot("model", "model");
    slot->Value["results"].Push(CreateJsonedModel(model, 0 /* galleryPosition */));
}

void TMarketContext::ClearModel()
{
    Ctx().CreateSlot("model", "model");
}

void TMarketContext::SetPopularGoodCategoryName(TStringBuf name)
{
    auto slot = Ctx().GetOrCreateSlot("popular_good", "popular_good");
    slot->Value["category_name"] = name;
}

void TMarketContext::SetPopularGoodPrices(const NSc::TValue& prices)
{
    auto slot = Ctx().GetOrCreateSlot("popular_good", "popular_good");
    slot->Value["prices"] = prices;
}

void TMarketContext::SetPopularGoodUrl(TStringBuf url)
{
    auto slot = Ctx().GetOrCreateSlot("popular_good", "popular_good");
    slot->Value["url"] = url;
}

void TMarketContext::AddPopularGoodOffer(const TOffer& offer, ui32 galleryPosition, bool isVoicePurchase)
{
    /* Используется только в сценарии "сколько стоит", поэтому отключаю карточку с детальной информацией о товаре
    */
    auto slot = Ctx().GetOrCreateSlot("popular_good", "popular_good");
    slot->Value["results"].Push(CreateJsonedOffer(offer, galleryPosition, isVoicePurchase));
}

void TMarketContext::AddPopularGoodModel(const TModel& model, ui32 galleryPosition, bool isVoicePurchase)
{
    /* Используется только в сценарии "сколько стоит", поэтому отключаю карточку с детальной информацией о товаре
    */
    auto slot = Ctx().GetOrCreateSlot("popular_good", "popular_good");
    slot->Value["results"].Push(CreateJsonedModel(model, galleryPosition, isVoicePurchase));
}

void TMarketContext::ClearPopularGood()
{
    Ctx().CreateSlot("popular_good", "popular_good");
}

void TMarketContext::SetCurrentResult(const TString& result)
{
    Log(TStringBuilder() << "Current result: " << result);
}

bool TMarketContext::DoesCategoryExist() const
{
    return !IsSlotEmpty(Ctx().GetSlot("category"));
}

TStringBuf TMarketContext::GetTextRedirect() const
{
    const TSlot* const slot = Ctx().GetSlot("text_redirect");
    if (IsSlotEmpty(slot)) {
        return TStringBuf();
    }
    return slot->Value.GetString();
}

void TMarketContext::SetTextRedirect(TStringBuf result)
{
    Ctx().CreateSlot(
        "text_redirect",
        "string",
        true, /* optional */
        NSc::TValue(result));
}

TStringBuf TMarketContext::GetSuggestTextRedirect() const
{
    const TSlot* const slot = Ctx().GetSlot("suggest_text_redirect");
    if (IsSlotEmpty(slot)) {
        return TStringBuf();
    }
    return slot->Value.GetString();
}

void TMarketContext::SetSuggestTextRedirect(TStringBuf result)
{
    Ctx().CreateSlot(
        "suggest_text_redirect",
        "string",
        true, /* optional */
        NSc::TValue(result));
}

TRedirectCgiParams TMarketContext::GetRedirectCgiParams() const
{
    const TSlot* const slot = Ctx().GetSlot("redirect_cgi_params");
    if (IsSlotEmpty(slot)) {
        return TRedirectCgiParams();
    }
    return TRedirectCgiParams(slot->Value["rs"].ForceString(), slot->Value["was_redir"].ForceString());
}

void TMarketContext::SetRedirectCgiParams(const TRedirectCgiParams& params)
{
    NSc::TValue slotValue;
    slotValue["rs"] = params.ReportState;
    slotValue["was_redir"] = params.WasRedir;
    Ctx().CreateSlot(
        "redirect_cgi_params",
        "redirect_cgi_params",
        true, /* optional */
        slotValue);
}

void TMarketContext::ClearRedirectCgiParams()
{
    Ctx().CreateSlot(
        "redirect_cgi_params",
        "redirect_cgi_params",
        true, /* optional */
        NSc::TValue());
}

const NSc::TValue& TMarketContext::GetConfirmation() const
{
    const TSlot* const slot = Ctx().GetSlot("confirmation");
    if (IsSlotEmpty(slot)) {
        return null;
    }
    return slot->Value;
}

TGlFilters TMarketContext::GetGlFilters() const
{
    const TSlot* const slot = Ctx().GetSlot("gl_filters");
    if (IsSlotEmpty(slot)) {
        return TGlFilters();
    }

    TGlFilters filters;
    for (const auto& [id, rawFilter] : slot->Value.GetDict()) {
        if (rawFilter["type"] == "number") {
            const auto& min = rawFilter["min"];
            const auto& max = rawFilter["max"];
            filters.Add(TNumberGlFilter(
                id,
                rawFilter["name"].GetString(),
                min.IsNull() ? Nothing() : TMaybe<double>(min.GetNumber()),
                max.IsNull() ? Nothing() : TMaybe<double>(max.GetNumber()),
                rawFilter["unit"].GetString()
            ));
        } else if (rawFilter["type"] == "boolean") {
            filters.Add(TBoolGlFilter(id, rawFilter["name"].GetString(), rawFilter["value"].GetBool()));
        } else if (rawFilter["type"] == "enum") {
            TEnumGlFilter::TValues values;
            for (const auto& kv : rawFilter["values"].GetDict()) {
                values.Add(TEnumGlFilter::TValue(kv.first));
            }
            filters.Add(TEnumGlFilter(id, rawFilter["name"].GetString(), values));
        } else if (rawFilter["type"] == "raw") {
            TVector<TString> values;
            for (const auto& rawVal : rawFilter["values"].GetArray()) {
                values.push_back(rawVal.ForceString());
            }
            filters.Add(TRawGlFilter(id, values));
        } else {
            LOG(ERR) << "Got unknown gl filter type \"" << rawFilter["type"]
                       << "\". Full filter: " << rawFilter.ToJsonPretty() << Endl;
            Y_ASSERT(false);
        }
    }
    return filters;
}

NSc::TValue TMarketContext::GetRawGlFilters() const
{
    NSc::TValue rawFilters;
    for (const auto& [id, values] : GetCgiGlFilters()) {
        for (const auto& val : values) {
            rawFilters[id].Push(val);
        }
    }
    return rawFilters;
}

TCgiGlFilters TMarketContext::GetCgiGlFilters() const
{
    TCgiGlFilters cgiFilters;
    for (const auto& filter : GetGlFilters()) {
        TCgiValuesExtractor extractor(cgiFilters);
        std::visit(extractor, filter.second);
    }
    return cgiFilters;
}

void TMarketContext::ClearGlFilters()
{
    Ctx().CreateSlot("gl_filters", "gl_filters");
    Ctx().CreateSlot("gl_filters_description", "string");
}

void TMarketContext::ClearAllSlots()
{
    Ctx().CreateSlot(TStringBuf("region_id"), TStringBuf("region_id"));
    ClearGlFilters();
    ClearCategory();
    ClearModel();
    ClearPrice();
    ClearPopularGood();
    SetTextRedirect(TStringBuf());
    SetSuggestTextRedirect(TStringBuf());
    SetGalleryNumber(0);
}

void TMarketContext::ClearPreviousSearchSlots()
{
    Ctx().CreateSlot(TStringBuf("region_id"), TStringBuf("region_id"));
    ClearGlFilters();
    ClearCategory();
    ClearRedirectCgiParams();
    ClearFesh();
    SetSuggestTextRedirect(TStringBuf());
}

void TMarketContext::SetGlFilterDescription(const TStringBuf description)
{
    if (IsDebugMode()) {
        Ctx().CreateSlot("gl_filters_description", "string", true, description);
    }
}

void TMarketContext::AddGlFilter(const TGlFilter& filter)
{
    TGlFilterSerializer visitor(Ctx().GetOrCreateSlot("gl_filters", "gl_filters")->Value);
    std::visit(visitor, filter);
}

void TMarketContext::AddGlFilter(const TStringBuf id, const TVector<TString>& values)
{
    AddGlFilter(TRawGlFilter(id, values));
}

void TMarketContext::ClearGlFilter(const TStringBuf id)
{
    Ctx().GetOrCreateSlot("gl_filters", "gl_filters")->Value.Delete(id);
}

void TMarketContext::SetPrice(const NSc::TValue& from, const NSc::TValue& to)
{
    NSc::TValue price;
    price["from"] = from;
    price["to"] = to;
    Ctx().CreateSlot(
        "price",
        "price",
        true, /* optional */
        price
    );
}

bool TMarketContext::DoesPriceExist() const
{
    return !IsSlotEmpty(Ctx().GetSlot("price"));
}

const NSc::TValue& TMarketContext::GetPrice() const
{
    if (DoesPriceExist()) {
        return Ctx().GetSlot("price")->Value;
    }
    return null;
}

NSlots::TChoicePriceSchemeConst TMarketContext::GetPriceScheme() const
{
    const NSc::TValue& value = DoesPriceExist() ? Ctx().GetSlot("price")->Value : null;
    return NSlots::TChoicePriceSchemeConst(&value);
}

void TMarketContext::ClearPrice()
{
    Ctx().CreateSlot(
        "price",
        "price",
        true, /* optional */
        null
    );
}

void TMarketContext::SetFilterExamples(const TVector<NSc::TValue>& filterExamples)
{
    LOG(DEBUG) << "SetFiltersExamples filterExamples size" << filterExamples.size() << Endl;
    NSc::TValue slotValue = NSc::TValue().SetArray();
    for (const auto& example : filterExamples) {
        slotValue.Push(example);
    }
    Ctx().CreateSlot(
        "filter_examples",
        "filter_example",
        true, /* optional */
        slotValue
    );
}

void TMarketContext::ClearFilterExamples()
{
    Ctx().CreateSlot(
        "filter_examples",
        "filter_example",
        true, /* optional */
        NSc::TValue()
    );
}

void TMarketContext::SetFirstRequest(bool firstRequest)
{
    Ctx().CreateSlot(
        "is_first_request",
        "bool",
        true, /* optional */
        firstRequest
    );
}

bool TMarketContext::WereFilterExamplesShown() const
{
    const TSlot* const slot = Ctx().GetSlot("filter_examples_shown");
    return !IsSlotEmpty(slot) && slot->Value.GetBool();
}

void TMarketContext::SetFilterExamplesShown()
{
    Ctx().CreateSlot(
        "filter_examples_shown",
        "bool",
        true, /* optional */
        true
    );
}

TLazyValue<TCategory> TMarketContext::InitCategory()
{
    return [this](){
        const auto& category = GetJsonedCategory();
        return TCategory(
            category["hid"].GetIntNumber(),
            category["nid"].GetIntNumber(),
            category["slug"].GetString(),
            category["name"].GetString()
        );
    };
}

const TCategory& TMarketContext::GetCategory() const
{
    Y_ASSERT(DoesCategoryExist());

    return Category.GetRef();
}

const TVector<i64>& TMarketContext::GetFesh() const
{
    return Fesh;
}

void TMarketContext::ClearFesh()
{
    Fesh.clear();
}

NSc::TValue& TMarketContext::GetJsonedCategory() const
{
    return Ctx().GetSlot("category")->Value;
}

void TMarketContext::SetResponseForm(TStringBuf formName)
{
    TContext::TPtr newContextPtr = Ctx().SetResponseForm(formName, false);
    Y_ENSURE(newContextPtr);
    newContextPtr->CopySlotsFrom(Ctx(), {STATE_SLOT_NAME, MuidSlotName});
    CtxPtr = newContextPtr;
}

void TMarketContext::SetResponseFormAndCopySlots(TStringBuf formName, std::initializer_list<TStringBuf> slotNames)
{
    TContext::TPtr newContextPtr = Ctx().SetResponseForm(formName, false);
    Y_ENSURE(newContextPtr);
    newContextPtr->CopySlotsFrom(Ctx(), slotNames);
    newContextPtr->CopySlotsFrom(Ctx(), {STATE_SLOT_NAME, MuidSlotName});
    CtxPtr = newContextPtr;
}

void TMarketContext::AddMarketSuggests()
{
    NMarket::AddMarketSuggests(*this);
}

void TMarketContext::AddProductDetailsCardSuggests(bool isBlueOffer, bool hasShopUrl)
{
    NMarket::AddProductDetailsCardSuggests(*this, isBlueOffer, hasShopUrl);
}

void TMarketContext::AddBeruProductDetailsCardSuggest()
{
    NMarket::AddBeruProductDetailsCardSuggest(*this);
}

void TMarketContext::AddBeruOrderCardSuggests()
{
    NMarket::AddBeruOrderCardSuggests(*this);
}

void TMarketContext::ClearCategory()
{
    Ctx().CreateSlot(
        "category",
        "category",
        true, /* optional */
        NSc::TValue()
    );
    Category = InitCategory();
}

void TMarketContext::SetCategory(const TCategory& category)
{
    Ctx().CreateSlot(
        "category",
        "category",
        true, /* optional */
        CreateJsonedCategory(category)
    );
}

void TMarketContext::SetFesh(const TVector<i64>& fesh)
{
    Fesh = fesh;
}

void TMarketContext::AddTotal(i64 total)
{
    Ctx().GetOrCreateSlot("result", "result")->Value["total_count"] = total;
}

void TMarketContext::AddHowMuchTotal(i64 total)
{
    Ctx().GetOrCreateSlot("popular_good", "popular_good")->Value["total_count"] = total;
}

NSc::TValue TMarketContext::CreateJsonedCategory(const TCategory& category) const
{
    NSc::TValue val;
    val["hid"] = category.GetHid();
    val["nid"] = category.GetNid();
    val["slug"] = category.GetSlug();
    if (category.DoesNameExist()) {
        val["name"] = category.GetName();
    }
    return val;
}

double TMarketContext::GetAmount() const
{
    if (DoesAmountExist()) {
        return NormalizeAmount(Ctx().GetSlot("amount")->Value.GetString());
    }
    return 0;
}

double TMarketContext::GetAmountFrom() const
{
    if (DoesAmountFromExist()) {
        return NormalizeAmount(Ctx().GetSlot("amount_from")->Value.GetString());
    }
    return 0;
}

double TMarketContext::GetAmountTo() const
{
    if (DoesAmountToExist()) {
        return NormalizeAmount(Ctx().GetSlot("amount_to")->Value.GetString());
    }
    return 0;
}

const TStringBuf TMarketContext::GetUnit() const
{
    const auto slot = Ctx().GetSlot("unit");
    if (!IsSlotEmpty(slot)) {
        return slot->Value.GetString();
    }
    return TStringBuf("");
}

const TStringBuf TMarketContext::GetParameter() const
{
    const auto slot = Ctx().GetSlot("parameter");
    if (!IsSlotEmpty(slot)) {
        return slot->Value.GetString();
    }
    return TStringBuf("");
}

bool TMarketContext::DoesAmountExist() const
{
    return !IsSlotEmpty(Ctx().GetSlot("amount"));
}

bool TMarketContext::DoesAmountNeedRange() const
{
    return !IsSlotEmpty(Ctx().GetSlot("amount_need_range"));
}

bool TMarketContext::DoesAmountFromExist() const
{
    return !IsSlotEmpty(Ctx().GetSlot("amount_from"));
}

bool TMarketContext::DoesAmountToExist() const
{
    return !IsSlotEmpty(Ctx().GetSlot("amount_to"));
}

bool TMarketContext::DoesAnyAmountExist() const
{
    return DoesAmountExist() || DoesAmountFromExist() || DoesAmountToExist();
}

void TMarketContext::ClearAmounts()
{
    Ctx().CreateSlot("amount", "string");
    Ctx().CreateSlot("amount_from", "string");
    Ctx().CreateSlot("amount_to", "string");
}

bool TMarketContext::IsCalledDirectly() const
{
    return IsSlotEmpty(Ctx().GetSlot("called_from", "string"));
}

TStringBuf TMarketContext::GetCalledFrom() const
{
    const TSlot* slot = Ctx().GetSlot("called_from", "string");
    if (IsSlotEmpty(slot)) {
        return {};
    }
    return slot->Value.GetString();
}

void TMarketContext::SetCalledFrom(TStringBuf calledFrom) {
    Ctx().CreateSlot("called_from", "string", true, NSc::TValue().SetString(calledFrom));
}

bool TMarketContext::IsNative() const
{
    const TSlot* const isNativeSlot = Ctx().GetSlot("is_native_activation");
    return !IsSlotEmpty(isNativeSlot) && isNativeSlot->Value.GetBool();
}

void TMarketContext::SetNative(bool value)
{
    auto slot = Ctx().GetOrCreateSlot("is_native_activation", "bool");
    slot->Value = value;
}

bool TMarketContext::IsNativeOnMarket()
{
    return GetChoiceMarketType() == EMarketType::GREEN && HasChoiceMarketType() && GetExperiments().MarketNativeOnMarket();
}

bool TMarketContext::IsOpen() const
{
    return GetBoolSlot("is_open");
}

void TMarketContext::SetOpen(bool value)
{
    SetBoolSlot("is_open", value);
}

void TMarketContext::Log(const TStringBuf log)
{
    if (IsDebugMode()) {
        Ctx().GetOrCreateSlot(TStringBuf("logs"), TStringBuf("logs"))->Value.Push(NSc::TValue(log));
    }
}

TVector<int> TMarketContext::GetIndices() const
{
    constexpr std::array<const char*, 3> slotNames = {{"index1", "index2", "index3"}};
    TVector<int> indices(Reserve(slotNames.size()));
    for (auto& slotName : slotNames) {
        TSlot* slot = Ctx().GetSlot(slotName);
        if (!IsSlotEmpty(slot)) {
            indices.push_back(slot->Value.GetIntNumber() - 1);
        }
    }
    return indices;
}

// todo кажется достаточно использовать только слоты index1, index2, index3
TVector<int> TMarketContext::GetExclusiveIndices() const
{
    constexpr std::array<const char*, 3> slotNames = {{"exclusive_index1", "exclusive_index2", "exclusive_index3"}};
    TVector<int> indices(Reserve(slotNames.size()));
    for (auto& slotName : slotNames) {
        TSlot* slot = Ctx().GetSlot(slotName);
        if (!IsSlotEmpty(slot)) {
            indices.push_back(slot->Value.GetIntNumber() - 1);
        }
    }
    return indices;
}

void TMarketContext::SetVulgar()
{
    Ctx().AddTextCardBlock("how_much__vulgar_query_result");
}

void TMarketContext::SetCurrency(TStringBuf currency)
{
    Ctx().GetOrCreateSlot(TStringBuf("currency"), TStringBuf("string"))->Value.SetString(currency);
}

bool TMarketContext::HasProduct() const
{
    const auto& slot = Ctx().GetSlot("product");
    if (IsSlotEmpty(slot)) {
        return false;
    }
    return true;
}

NSlots::TProduct TMarketContext::GetProduct() const
{
    const auto& slot = Ctx().GetSlot("product");
    if (IsSlotEmpty(slot)) {
        Y_ASSERT(false);
        return NSlots::TProduct();
    }
    NSlots::TProduct product(slot->Value);
    Y_ASSERT(product->Validate());
    return product;
}

void TMarketContext::SetProduct(NSlots::TProduct product)
{
    Y_ASSERT(product->Validate());
    auto slot = Ctx().GetOrCreateSlot("product", "product");
    Y_ASSERT(slot);
    slot->Value = product.Value();
}

void TMarketContext::SetQueryInfo(const TQueryInfo& info)
{
    SetTextRedirect(info.Query);
    SetPrice(info.Price["from"], info.Price["to"]);
    ClearGlFilters();
    for (const auto& kv : info.GlFilters) {
        AddGlFilter(kv.first, kv.second);
    }
}

ui32 TMarketContext::GetAttempt() const
{
    TSlot* slot = Ctx().GetSlot(TStringBuf("attempt"));
    return slot ? slot->Value.GetIntNumber(0) : 0;
}

void TMarketContext::SetAttempt(ui32 value)
{
    Ctx().GetOrCreateSlot("attempt", "int")->Value.SetIntNumber(value);
}

void TMarketContext::SetAttemptReminder()
{
    Ctx().GetOrCreateSlot("attempt_reminder", "bool")->Value.SetBool(true);
}

void TMarketContext::SetGalleryNumber(ui32 galleryNumber)
{
    Ctx().CreateSlot(
        "gallery_number",
        "number",
        true, /* optional */
        galleryNumber
    );
}

ui32 TMarketContext::GetGalleryNumber() const
{
    const TSlot* const slot = Ctx().GetSlot(TStringBuf("gallery_number"));
    return slot ? slot->Value.GetIntNumber(0) : 0;
}

void TMarketContext::SetProductGalleryNumber(ui32 galleryNumber)
{
    Ctx().CreateSlot(
        "product_gallery_number",
        "number",
        true, /* optional */
        galleryNumber
    );
}

ui32 TMarketContext::GetProductGalleryNumber() const
{
    const TSlot* const slot = Ctx().GetSlot(TStringBuf("product_gallery_number"));
    return slot ? slot->Value.GetIntNumber(0) : 0;
}

ui32 TMarketContext::GetGalleryPosition() const
{
    const TSlot* const slot = Ctx().GetSlot(TStringBuf("gallery_position"));
    return slot ? slot->Value.GetIntNumber(0) : 0;
}

void TMarketContext::RenderHowMuchEmptySerp() {
    auto uri = GenerateSearchUri(&Ctx(), Utterance());
    Ctx().AddTextCardBlock("how_much__empty_serp");
    Ctx().AddSuggest("how_much__yandex_search", NSc::TValue(uri));
    AddOpenSerpSearchUriCommand(uri);
}

void TMarketContext::RenderEmptySerp() {
    auto uri = GenerateSearchUri(&Ctx(), Utterance());
    Ctx().AddTextCardBlock("market_common__empty_serp");
    Ctx().AddSuggest("market_common__yandex_search", NSc::TValue(uri));
    AddOpenSerpSearchUriCommand(uri);
}

void TMarketContext::RenderDebugInfo()
{
    Ctx().AddTextCardBlock("market__debug");
}

void TMarketContext::RenderHowMuchModel()
{
    if (Ctx().ClientFeatures().SupportsDivCards()) {
        Ctx().AddTextCardBlock("how_much__model");
        Ctx().AddDivCardBlock("market_model_offers", NSc::TValue());
    } else {
        Ctx().AddTextCardBlock("how_much__model__no_cards");
    }
}

THowMuchScenarioContextScheme TMarketContext::GetHowMuchScenarioCtx()
{
    NSc::TValue& slotValue = Ctx().GetOrCreateSlot(TStringBuf("scenario_ctx"), TStringBuf("scenario_ctx"))->Value;
    return THowMuchScenarioContextScheme(&slotValue);
}

void TMarketContext::AddBeruActivationSuggest(
    TStringBuf modelName,
    bool attachToCard,
    EReferer referer)
{
    NSc::TValue suggestData;
    suggestData["attach_to_card"].SetBool(attachToCard);
    TFormUpdate formUpdate;
    formUpdate->Resubmit() = true;

    AddSlotToFormUpdate(
        TStringBuf("choice_market_type"), TStringBuf("market_type"), ToString(EMarketType::BLUE), formUpdate);
    AddSlotToFormUpdate(TStringBuf("referer"), TStringBuf("referer"), ToString(referer), formUpdate);
    if (!modelName.empty()) {
        suggestData["beru_model_name"] = modelName;

        formUpdate->Name() = ToString(EChoiceForm::MarketChoice);
        AddSlotToFormUpdate(TStringBuf("request"), TStringBuf("string"), modelName, formUpdate);
        AddSlotToFormUpdate(TStringBuf("is_native_activation"), TStringBuf("bool"), true, formUpdate);
    } else {
        formUpdate->Name() = ToString(EChoiceForm::Activation);
    }
    Y_ASSERT(formUpdate->Validate());
    Ctx().AddSuggest(TStringBuf("market__beru_activation"), suggestData, formUpdate.Value());
}

void TMarketContext::RenderHowMuchPopularGoods(TStringBuf currentRequest, bool showBeruAdv)
{
    if (Ctx().ClientFeatures().SupportsDivCards()) {
        if (showBeruAdv) {
            TStringBuf modelName;
            if (IsCalledDirectly()) {
                modelName = currentRequest;
            }

            GetHowMuchScenarioCtx().FirstGalleryWasShown() = true;
            NSc::TValue cardData;
            cardData["beru_suggest"]["with_voice"] = GetExperiments().UseVoiceInBeruAdvInHowMuch();
            Ctx().AddTextCardBlock(TStringBuf("how_much__popular_goods"), cardData);

            AddBeruActivationSuggest(modelName, true /* attachToCard */, EReferer::UPPER_BERU_ADV_IN_HOW_MUCH);
            AddBeruActivationSuggest(modelName, false /* attachToCard */, EReferer::LOWER_BERU_ADV_IN_HOW_MUCH);
        } else {
            Ctx().AddTextCardBlock("how_much__popular_goods");
        }

        if (GetExperiments().HowMuchExtGallery()) {
            const auto &result = Ctx().GetOrCreateSlot("popular_good", "popular_good")->Value;
            TExtendedGalleryOpts opts {
                .CardAction = (
                    GetExperiments().HowMuchExtGalleryOpenMarket()
                    ? TExtendedGalleryOpts::EActionType::MARKET
                    : TExtendedGalleryOpts::EActionType::DETAILS_CARD),
                .DetailsCardButton =
                    GetExperiments().HowMuchExtGalleryAddButton()
                    ? TExtendedGalleryOpts::EDetailsCardButton::VIOLET
                    : TExtendedGalleryOpts::EDetailsCardButton::NONE,
                .RenderShopName = false,
                .EnableVoicePurchase = GetExperiments().Market(),
            };
            RenderExtendedGallery(
                result["results"].GetArray(),
                result["url"].GetString(),
                EMarketType::GREEN /* resultUrlType */,
                static_cast<ui64>(result["total_count"].GetIntNumber()),
                opts);
        } else {
            Ctx().AddDivCardBlock("market_popular_goods", NSc::TValue());
        }
    } else {
        Ctx().AddTextCardBlock("how_much__popular_goods__no_cards");
    }
}

void TMarketContext::RenderChoiceEmptyResult()
{
    Ctx().AddTextCardBlock("market__empty_result");
}

void TMarketContext::RenderChoiceAskContinueCustom(
    TStringBuf phrase,
    ui64 total,
    TStringBuf region,
    TStringBuf regionPrepcase)
{
    NSc::TValue data;
    data["phrase"] = phrase;
    data["total"] = total;
    data["region"] = region;
    data["region_prepcase"] = regionPrepcase;
    data["utterance"] = Utterance();
    Ctx().AddTextCardBlock(TStringBuf("market__ask_continue_custom"), data);
}

void TMarketContext::RenderChoiceAskContinue(ui64 total)
{
    NSc::TValue data;
    data["utterance"] = Utterance();
    data["total"] = total;
    data["market_type"] = ToString(GetChoiceMarketType());

    TString regionName;
    TString regionPrepcase;
    bool gotRegionNames = TryGetUserRegionNames(&regionName, &regionPrepcase);
    if (gotRegionNames) {
        data["region"]["name"] = regionName;
        data["region"]["prepcase"] = regionPrepcase;
    }

    Ctx().AddTextCardBlock(
        IsNative()
            ? IsOpen()
                ? "market__native_open_ask_continue"
                : "market__native_ask_continue"
            : "market__ask_continue_internal"
        , data);
}

void TMarketContext::RenderChoiceGallery(bool useExtendedGallery)
{
    if (useExtendedGallery) {
        const auto& resultData = Ctx().GetOrCreateSlot(TStringBuf("result"), TStringBuf("result"))->Value;
        RenderExtendedGallery(
            resultData["models"].GetArray(),
            resultData["url"].GetString(),
            GetChoiceMarketType(),
            static_cast<ui64>(resultData["total_count"].GetIntNumber()),
            {
                .CardAction = (
                    GetExperiments().ChoiceExtGalleryOpenMarket()
                    ? TExtendedGalleryOpts::EActionType::MARKET
                    : (
                        GetExperiments().GalleryOpenShop() // используется обратный флаг
                        ? TExtendedGalleryOpts::EActionType::SHOP
                        : TExtendedGalleryOpts::EActionType::DETAILS_CARD
                    )
                ),
                .ShopNameAction = (
                    GetExperiments().ChoiceExtGalleryOpenMarket()
                    ? TExtendedGalleryOpts::EActionType::MARKET
                    : TExtendedGalleryOpts::EActionType::SHOP
                ),
            });
    } else {
        Ctx().AddDivCardBlock("market_models", NSc::TValue());
    }
    if (Experiments.DisableListening()) {
        Ctx().AddStopListeningBlock();
    }
}

void TMarketContext::RenderExtendedGallery(
    const NSc::TArray& docs, TStringBuf resultUrl, EMarketType resultUrlType, ui64 totalDocs, TExtendedGalleryOpts opts)
{
    NSc::TValue data;
    data["docs"].AppendAll(docs);
    data["total"]["url"] = resultUrl;
    data["total"]["market_type"] = ToString(resultUrlType);
    data["total"]["count"] = totalDocs;
    for (const auto& item : docs) {
        if (item.Has("rating")) {
            opts.RenderRating = true;
        }
        if (item.Has("adviser_percentage")) {
            opts.RenderAdviserPercentage = true;
        }
        opts.ReasonsToBuyMaxSize = std::max(
            opts.ReasonsToBuyMaxSize,
            static_cast<ui8>(item["reasons_to_buy"].ArraySize()));
    }
    data["opts"] = opts.ToJson();
    if (Experiments.UseV3Gallery()) {
        Ctx().AddDivCardBlock("market_models_extended_v3", data);
    } else if (Experiments.UseV2Gallery()) {
        Ctx().AddDivCardBlock("market_models_extended_v2", data);
    } else {
        Ctx().AddDivCardBlock("market_models_extended", data);
    }
}

void TMarketContext::RenderChoiceProductOutdated()
{
    Ctx().AddTextCardBlock("market__product_outdated");
}

void TMarketContext::RenderChoiceBeruOfferOutdated()
{
    Ctx().AddTextCardBlock("market__beru_offer_outdated");
}

void TMarketContext::RenderChoiceBeruNoSku()
{
    Ctx().AddTextCardBlock("market__beru_no_sku");
}

void TMarketContext::RenderChoiceNoSkuForCart()
{
    Ctx().AddTextCardBlock(TStringBuf("market__no_sku_for_cart"));
}

void TMarketContext::RenderChoiceAddToCart()
{
    Ctx().AddTextCardBlock(TStringBuf("market__add_to_cart"));
}

void TMarketContext::OpenBundleUrl(TStringBuf url)
{
    TMarketUrlBuilder urlBuilder(*this);

    NSc::TValue data;
    data["uri"] = urlBuilder.GetClickUrl(url);
    Ctx().AddCommand<TMarketAddAndOpenCart>(TStringBuf("open_uri"), data);
}

void TMarketContext::RenderChoiceProductDetailsCard(const NSc::TValue& details)
{
    Ctx().AddDivCardBlock("market_product_details", details);
    Ctx().AddStopListeningBlock();
}

void TMarketContext::RenderChoiceProductOffersCard(const NSc::TValue& data, const TProductOffersCardOpts& opts)
{
    NSc::TValue cardData = data;
    cardData["opts"] = opts.ToJson();
    Ctx().AddDivCardBlock(TStringBuf("market_product_offers"), cardData);
    Ctx().AddStopListeningBlock();
}

void TMarketContext::RenderChoiceBeruProductDetailsCard(const NSc::TValue& details)
{
    Ctx().AddDivCardBlock("market_beru_product_details", details);
    Ctx().AddStopListeningBlock();
}

void TMarketContext::RenderChoiceBeruOrderCard(const TBeruOrderCardData& data)
{
    Ctx().AddDivCardBlock("market_beru_order", data.Value());
    Ctx().AddStopListeningBlock();
}

void TMarketContext::RenderChoiceSuggestGreenSearch(TStringBuf greenUrl, ui64 greenTotal)
{
    NSc::TValue phraseData;
    phraseData["total"] = greenTotal;
    Ctx().AddTextCardBlock(TStringBuf("market__green_search_suggest_because_blue_is_empty"), phraseData);

    NSc::TValue suggestData;
    suggestData["url"] = greenUrl;
    Ctx().AddSuggest(TStringBuf("market__green_search"), suggestData);
}

void TMarketContext::RenderActivationPhrase(bool freeDelivery) {
    NSc::TValue data;
    data["free_delivery"] = freeDelivery;
    Ctx().AddTextCardBlock(TStringBuf("market__activation"), data);
}

void TMarketContext::RenderNoActivationPhrase()
{
    Ctx().AddTextCardBlock(TStringBuf("market__no_activation"));
}

void TMarketContext::RenderCancel() {
    Ctx().AddTextCardBlock(TStringBuf("market__cancel"));
}

void TMarketContext::RenderChoiceGarbage() {
    Ctx().AddTextCardBlock(TStringBuf("market__garbage"));
}

void TMarketContext::RenderChoiceStartChoiceAgain(bool isClosed) {
    NSc::TValue data;
    data["is_closed"] = isClosed;
    Ctx().AddTextCardBlock(TStringBuf("market__start_choice_again"), data);
}

void TMarketContext::RenderChoiceAttemptsLimit() {
    Ctx().AddTextCardBlock(TStringBuf("market__attempts_limit"));
}

void TMarketContext::RenderMarketNotSupportedInLocation(EMarketType rgb, TStringBuf geoLocation) {
    NSc::TValue data;
    data["geo_location"] = geoLocation;
    data["rgb"] = ToString(rgb);
    Ctx().AddTextCardBlock(TStringBuf("market_common__market_not_supported_in_location"), data);
}

void TMarketContext::RenderYandexSearch() {
    if (CanOpenUri()) {
        auto uri = GenerateSearchUri(&Ctx(), Utterance());
        Ctx().AddTextCardBlock(TStringBuf("market_common__yandex_search"));
        Ctx().AddSuggest(TStringBuf("market_common__yandex_search"), NSc::TValue(uri));
        AddOpenSerpSearchUriCommand(uri);
    } else {
        Ctx().AddTextCardBlock(TStringBuf("market_common__yandex_search_not_supported"));
    }
}

void TMarketContext::AddOpenSerpSearchUriCommand(TStringBuf uri) {
    // автоматически открывать в поиске uri
    NSc::TValue data;
    data["uri"] = uri;
    Ctx().AddCommand<TMarketOpenSerpSearchDirective>(TStringBuf("open_uri"), data);
}

NSc::TValue TMarketContext::GetProductDetailsFormUpdate(
    const NSlots::TProduct& product,
    ui32 galleryNumber,
    ui32 galleryPosition) const
{
    TFormUpdate formUpdate;
    formUpdate->Resubmit() = true;
    TString formName;
    TString stateName;
    switch(ScenarioType) {
        case EScenarioType::CHOICE:
            formName = ToString(EChoiceForm::ProductDetails);
            switch (GetChoiceMarketType()) {
                case EMarketType::BLUE:
                    stateName = ToString(IsOpen() ? EChoiceState::BeruProductDetailsOpen : EChoiceState::BeruProductDetails);
                    break;
                case EMarketType::GREEN:
                    stateName = ToString(IsOpen() ? EChoiceState::ProductDetailsOpen : EChoiceState::ProductDetails);
                    break;
            }
            if (IsOpen()) {
                AddSlotToFormUpdate(TStringBuf("is_open"), TStringBuf("bool"), true, formUpdate);
            }
            break;
        case EScenarioType::RECURRING_PURCHASE:
            formName = ToString(ERecurringPurchaseForm::ProductDetails);
            stateName = ToString(ERecurringPurchaseState::ProductDetails);
            break;
        case EScenarioType::HOW_MUCH:
            formName = ToString(EChoiceForm::ProductDetailsExternal);
            AddSlotToFormUpdate(
                TStringBuf("from_alice"),
                TStringBuf("bool"),
                true,
                formUpdate);
            AddSlotToFormUpdate(TStringBuf("is_open"), TStringBuf("bool"), true, formUpdate);
            break;
        default:
            ythrow TMarketException(TStringBuf("Unexpected scenario"));
    }
    formUpdate->Name() = formName;

    AddSlotToFormUpdate(TStringBuf("market_clid"), TStringBuf("clid_type"), ToString(GetMarketClid()), formUpdate);
    AddSlotToFormUpdate(TStringBuf("product"), TStringBuf("product"), product.Value(), formUpdate);
    AddSlotToFormUpdate(
        TStringBuf("product_market_type"), TStringBuf("market_type"), ToString(GetChoiceMarketType()), formUpdate);
    if (product->Type() == TStringBuf("sku")) {
        AddSlotToFormUpdate(TStringBuf("sku"), TStringBuf("number"), product->Id(), formUpdate);
    }
    if (!stateName.empty()) {
        AddStateValueToFormUpdate(stateName, formUpdate);
    }
    AddSlotToFormUpdate(TStringBuf("product_gallery_number"), TStringBuf("number"), galleryNumber, formUpdate);
    AddSlotToFormUpdate(TStringBuf("gallery_position"), TStringBuf("number"), galleryPosition, formUpdate);

    Y_ASSERT(formUpdate->Validate());
    return formUpdate.Value();
}

TFormUpdate TMarketContext::GetBeruOrderFormUpdate(ui64 sku, ui32 galleryNumber, ui32 galleryPosition) const
{
    if (ScenarioType != EScenarioType::CHOICE) {
        ythrow TMarketException(TStringBuf("Unexpected scenario"));
    }
    auto formUpdate = CreateSkuFormUpdate(sku, ToString(EChoiceForm::BeruOrder));

    AddStateToFormUpdate(IsOpen() ? EChoiceState::MakeOrderOpen : EChoiceState::MakeOrder, formUpdate);
    AddSlotToFormUpdate(TStringBuf("product_gallery_number"), TStringBuf("number"), galleryNumber, formUpdate);
    AddSlotToFormUpdate(TStringBuf("gallery_position"), TStringBuf("number"), galleryPosition, formUpdate);

    return formUpdate;
}

TFormUpdate TMarketContext::GetCheckoutFormUpdate(ui64 sku) const
{
    TString formName;
    switch(ScenarioType) {
        case EScenarioType::CHOICE:
            formName = ToString(EChoiceForm::Checkout);
            break;
        case EScenarioType::RECURRING_PURCHASE:
            formName = ToString(ERecurringPurchaseForm::Checkout);
            break;
        default:
            ythrow TMarketException(TStringBuf("Unexpected scenario"));
    }
    // Не добавляем state в форму, потому что при переходе в market__checkout мы всё равно не импортируем market__state.
    TFormUpdate formUpdate = CreateSkuFormUpdate(sku, formName);
    return formUpdate;
}

TFormUpdate TMarketContext::GetAddToCartFormUpdate(ui64 sku) const
{
    auto formUpdate = CreateSkuFormUpdate(sku, ToString(EChoiceForm::AddToCart));
    AddStateToFormUpdate(IsOpen() ? EChoiceState::ChoiceOpen : EChoiceState::Choice, formUpdate);
    return formUpdate;
}

TFormUpdate TMarketContext::CreateSkuFormUpdate(ui64 sku, TStringBuf formName)
{
    TFormUpdate formUpdate;
    formUpdate->Resubmit() = true;
    formUpdate->Name() = formName;
    AddSlotToFormUpdate(TStringBuf("sku"), TStringBuf("number"), sku, formUpdate);
    Y_ASSERT(formUpdate->Validate());
    return formUpdate;
}

TResultValue TMarketContext::RunSearchResponse()
{
    if (Ctx().HasExpFlag("market_context_disable_search_change_form")) {
        RenderChoiceEmptyResult();
        return TResultValue();
    }
    TContext::TPtr newCtx = TSearchFormHandler::SetAsResponse(Ctx(), false);
    if (newCtx) {
        return Ctx().RunResponseFormHandler();
    }
    return TError(TError::EType::MARKETERROR, TStringBuf("can't create search response"));
}

TContext::TBlock* TMarketContext::AddSuggest(TStringBuf type, NSc::TValue data, NSc::TValue formUpdate)
{
    return Ctx().AddSuggest(type, data, formUpdate);
}

NSc::TValue TMarketContext::DeleteSuggest(TStringBuf type)
{
    return Ctx().DeleteSuggest(type);
}

void TMarketContext::AddSearchSuggest(TStringBuf query)
{
    Ctx().AddSearchSuggest(query);
}

bool TMarketContext::AddAuthorizationSuggest()
{
    TString uri = GenerateAuthorizationUri(Ctx());
    if (uri.Empty()) {
        return false;
    }
    NSc::TValue data;
    data["uri"] = uri;
    data["attach_to_card"].SetBool(true);
    Ctx().AddSuggest(TStringBuf("market__authorization"), std::move(data));
    return true;
}

void TMarketContext::AddOpenBlueSuggest()
{
    NSc::TValue data;
    data["url"] = TMarketUrlBuilder().GetBeruUrl();
    data["attach_to_card"].SetBool(true);
    Ctx().AddSuggest(TStringBuf("market__open_blue"), data);
}

void TMarketContext::AddOnboardingSuggest()
{
    Ctx().AddOnboardingSuggest();
}

TContext::TBlock* TMarketContext::AddTextCardBlock(TStringBuf id, NSc::TValue data)
{
    return Ctx().AddTextCardBlock(id, data);
}

TContext::TBlock* TMarketContext::AddDivCardBlock(TStringBuf id, NSc::TValue data)
{
    return Ctx().AddDivCardBlock(id, data);
}

void TMarketContext::AddDebugTraces()
{
    NSc::TValue data;
    data["caption"] = TStringBuf("ЦУМ Trace");
    data["url"] = TStringBuilder() << TStringBuf("https://tsum.yandex-team.ru/trace/") << BaseMarketRequestId();
    data["attach_to_card"].SetBool(true);
    Ctx().AddSuggest("market__debug__trace", data);
}

} // namespace NMarket

} // namespace NBASS

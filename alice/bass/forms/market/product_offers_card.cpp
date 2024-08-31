#include "product_offers_card.h"

#include "delivery_builder.h"
#include "market_url_builder.h"
#include "settings.h"
#include "types/picture.h"
#include "util/serialize.h"
#include "util/suggests.h"

namespace NBASS {

namespace NMarket {

namespace {

constexpr ui64 BERU_FESH_ID = 431782;

bool IsColorFilter(TReportFilterSchemeConst filter)
{
    return filter.XslName() == TStringBuf("color_vendor");
}

TVector<TStringBuf> GetColorCodes(TReportFilterSchemeConst colorFilter)
{
    TVector<TStringBuf> colors;
    for (auto value : colorFilter.Values()) {
        if (value.HasCode()) {
            colors.push_back(value.Code());
        }
    }
    return colors;
}

template <class TOutputOffer>
void FillOfferBaseFields(
    const TMarketContext& ctx,
    const TReportDocumentSchemeConst& offer,
    bool fillColors,
    TOutputOffer& resultOffer)
{
    resultOffer.Shop().Rating() = offer.Shop().QualityRating();
    resultOffer.Shop().RatingCount() = offer.Shop().OverallGradesCount();
    resultOffer.Price().Value() = offer.Prices().Value();
    resultOffer.Price().Currency() = offer.Prices().Currency();
    if (offer.Prices().HasDiscount()) {
        resultOffer.PriceBeforeDiscount().Value() = offer.Prices().Discount().OldMin();
        resultOffer.PriceBeforeDiscount().Currency() = offer.Prices().Currency();
    }

    if (fillColors) {
        for (const auto& filter : offer.Filters()) {
            if (IsColorFilter(filter)) {
                if (!filter.Values().Empty()) {
                    resultOffer.Color() = filter.Values(0).Code();
                }
            }
        }
    }
    resultOffer.Urls().Market() = TMarketUrlBuilder(ctx).GetMarketOfferUrl(
        offer->WareId(),
        offer->Cpc(),
        ctx.UserRegion(),
        ctx.GetProductGalleryNumber(),
        ctx.GetGalleryPosition());
}

} // namespace anonymous

TProductOffersCardHandler::TProductOffersCardHandler(TMarketContext& ctx)
    : Ctx(ctx)
    , ReportClient(ctx)
    , EnableVoicePurchase(ctx.GetExperiments().Market())
{
}

TResultValue TProductOffersCardHandler::HandleProductOutdated()
{
    Ctx.SetState(Ctx.IsOpen() ? EChoiceState::ChoiceOpen : EChoiceState::Choice);
    Ctx.RenderChoiceProductOutdated();
    return TResultValue();
}

template <class TIconMap>
void TProductOffersCardHandler::SetRatingIcons(TIconMap result)
{
    static TStringBuf iconNames[] = {
        TStringBuf("Fill"),
        TStringBuf("Half"),
        TStringBuf("None")
    };

    for (const auto iconName : iconNames) {
        const auto iconUrl = Ctx.GetAvatarPictureUrl(TStringBuf("poi"), iconName);
        if (iconUrl) {
            result[iconName] = *iconUrl;
        }
    }
}

bool TProductOffersCardHandler::TryFillBlueOffer(
    const TReportSearchResponse& blueOfferResponse,
    bool fillColors,
    TProductOffersCardData& cardData)
{
    if (!blueOfferResponse->Search().Results().Empty()) {
        const auto& offer = blueOfferResponse->Search().Results(0);
        const ui64 sku = FromString<ui64>(offer.MarketSku().Get());
        auto resultOffer = cardData->Offers().Beru();
        FillOfferBaseFields(Ctx, offer, fillColors, resultOffer);
        TDeliveryBuilder::FillBlueDelivery(offer, resultOffer.Delivery());

        // Иногда репорт возвращает не beru.ru в поле shop, а магазин поставщик.
        // Нужно либо поправить репорт (MARKETOUT-26226), либо запрашивать shop_info, чтоб получить рейтинг.
        // Пока в таком случае просто не будем отображать рейтинг и отзывы beru.ru
        if (offer.Shop().Id() != BERU_FESH_ID) {
            resultOffer.Shop().Clear();
        }

        TMarketUrlBuilder urlBuilder(Ctx);
        resultOffer.Urls().TermsOfUse() = urlBuilder.GetBeruTermsOfUseUrl();
        resultOffer.Urls().Supplier() = urlBuilder.GetBeruSupplierUrl(offer.WareId());
        resultOffer.Urls().Model() = urlBuilder.GetBeruModelUrl(
            offer.MarketSku(),
            offer.Slug(),
            Ctx.UserRegion(),
            Ctx.GetProductGalleryNumber(),
            Ctx.GetGalleryPosition());

        resultOffer.BeruOrder().FormUpdate() = Ctx.GetCheckoutFormUpdate(sku).Scheme();
        if (Ctx.GetExperiments().AddToCart()) {
            resultOffer.AddToCart().FormUpdate() = Ctx.GetAddToCartFormUpdate(sku).Scheme();
            AddCartSuggest(Ctx, sku);
        }
        if (EnableVoicePurchase) {
            Ctx.AddSuggest(TStringBuf("market__product_details__order_witch_alice"));
            Ctx.SetBeruOrderSku(sku);
        }
        return true;
    }
    return false;
}

bool TProductOffersCardHandler::TryFillDefaultOffer(
    const TReportSearchResponse& offerResponse,
    bool fillColors,
    TProductOffersCardData& cardData)
{
    if (offerResponse.Documents().empty()) {
        return false;
    }
    const auto& offer = offerResponse.Documents()[0];
    if (offer->HasMarketSku()) {
        return false;
    }
    auto resultOffer = cardData->Offers().Other().Add();
    FillOfferBaseFields(Ctx, offer.Scheme(), fillColors, resultOffer);
    TDeliveryBuilder::TryFillWhiteDelivery(offer.Scheme(), resultOffer.Delivery());
    resultOffer.Shop().Name() = offer->Shop().Name();
    resultOffer.Urls().Shop() = TMarketUrlBuilder(Ctx).GetClickUrl(offer->Urls().Encrypted());
    return true;
}

void TProductOffersCardHandler::FillFieldsFromProductOffers(
    const TReportSearchResponse& offersResponse,
    TMaybe<TStringBuf> wareId,
    size_t count,
    bool fillColors,
    TProductOffersCardData& result)
{
    auto resultOffers = result->Offers().Other();
    for (const auto& offer : offersResponse->Search().Results()) {
        if (resultOffers.Size() >= count) {
            break;
        }
        if (offer.Entity() == TStringBuf("regionalDelimiter")) {
            continue;
        }
        if (offer.HasMarketSku()) {
            continue;
        }
        if (wareId.Defined() && offer.WareId() == wareId.GetRef()) {
            continue;
        }
        auto resultOffer = resultOffers.Add();
        FillOfferBaseFields(Ctx, offer, fillColors, resultOffer);
        TDeliveryBuilder::TryFillWhiteDelivery(offer, resultOffer.Delivery());
        resultOffer.Shop().Name() = offer.Shop().Name();
        resultOffer.Urls().Shop() = TMarketUrlBuilder(Ctx).GetClickUrl(offer.Urls().Encrypted());
    }

    result->Offers().Total().Count() = offersResponse->Search()->Total();
}


TResultValue TProductOffersCardHandler::Do(
    TModelId modelId,
    TMaybe<TStringBuf> wareId,
    const TCgiGlFilters& glFilters,
    NSlots::TChoicePriceSchemeConst price,
    TMaybe<ui64> hidMaybe,
    const TRedirectCgiParams& redirectParams,
    bool increaseGalleryNumber,
    TStringBuf textCardName,
    NSc::TValue textCardData)
{
    const NSc::TValue& priceValue = *price.GetRawValue();
    auto modelInfoHandle = ReportClient.GetModelInfoAsync(modelId, glFilters, redirectParams, priceValue, hidMaybe);
    auto offersHandle = ReportClient.GetProductOffersAsync(modelId, glFilters, priceValue, hidMaybe);
    auto blueOfferHandle = Ctx.GetExperiments().ProductOffersCardOpenMarket()
        ? ReportClient.GetDefaultOfferAsync(modelId, glFilters, EMarketType::GREEN, priceValue, hidMaybe, BERU_FESH_ID)
        : ReportClient.GetDefaultOfferAsync(modelId, glFilters, EMarketType::BLUE, priceValue, hidMaybe);
    const auto wareResponse = wareId.Defined() ? MakeMaybe(ReportClient.GetOfferAsync(wareId.GetRef()).Wait().GetResponse()) : Nothing();
    const auto& modelInfoResponse = modelInfoHandle.Wait().GetResponse();
    const auto& offersResponse = offersHandle.Wait().GetResponse();
    const auto& blueOfferResponse = blueOfferHandle.Wait().GetResponse();

    if (modelInfoResponse->Search().Results().Empty()
        || (blueOfferResponse->Search().Results().Empty()
            && offersResponse->Search().Results().Empty()
            && (wareResponse.Empty() || wareResponse.GetRef()->Search().Results().Empty())))
    {
        return HandleProductOutdated();
    }

    if (modelInfoResponse.Documents().empty()) {
        return HandleProductOutdated();
    }
    const TReportDocument& model = modelInfoResponse.Documents()[0];
    if (model.Type() != TReportDocument::EType::MODEL) {
        Y_ASSERT(false);
        LOG(ERR) << "Unexpected market report behaviour. "
                 << "Expected model document on modelinfo request."
                 << "Got \"" << model->Entity() << Endl;
        return HandleProductOutdated();
    }

    for (const auto& warning : model.Model().GetWarnings()) {
        if (warning.IsIllegal()) {
            return HandleProductOutdated();
        }
    }
    if (increaseGalleryNumber) {
        Ctx.SetGalleryNumber(Ctx.GetGalleryNumber() + 1);
        Ctx.SetProductGalleryNumber(Ctx.GetGalleryNumber());
    }

    TProductOffersCardData result;

    // fill model info
    TMarketUrlBuilder urlBuilder(Ctx);
    result->Title() = model->Titles().Raw();
    if (model->Prices().HasMin()) {
        result->Prices().Min() = model->Prices().Min();
    }
    if (model->Prices().HasMax()) {
        result->Prices().Max() = model->Prices().Max();
    }
    if (model->Prices().HasAvg()) {
        result->Prices().Avg() = model->Prices().Avg();
    }
    result->Prices().Currency() = model->Prices().Currency();
    if (model.Model().HasRating()) {
        result->RatingIcon() = urlBuilder.GetRatingIconUrl(model.Model().GetRating());
    }
    if (model.Model().HasReviewCount()) {
        result->ReviewCount() = model.Model().GetReviewCount();
    }
    if (model.Model().HasAdviserPercentage()) {
        result->AdviserPercentage() = model.Model().GetAdviserPercentage();
    }
    if (!model.Model().GetAdvantages().empty()) {
        for (TStringBuf advantage : model.Model().GetAdvantages()) {
            result->ReasonsToBuy().Add() = advantage;
        }
    }

    const auto wareOffer = wareResponse.Defined() && !wareResponse->Documents().empty()
        ? MakeMaybe(wareResponse->Documents()[0])
        : Nothing();
    if (wareOffer.Defined()) {
        SerializePicture(wareOffer->Offer().GetPicture(), result->Picture());
    } else {
        SerializePicture(model.Model().GetPicture(), result->Picture());
    }

    auto getModelUrl = [&](TMarketUrlBuilder::EMarketModelTab tab) {
        return urlBuilder.GetMarketModelUrl(
            modelId,
            modelInfoResponse->Search().Results(0).Slug(),
            Ctx.UserRegion(),
            Ctx.GetProductGalleryNumber(),
            Ctx.GetGalleryPosition(),
            glFilters,
            price.From(),
            price.To(),
            redirectParams,
            tab);
    };
    result->Urls().Model() = getModelUrl(TMarketUrlBuilder::EMarketModelTab::Main);
    result->Urls().Reviews() = getModelUrl(TMarketUrlBuilder::EMarketModelTab::Reviews);
    result->Offers().Total().Url() = getModelUrl(TMarketUrlBuilder::EMarketModelTab::Offers);
    SerializeWarnings(model.Model().GetWarnings(), result->Warnings());

    // fill colors
    THashSet<TStringBuf> colors;
    for (const auto& filter : offersResponse->Filters()) {
        if (IsColorFilter(filter)) {
            auto codes = GetColorCodes(filter);
            for (TStringBuf code : codes) {
                colors.insert(code);
            }
        }
    }
    if (wareOffer.Defined()) {
        for (const auto& filter : wareOffer.GetRef()->Filters()) {
            if (IsColorFilter(filter)) {
                auto codes = GetColorCodes(filter);
                for (TStringBuf code : codes) {
                    colors.insert(code);
                }
            }
        }
    }
    const bool setOfferColors = colors.size() > 1;
    if (setOfferColors) {
        for (const auto& code : colors) {
            result->Colors().Add() = code;
        }
    }

    // fill offers
    bool hasBlueOffer = TryFillBlueOffer(blueOfferResponse, setOfferColors, result);
    if (wareResponse.Defined()) {
        TryFillDefaultOffer(wareResponse.GetRef(), setOfferColors, result);
    }
    const size_t otherOffersMaxSize = MAX_OFFERS_IN_OFFERS_CARD - hasBlueOffer;
    FillFieldsFromProductOffers(offersResponse, wareId, otherOffersMaxSize, setOfferColors, result);

    SetRatingIcons(result->RatingIcons());

    if (!textCardName.empty()) {
        Ctx.AddTextCardBlock(textCardName, textCardData);
    }
    Ctx.RenderChoiceProductOffersCard(
        result.Value(),
        TProductOffersCardOpts {
            .EnableVoicePurchase = EnableVoicePurchase,
            .OfferAction = Ctx.GetExperiments().ProductOffersCardOpenMarket()
                ? TProductOffersCardOpts::EOfferActionType::MARKET
                : TProductOffersCardOpts::EOfferActionType::SHOP,
        });
    return TResultValue();
}

} // namespace NMarket

} // namespace NBASS

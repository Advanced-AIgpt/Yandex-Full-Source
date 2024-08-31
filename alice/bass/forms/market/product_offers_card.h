#pragma once

#include "client/report_client.h"
#include "types.h"

namespace NBASS {

namespace NMarket {

class TProductOffersCardHandler {
    using TProductOffersCardData = TSchemeHolder<NBassApi::TProductOffersCardData<TBoolSchemeTraits>>;

public:
    TProductOffersCardHandler(TMarketContext& ctx);

    TResultValue Do(
        TModelId modelId,
        TMaybe<TStringBuf> wareId,
        const TCgiGlFilters& glFilters,
        NSlots::TChoicePriceSchemeConst price,
        TMaybe<ui64> hidMaybe,
        const TRedirectCgiParams& redirectParams = TRedirectCgiParams(),
        bool increaseGalleryNumber = false,
        TStringBuf textCardName = {},
        NSc::TValue textCardData = {});

private:
    TMarketContext& Ctx;
    TReportClient ReportClient;
    bool EnableVoicePurchase;

    TResultValue HandleProductOutdated();
    template <class TIconMap>
    void SetRatingIcons(TIconMap result);
    bool TryFillBlueOffer(
        const TReportSearchResponse& blueOfferResponse,
        bool fillColors,
        TProductOffersCardData& cardData);
    bool TryFillDefaultOffer(
        const TReportSearchResponse& defaultOfferResponse,
        bool fillColors,
        TProductOffersCardData& cardData);
    void FillFieldsFromProductOffers(
        const TReportSearchResponse& offersResponse,
        TMaybe<TStringBuf> wareId,
        size_t count,
        bool fillColors,
        TProductOffersCardData& result);
};

} // namespace NMarket

} // namespace NBASS

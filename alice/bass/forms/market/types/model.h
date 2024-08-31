#pragma once

#include "offer.h"
#include "picture.h"
#include "warning.h"

#include <alice/bass/forms/market/types.h>
#include <library/cpp/scheme/scheme.h>

namespace NBASS {

namespace NMarket {

class TModel {
public:
    TModel(const NSc::TValue& data);

    TModelId GetId() const { return Id; }
    TString GetSlug() const { return Slug; }
    const TString& GetTitle() const { return Title; }
    const TPicture& GetPicture() const { return Picture; }
    const TMaybe<TOffer>& GetDefaultOffer() const { return DefaultOffer; }
    TPrice GetMinPrice() const { return MinPrice; }
    TPrice GetAvgPrice() const { return AvgPrice; }
    bool HasAvgPrice() const { return AvgPrice != UNDEFINED_PRICE_VALUE; }
    TPrice GetDefaultPrice() const { return DefaultPrice; }
    bool HasDefaultPrice() const { return DefaultPrice != UNDEFINED_PRICE_VALUE; }
    TPrice GetDefaultPriceBeforeDiscount() const { return DefaultPriceBeforeDiscount; }
    bool HasDefaultPriceBeforeDiscount() const { return DefaultPriceBeforeDiscount != UNDEFINED_PRICE_VALUE; }
    TStringBuf GetCurrency() const { return Currency; }
    const TVector<TWarning>& GetWarnings() const { return Warnings; }
    const TCgiGlFilters& GetGlFilters() const { return GlFilters; }
    bool HasRating() const { return Rating.Defined(); }
    float GetRating() const { return Rating.GetOrElse(0); }
    bool HasReviewCount() const { return ReviewCount.Defined(); }
    ui32 GetReviewCount() const { return ReviewCount.GetOrElse(0); }
    bool HasAdviserPercentage() const { return AdviserPercentage.Defined(); }
    ui8 GetAdviserPercentage() const { return AdviserPercentage.GetOrElse(0); }
    const TVector<TString>& GetAdvantages() const { return Advantages; }

private:
    TModelId Id;
    TString Slug;
    TString Title;
    TCgiGlFilters GlFilters;
    TPicture Picture;
    TMaybe<TOffer> DefaultOffer;
    TPrice MinPrice;
    TPrice AvgPrice;
    TPrice DefaultPrice;
    TPrice DefaultPriceBeforeDiscount;
    TString Currency;
    TVector<TWarning> Warnings;
    TMaybe<float> Rating;
    TMaybe<ui32> ReviewCount;
    TMaybe<ui8> AdviserPercentage;
    TVector<TString> Advantages;

private:
    static TPrice GetModelDefaultPrice(const NSc::TValue& data);
    static TPrice GetModelDefaultPriceBeforeDiscount(const NSc::TValue& data);
    static TMaybe<TOffer> GetDefaultOffer(const NSc::TValue& data);
    static TVector<TWarning> GetWarnings(const NSc::TValue& data, TMaybe<TOffer>& defaultOffer);
    static TCgiGlFilters CreateGlFilters(const NSc::TValue& data);
    static TMaybe<ui32> GetReviewCount(const NSc::TValue& data);
    static TMaybe<ui8> GetAdviserPercentage(const NSc::TValue& data);
    static TVector<TString> GetAdvantages(const NSc::TValue& data);
};

} // namespace NMarket

} // namespace NBASS

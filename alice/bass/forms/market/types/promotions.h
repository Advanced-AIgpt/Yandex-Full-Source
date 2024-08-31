#pragma once

#include <alice/bass/forms/market/client/mds.sc.h>
#include <alice/bass/forms/market/client/bool_scheme_traits.h>

#include <util/datetime/base.h>
#include <util/generic/fwd.h>
#include <util/generic/maybe.h>

namespace NBASS {

namespace NMarket {

struct TTimeInterval {
    using TSchemeConst = NBassApi::TTimeIntervalConst<TBoolSchemeTraits>;

    TInstant From;
    TInstant To;

    static TMaybe<TTimeInterval> FromScheme(TSchemeConst scheme);

    static TMaybe<TTimeInterval> FromJson(const NSc::TValue& data) {
        return FromScheme(TSchemeConst(&data));
    }
};

struct TPromotions {
    using TSchemeConst = NBassApi::TPromotionsConst<TBoolSchemeTraits>;

    struct TVendorFreeDelivery {
        TString Description;
        TTimeInterval Interval;
    };

    class TSloboda {
    public:
        using TProductFacts = TVector<TString>;
        using TSchemeConst = NBassApi::TSlobodaConst<TBoolSchemeTraits>;

        TSloboda() : Facts(), FactsByProductName()
        {
        }
        void Update(TSchemeConst dataScheme);
        const TProductFacts* GetFacts(TStringBuf product) const;
        TVector<TStringBuf> GetFactNames() const;

    private:

        TVector<TProductFacts> Facts;
        THashMap<TString, size_t> FactsByProductName;
    };

    TMaybe<TTimeInterval> FreeDeliveryInterval;
    THashMap<ui32, TVendorFreeDelivery> FreeDeliveryByVendor;
    TSloboda Sloboda;

    static TPromotions FromJson(const NSc::TValue& data);
};

} // namespace NMarket

} // namespace NBASS

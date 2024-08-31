#include "promotions.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <util/charset/utf8.h>

namespace NBASS {

namespace NMarket {

TMaybe<TTimeInterval> TTimeInterval::FromScheme(TSchemeConst scheme)
{
    TTimeInterval result;
    if (!TInstant::TryParseIso8601(scheme.From(), result.From)
        || !TInstant::TryParseIso8601(scheme.To(), result.To))
    {
        return Nothing();
    }
    return result;
}

void TPromotions::TSloboda::Update(TSchemeConst dataScheme)
{
    for (const auto productFacts : dataScheme.Facts()) {
        if (productFacts.Texts().Empty()) {
            LOG(ERR) << "Got empty sloboda facts list" << Endl;
            continue;
        }
        TProductFacts& newFacts = Facts.emplace_back(Reserve(productFacts.Texts().Size()));
        for (const auto& fact : productFacts.Texts()) {
            newFacts.push_back(ToString(fact));
        }

        size_t factsIdx = Facts.size() - 1;
        for (const auto& key : productFacts.KeyWords()) {
            FactsByProductName[ToLowerUTF8(key)] = factsIdx;
        }
    }
}

const TPromotions::TSloboda::TProductFacts* TPromotions::TSloboda::GetFacts(TStringBuf product) const
{
    TString key = ToLowerUTF8(product);
    if (!FactsByProductName.contains(key)) {
        return nullptr;
    }
    return &Facts[FactsByProductName.at(key)];
}

TVector<TStringBuf> TPromotions::TSloboda::GetFactNames() const
{
    TVector<TStringBuf> result;
    for (const auto& it : FactsByProductName) {
        result.push_back(it.first);
    }
    return result;
}

TPromotions TPromotions::FromJson(const NSc::TValue& data)
{
    TSchemeConst dataScheme(&data);
    TPromotions result;

    result.FreeDeliveryInterval = TTimeInterval::FromScheme(dataScheme.FreeDeliveryInterval());
    result.Sloboda.Update(dataScheme.Sloboda());

    for (auto promoKv : dataScheme.FreeDeliveryByVendor()) {
        if (auto intervalMaybe = TTimeInterval::FromScheme(promoKv.Value().Interval())) {
            result.FreeDeliveryByVendor[promoKv.Key()] = TVendorFreeDelivery{
                .Description = ToString(promoKv.Value().Description()),
                .Interval = intervalMaybe.GetRef(),
            };
        } else {
            LOG(WARNING)
            << "Cannot parse delivery intervals for vendor free delivery promo: "
            << "vendor_id=" << promoKv.Key() << " "
            << "promo data=" << promoKv.Value().GetRawValue()->ToJsonPretty()
            << Endl;
        }
    }
    return result;
}

} // namespace NMarket

} // namespace NBASS

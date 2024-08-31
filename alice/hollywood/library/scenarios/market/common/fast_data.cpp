#include "fast_data.h"

#include <util/string/split.h>

namespace NAlice::NHollywood::NMarket {

TMarketFastData::TMarketFastData(const NProto::TMarketFastData& proto)
    : ForbiddenHids(proto.GetForbiddenHids().begin(), proto.GetForbiddenHids().end())
    , ForbiddenWords(proto.GetForbiddenWords().begin(), proto.GetForbiddenWords().end())
{}

bool TMarketFastData::ContainsVulgarQuery(const TStringBuf original) const
{
    for (const auto& it : StringSplitter(original).Split(' ').SkipEmpty()) {
        if (ForbiddenWords.contains(it.Token())) {
            return true;
        }
    }
    return false;
}

bool TMarketFastData::IsSupportedCategory(const TCategory& category) const
{
    if (category.HasHid()) {
        return !ForbiddenHids.contains(category.GetHid());
    }
    return true;
}

} // namespace NAlice::NHollywood::NMarket

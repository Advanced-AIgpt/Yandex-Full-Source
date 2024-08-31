#pragma once

#include "types.h"
#include <alice/hollywood/library/scenarios/market/common/proto/fast_data.pb.h>
#include <alice/hollywood/library/fast_data/fast_data.h>

#include <util/generic/hash_set.h>

namespace NAlice::NHollywood::NMarket {

class TMarketFastData : public IFastData {
public:
    TMarketFastData(const NProto::TMarketFastData& proto);

    bool ContainsVulgarQuery(const TStringBuf original) const;
    bool IsSupportedCategory(const TCategory& category) const;

private:
    THashSet<THid> ForbiddenHids;
    THashSet<TString> ForbiddenWords;
};

} // namespace NAlice::NHollywood::NMarket

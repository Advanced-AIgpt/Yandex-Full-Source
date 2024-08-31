#pragma once

#include <alice/library/util/rng.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywood {

struct TResponseNlg {
    TString Text;
    TString Voice;
};

template <typename TItem, typename TRestrictionsChecker>
const TItem* Recommend(const TVector<TItem>& items, IRng& rng, TRestrictionsChecker restrictionsChecker) {
    TVector<const TItem*> recommendableItems(Reserve(items.size()));
    for (const TItem& item : items) {
        if (restrictionsChecker(item)) {
            recommendableItems.push_back(&item);
        }
    }

    if (recommendableItems.empty()) {
        return nullptr;
    }

    const auto recommendedItemIndex = rng.RandomInteger(recommendableItems.size());
    return recommendableItems[recommendedItemIndex];
}

}  // namespace NAlice::NHollywood

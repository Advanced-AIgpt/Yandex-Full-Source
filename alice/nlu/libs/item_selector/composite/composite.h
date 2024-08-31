#pragma once

#include <alice/nlu/libs/item_selector/interface/item_selector.h>

#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace NAlice::NItemSelector {

    using TSelectorModels = THashMap<TSelectorName, THolder<NAlice::NItemSelector::IItemSelector>>;
    using TSelectorCollection = THashMap<TEntityType, TSelectorModels>;

    class TCompositeItemSelector : public IItemSelector {
    public:
        explicit TCompositeItemSelector(
            TSelectorCollection&& collection,
            THolder<NAlice::NItemSelector::IItemSelector>&& defaultSelector
        )
            : Collection(std::move(collection))
            , DefaultSelector(std::move(defaultSelector))
        {}

        TVector<TSelectionResult> Select(const TSelectionRequest& request, const TVector<TSelectionItem>& items) const;

    private:
        TSelectorCollection Collection;
        THolder<NAlice::NItemSelector::IItemSelector> DefaultSelector;
    };
}

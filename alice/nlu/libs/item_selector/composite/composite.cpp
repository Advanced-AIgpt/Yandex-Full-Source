#include "composite.h"

namespace NAlice::NItemSelector {

    TVector<TSelectionResult> TCompositeItemSelector::Select(const TSelectionRequest& request, const TVector<TSelectionItem>& items) const {
        if (request.EntityType.Empty() || request.SelectorName.Empty() || !Collection.contains(request.EntityType.GetRef())) {
            return DefaultSelector->Select(request, items);
        }

        return Collection.at(request.EntityType.GetRef()).ValueRef(request.SelectorName.GetRef(), DefaultSelector)->Select(request, items);
    }

}

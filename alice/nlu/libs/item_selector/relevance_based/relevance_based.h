#pragma once

#include <alice/nlu/libs/item_selector/interface/item_selector.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {
namespace NItemSelector {

class IRelevanceComputer {
public:
    virtual ~IRelevanceComputer() = default;
    virtual float ComputeRelevance(const TString& request, const TVector<TString>& synonyms) const = 0;
};

class TRelevanceBasedItemSelector final : public IItemSelector {
public:
    TRelevanceBasedItemSelector(THolder<const IRelevanceComputer> relevanceComputer, const double threshold,
                                const bool filterItemsByRequestLanguage)
        : RelevanceComputer(std::move(relevanceComputer))
        , Threshold(threshold)
        , FilterItemsByRequestLanguage(filterItemsByRequestLanguage)
    {
        Y_ENSURE(RelevanceComputer);
    }

    TVector<TSelectionResult> Select(const TSelectionRequest& request, const TVector<TSelectionItem>& items) const override;

private:
    const THolder<const IRelevanceComputer> RelevanceComputer;
    const double Threshold;
    const bool FilterItemsByRequestLanguage = false;
};

} // namespace NItemSelector
} // namespace NAlice

#pragma once

#include "easy_tagger.h"
#include "utils.h"

#include <alice/nlu/libs/item_selector/interface/item_selector.h>

#include <catboost/libs/model/model.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <functional>

namespace NAlice {
namespace NItemSelector {

using TFeatureMap = THashMap<TString, float>;

struct TCatboostSelectionRequest : public TSelectionRequest {
    TCatboostSelectionRequest() = default;

    TCatboostSelectionRequest(const TSelectionRequest& base, const TTaggerResult& taggerResult)
        : TSelectionRequest(base)
        , TaggerResult(taggerResult)
        {
        }

    TTaggerResult TaggerResult;
};

struct TCatboostSelectionItem : public TSelectionItem {
    TCatboostSelectionItem() = default;

    TCatboostSelectionItem(const TSelectionItem& base,
                           const TVector<TTaggerResult>& taggerResults,
                           const TMaybe<TTaggerResult>& positionTaggerResult)
        : TSelectionItem(base)
        , TaggerResults(taggerResults)
        , PositionTaggerResult(positionTaggerResult)
        {
        }

    // It is supposed that each of Synonyms has its own tagging
    TVector<TTaggerResult> TaggerResults;
    TMaybe<TTaggerResult> PositionTaggerResult;
};

TFeatureMap ComputeFeatureMap(const TCatboostSelectionRequest& request,
                              const TCatboostSelectionItem& item,
                              const bool removeBioPrefix);

struct TCatboostItemSelectorSpec {
    THashMap<TString, size_t> FeatureSpec;
    size_t FeatureVectorSize = 0;
    double SelectionThreshold = 0.5;
    bool RemoveBioPrefix = true;
};

TVector<float> ComputeFeatures(const TCatboostSelectionRequest& request,
                               const TCatboostSelectionItem& item,
                               const TCatboostItemSelectorSpec spec);

class TCatboostItemSelector {
public:
    TCatboostItemSelector(const TFullModel& catboostModel, const TCatboostItemSelectorSpec& spec)
        : CatboostModel(catboostModel)
        , Spec(spec)
    {
    }

    TVector<TSelectionResult> Select(const TCatboostSelectionRequest& request,
                                     const TVector<TCatboostSelectionItem>& items) const;

private:
    TFullModel CatboostModel;
    const TCatboostItemSelectorSpec Spec;
};

class TEasyCatboostItemSelector : public IItemSelector {
public:
    TEasyCatboostItemSelector(const TEasyTagger& easyTagger, const TCatboostItemSelector& catboostItemSelector)
        : EasyTagger(easyTagger)
        , CatboostItemSelector(catboostItemSelector)
    {
    }

    TVector<TSelectionResult> Select(const TSelectionRequest& request,
                                     const TVector<TSelectionItem>& items) const;

private:
    const TEasyTagger EasyTagger;
    TCatboostItemSelector CatboostItemSelector;
};

} // namespace NItemSelector
} // namespace NAlice

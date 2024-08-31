#include "relevance_based.h"

#include <alice/nlu/libs/item_selector/common/common.h>

#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>
#include <util/string/cast.h>

namespace NAlice {
namespace NItemSelector {
namespace {

constexpr double NO_INSTANCES = -2;
constexpr double NEGATIVE_MATCH = -1;
constexpr double POSITIVE_EXACT = 1;

double ComputeRelevance(const TSelectionRequest& request, const TSelectionItem& item,
                        const IRelevanceComputer& relevanceComputer) {
    TVector<TString> positiveTexts = GetTexts(item.Synonyms);

    if (const TVideoItemMeta* meta = std::get_if<TVideoItemMeta>(&item.Meta); meta) {
        const TVector<TString> metaTexts = {
            ToString(meta->Position),
            ToString(meta->Episode),
            ToString(meta->EpisodesCount),
            meta->Genre,
            meta->ProviderName,
            ToString(meta->Season),
            ToString(meta->SeasonsCount),
            ToString(meta->MinAge),
            ToString(meta->ReleaseYear),
            meta->Type
        };
        positiveTexts.insert(positiveTexts.end(), metaTexts.begin(), metaTexts.end());
    }

    if (!item.Synonyms) {
        return NO_INSTANCES;
    }
    if (IsIn(positiveTexts, request.Phrase.Text)) {
        return POSITIVE_EXACT;
    }
    const double positiveRelevance = relevanceComputer.ComputeRelevance(request.Phrase.Text, positiveTexts);
    if (!item.Negatives) {
        return positiveRelevance;
    }
    const TVector<TString> negativeTexts = GetTexts(item.Negatives);
    const double negativeRelevance = relevanceComputer.ComputeRelevance(request.Phrase.Text, negativeTexts);
    if (IsIn(negativeTexts, request.Phrase.Text) || negativeRelevance > positiveRelevance) {
        return NEGATIVE_MATCH;
    }
    return positiveRelevance;
}

TVector<double> ComputeRelevances(const TSelectionRequest& request, const TVector<TSelectionItem>& items,
                                  const IRelevanceComputer& relevanceComputer, const bool filterItemsByRequestLanguage) {
    TVector<double> relevances;
    const TSelectionRequest lowercasedRequest = Lowercase(request);
    for (const TSelectionItem& item : items) {
        if (filterItemsByRequestLanguage) {
            relevances.push_back(ComputeRelevance(
                lowercasedRequest,
                FilterByLanguage(Lowercase(item), request.Phrase.Language),
                relevanceComputer
            ));
        } else {
            relevances.push_back(ComputeRelevance(lowercasedRequest, Lowercase(item), relevanceComputer));
        }
    }
    return relevances;
}

} // anonymous namespace

TVector<TSelectionResult> TRelevanceBasedItemSelector::Select(const TSelectionRequest& request, const TVector<TSelectionItem>& items) const {
    if (items.empty()) {
        return {};
    }
    const TVector<double> relevances = ComputeRelevances(request, items, *RelevanceComputer, FilterItemsByRequestLanguage);
    if (relevances.empty()) {
        return {};
    }
    const double maxScore = *MaxElement(relevances.begin(), relevances.end());

    TVector<TSelectionResult> result(Reserve(relevances.size()));
    for (size_t i = 0; i < relevances.size(); ++i) {
        result.push_back({relevances[i], FuzzyEquals(relevances[i], maxScore) && relevances[i] > Threshold});
    }
    return result;
}

} // namespace NItemSelector
} // namespace NAlice

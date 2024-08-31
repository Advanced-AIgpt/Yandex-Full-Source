#pragma once

#include <alice/bass/libs/video_common/utils.h>

#include <util/generic/algorithm.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>

#include <cstddef>

namespace NVideoCommon {
namespace NHasGoodResult {
struct TFactors {
    static constexpr size_t TOP_RESULTS = 3;

    static float CalcSimilarity(TStringBuf query, TStringBuf name);

    template <typename TResults>
    void FromResults(TStringBuf originalQuery, TStringBuf refinedQuery, const TResults& results) {
        Clear();

        TVector<float> relevances;
        TVector<float> relevancePredictions;
        TVector<float> similarities;
        TVector<float> ratings;

        for (const auto& result : results) {
            relevances.emplace_back(NormalizeRelevance(result.Relevance()));
            relevancePredictions.emplace_back(result.RelevancePrediction());

            const float similarity =
                Max(CalcSimilarity(originalQuery, result.Name()), CalcSimilarity(refinedQuery, result.Name()));
            similarities.emplace_back(ClampVal(similarity, 0.0f, 1.0f));

            ratings.emplace_back(result.Rating());
        }

        Sort(relevances.rbegin(), relevances.rend());
        Sort(relevancePredictions.rbegin(), relevancePredictions.rend());
        Sort(similarities.rbegin(), similarities.rend());

        for (size_t i = 0; i < TOP_RESULTS; ++i) {
            if (i < relevances.size())
                Relevances[i] = relevances[i];
            if (i < relevancePredictions.size())
                RelevancePredictions[i] = relevancePredictions[i];
            if (i < similarities.size())
                Similarities[i] = similarities[i];
        }
    }

    template <typename TItem>
    void FromItem(const TItem& item) {
        FromResults(item.Query(), item.RefinedQuery(), item.Results());
    }

    static void GetNames(TVector<TString>& names);
    void GetValues(TVector<float>& values) const;

    void Clear();

    // Top relevances, sorted in descending order.
    float Relevances[TOP_RESULTS] = {};

    // Top relevance predictions, sorted in descending order.
    float RelevancePredictions[TOP_RESULTS] = {};

    // Top similarities by name.
    float Similarities[TOP_RESULTS] = {};
};
} // namespace NHasGoodResult
} // namespace NVideoCommon

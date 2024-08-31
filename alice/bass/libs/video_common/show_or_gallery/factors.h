#pragma once

#include <alice/bass/libs/video_common/show_or_gallery/video.sc.h>
#include <alice/bass/libs/video_common/utils.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/utility.h>

#include <algorithm>
#include <cstddef>

namespace NVideoCommon {
namespace NShowOrGallery {
struct TFactors {
    static constexpr size_t TOP_RESULTS = 3;

    static float CalcSimilarity(TStringBuf query, TStringBuf name);

    struct TEntry {
        template <typename TResult>
        void FromResult(TStringBuf originalQuery, TStringBuf refinedQuery, const TResult& result) {
            RelevancePrediction = result.RelevancePrediction();
            Relevance = NormalizeRelevance(result.Relevance());
            Rating = result.Rating();

            const float similarity =
                Max(CalcSimilarity(originalQuery, result.Name()), CalcSimilarity(refinedQuery, result.Name()));
            Similarity = ClampVal(similarity, 0.0f, 1.0f);
        }

        void Clear();

        template <typename TVisitor>
        void Visit(TVisitor& visitor) const {
            visitor(RelevancePrediction, "relevance_prediction");
            visitor(Relevance, "relevance");
            visitor(Rating, "rating");
            visitor(Similarity, "similarity");
        }

        float RelevancePrediction = 0;
        float Relevance = 0;
        float Rating = 0;
        float Similarity = 0;
    };

    template <typename TResults>
    void FromResults(TStringBuf originalQuery, TStringBuf refinedQuery, const TResults& results) {
        // This constant is used to prevent division by small number.
        // I.e. we assume that all relevance-predictions that are less
        // than EPS are equal to zero.
        constexpr float EPS = 1e-6;

        for (auto& entry : Entries)
            entry.Clear();

        size_t i = 0;
        for (const auto& result : results) {
            if (i >= TOP_RESULTS)
                break;
            Entries[i].FromResult(originalQuery, refinedQuery, result);
            ++i;
        }

        const float topRelevance = Entries[0].Relevance;
        for (size_t i = 1; i < TOP_RESULTS; ++i) {
            if (topRelevance < EPS)
                RelevanceRatios[i - 1] = 1;
            else
                RelevanceRatios[i - 1] = Entries[i].Relevance / topRelevance;
        }
    }

    template <typename TItem>
    void FromItem(const TItem& item) {
        FromResults(item.Query().Text(), item.Query().SearchText(), item.Results());
    }

    void GetNames(TVector<TString>& names) const;
    void GetValues(TVector<float>& values) const;

    TEntry Entries[TOP_RESULTS];
    float RelevanceRatios[TOP_RESULTS - 1] = {};
};
} // namespace NShowOrGallery
} // namespace NVideoCommon
